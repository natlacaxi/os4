/* TALCO UI : affiche le launcher (telephone/tablette) sur /dev/fb0 et gere le tactile (/dev/input).
 * Mode hote : talco_ui --render fichier.ppm LARGEUR HAUTEUR [index_app]  (rendu sans ecran, pour tests) */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>
#include "launcher.h"

static int dpi_for(int w, int h) { return h > w ? 320 : 200; }

static void compose(fb_t *f, int dpi, int app)
{
    time_t t = time(0); struct tm *tm = localtime(&t);
    if (tm) launcher_set_date(tm->tm_wday, tm->tm_mday, tm->tm_mon);
    launcher_draw(f, dpi, tm ? tm->tm_hour : 9, tm ? tm->tm_min : 41, 78);
    if (app >= 0) launcher_app_window(f, dpi, app);
}

static int render_file(const char *path, int w, int h, int app, const char *keys)
{
    fb_t f = { calloc((size_t)w * h, 4), w, h };
    for (; keys && *keys; keys++) calc_press(*keys);
    compose(&f, dpi_for(w, h), app);
    FILE *o = fopen(path, "wb");
    if (!o) return 1;
    fprintf(o, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) { unsigned c = f.px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o);
    return 0;
}

#define MAXDEV 16
static int ifd[MAXDEV], imin_x[MAXDEV], imax_x[MAXDEV], imin_y[MAXDEV], imax_y[MAXDEV], icur_x[MAXDEV], icur_y[MAXDEV], n_in;

static void scan_inputs(void)
{
    char p[32];
    for (int i = 0; i < 32 && n_in < MAXDEV; i++) {
        snprintf(p, sizeof p, "/dev/input/event%d", i);
        int already = 0;
        int fd = open(p, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        unsigned long ev[(EV_MAX + 8 * sizeof(long)) / (8 * sizeof(long))] = {0};
        ioctl(fd, EVIOCGBIT(0, sizeof ev), ev);
        struct input_absinfo ax, ay;
        if (!(ev[0] & (1UL << EV_ABS)) || ioctl(fd, EVIOCGABS(ABS_X), &ax) < 0 || ioctl(fd, EVIOCGABS(ABS_Y), &ay) < 0) { close(fd); continue; }
        for (int k = 0; k < n_in; k++) already |= 0;
        ifd[n_in] = fd; imin_x[n_in] = ax.minimum; imax_x[n_in] = ax.maximum;
        imin_y[n_in] = ay.minimum; imax_y[n_in] = ay.maximum; n_in++;
    }
}

int main(int argc, char **argv)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);     /* heure de Paris, sans base de fuseaux */
    tzset();
    if (argc >= 5 && !strcmp(argv[1], "--render"))
        return render_file(argv[2], atoi(argv[3]), atoi(argv[4]), argc > 5 ? atoi(argv[5]) : -1, argc > 6 ? argv[6] : 0);

    int fd = -1;
    for (int i = 0; i < 30 && fd < 0; i++) { fd = open("/dev/fb0", O_RDWR); if (fd < 0) sleep(1); }
    if (fd < 0) { fprintf(stderr, "talco-ui: pas de /dev/fb0 (pas de pilote ecran)\n"); return 0; }
    struct fb_var_screeninfo v; struct fb_fix_screeninfo fx;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &v) || ioctl(fd, FBIOGET_FSCREENINFO, &fx)) return 1;
    if (v.bits_per_pixel != 32) { fprintf(stderr, "talco-ui: %u bpp non gere\n", v.bits_per_pixel); return 1; }
    int tty = open("/dev/tty0", O_WRONLY);                 /* cache le curseur texte de la console */
    if (tty >= 0) { (void)!write(tty, "\033[?25l", 6); close(tty); }
    unsigned char *map = mmap(0, fx.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) { fprintf(stderr, "talco-ui: mmap impossible\n"); return 1; }
    int W = (int)v.xres, H = (int)v.yres, dpi = dpi_for(W, H);
    fprintf(stderr, "talco-ui: ecran %dx%d, %s\n", W, H, ui_form_factor(W, H, dpi) == FF_TABLET ? "tablette" : "telephone");
    fb_t f = { calloc((size_t)W * H, 4), W, H };
    int app = -1, lastmin = -1, swap = v.red.offset != 16;

    for (;;) {
        time_t t = time(0); struct tm *tm = localtime(&t);
        int redraw = (tm && tm->tm_min != lastmin);
        if (tm) lastmin = tm->tm_min;
        if (!n_in) scan_inputs();
        struct pollfd pf[MAXDEV];
        for (int i = 0; i < n_in; i++) { pf[i].fd = ifd[i]; pf[i].events = POLLIN; }
        if (n_in) poll(pf, (nfds_t)n_in, 1000); else sleep(1);
        for (int i = 0; i < n_in; i++) {
            struct input_event e;
            while (read(ifd[i], &e, sizeof e) == (ssize_t)sizeof e) {
                if (e.type == EV_ABS && e.code == ABS_X) icur_x[i] = (int)((long)(e.value - imin_x[i]) * W / (imax_x[i] - imin_x[i] + 1));
                if (e.type == EV_ABS && e.code == ABS_Y) icur_y[i] = (int)((long)(e.value - imin_y[i]) * H / (imax_y[i] - imin_y[i] + 1));
                if (e.type == EV_KEY && (e.code == BTN_LEFT || e.code == BTN_TOUCH) && e.value == 1) {
                    if (app >= 0) { if (!launcher_app_tap(icur_x[i], icur_y[i], app)) app = -1; }
                    else app = launcher_hit(icur_x[i], icur_y[i]);
                    redraw = 1;
                }
            }
        }
        if (!redraw && lastmin >= 0 && f.px[0]) { /* rien a refaire */ }
        if (redraw || !f.px[0]) {
            compose(&f, dpi, app);
            for (int y = 0; y < H; y++) {
                unsigned char *dst = map + (size_t)y * fx.line_length;
                if (!swap) memcpy(dst, &f.px[y * W], (size_t)W * 4);
                else for (int x = 0; x < W; x++) { unsigned c = f.px[y * W + x]; unsigned o = (c & 0xFF00FF00u) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF); memcpy(dst + x * 4, &o, 4); }
            }
        }
    }
}
