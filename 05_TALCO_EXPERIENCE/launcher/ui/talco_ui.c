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
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <stdint.h>
#include "launcher.h"

static int dpi_for(int w, int h) { return h > w ? 320 : 200; }
/* densite imposee par la ligne de commande du noyau : talco.dpi=420 (utile pour un vrai telephone) */
static int cmdline_dpi(void)
{
    char b[2048] = {0}; FILE *f = fopen("/proc/cmdline", "r");
    if (!f) return 0;
    size_t n = fread(b, 1, sizeof b - 1, f); (void)n; fclose(f);
    char *p = strstr(b, "talco.dpi=");
    return p ? atoi(p + 10) : 0;
}

static void compose(fb_t *f, int dpi, int app)
{
    time_t t = time(0); struct tm *tm = localtime(&t);
    if (tm) launcher_set_date(tm->tm_wday, tm->tm_mday, tm->tm_mon);
    launcher_render(f, dpi, tm ? tm->tm_hour : 9, tm ? tm->tm_min : 41, 78, app);
}

static int render_file(const char *path, int w, int h, int app, const char *keys)
{
    fb_t f = { calloc((size_t)w * h, 4), w, h };
    for (; keys && *keys; keys++) calc_press(*keys);
    const char *e = getenv("TALCO_DPI");
    compose(&f, e ? atoi(e) : dpi_for(w, h), app);
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


/* ===== Rendu 1 : DRM/KMS, 2 tampons + page flip synchronise sur le vblank (methode recommandee) ===== */
#define P64(p) ((unsigned long long)(uintptr_t)(p))
static struct { int fd; unsigned crtc, conn, W, H, fb[2], pitch[2]; unsigned char *map[2]; size_t size[2]; int cur, pending;
                struct drm_mode_modeinfo mode; } drm;

static int drm_open(int *W, int *H)
{
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) return -1;
    ioctl(fd, DRM_IOCTL_SET_MASTER, 0);
    struct drm_mode_card_res r; memset(&r, 0, sizeof r);
    if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &r) || !r.count_connectors || !r.count_crtcs) { close(fd); return -1; }
    uint32_t nc = r.count_connectors, ncr = r.count_crtcs;
    uint32_t *cons = calloc(nc, 4), *crtcs = calloc(ncr, 4);
    memset(&r, 0, sizeof r);
    r.count_connectors = nc; r.connector_id_ptr = P64(cons); r.count_crtcs = ncr; r.crtc_id_ptr = P64(crtcs);
    if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &r)) { close(fd); return -1; }
    int found = 0;
    for (uint32_t i = 0; i < nc && !found; i++) {
        struct drm_mode_get_connector c; memset(&c, 0, sizeof c); c.connector_id = cons[i];
        if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &c) || c.connection != 1 || !c.count_modes || !c.count_encoders) continue;
        uint32_t nm = c.count_modes, ne = c.count_encoders;
        struct drm_mode_modeinfo *modes = calloc(nm, sizeof *modes); uint32_t *encs = calloc(ne, 4);
        memset(&c, 0, sizeof c); c.connector_id = cons[i];
        c.count_modes = nm; c.modes_ptr = P64(modes); c.count_encoders = ne; c.encoders_ptr = P64(encs);
        if (!ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &c)) {
            struct drm_mode_get_encoder e; memset(&e, 0, sizeof e); e.encoder_id = c.encoder_id ? c.encoder_id : encs[0];
            if (!ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &e)) {
                unsigned crtc = e.crtc_id;
                for (uint32_t k = 0; !crtc && k < ncr; k++) if (e.possible_crtcs & (1u << k)) crtc = crtcs[k];
                if (crtc) { drm.mode = modes[0]; drm.conn = cons[i]; drm.crtc = crtc; found = 1; }
            }
        }
        free(modes); free(encs);
    }
    free(cons); free(crtcs);
    if (!found) { close(fd); return -1; }
    drm.fd = fd; drm.W = drm.mode.hdisplay; drm.H = drm.mode.vdisplay;
    for (int b = 0; b < 2; b++) {
        struct drm_mode_create_dumb cd; memset(&cd, 0, sizeof cd); cd.width = drm.W; cd.height = drm.H; cd.bpp = 32;
        if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &cd)) { close(fd); return -1; }
        struct drm_mode_fb_cmd fb; memset(&fb, 0, sizeof fb);
        fb.width = drm.W; fb.height = drm.H; fb.pitch = cd.pitch; fb.bpp = 32; fb.depth = 24; fb.handle = cd.handle;
        if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fb)) { close(fd); return -1; }
        struct drm_mode_map_dumb md; memset(&md, 0, sizeof md); md.handle = cd.handle;
        if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &md)) { close(fd); return -1; }
        void *m = mmap(0, cd.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, md.offset);
        if (m == MAP_FAILED) { close(fd); return -1; }
        memset(m, 0, cd.size);
        drm.fb[b] = fb.fb_id; drm.pitch[b] = cd.pitch; drm.map[b] = m; drm.size[b] = cd.size;
    }
    struct drm_mode_crtc sc; memset(&sc, 0, sizeof sc);
    uint32_t conn = drm.conn;
    sc.set_connectors_ptr = P64(&conn); sc.count_connectors = 1; sc.crtc_id = drm.crtc; sc.fb_id = drm.fb[0];
    sc.mode_valid = 1; sc.mode = drm.mode;
    if (ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &sc)) { close(fd); return -1; }
    *W = (int)drm.W; *H = (int)drm.H;
    return 0;
}
static void drm_present(const unsigned *px)
{
    if (drm.pending) {                                   /* attend la fin du flip precedent (vblank) */
        struct pollfd p = { drm.fd, POLLIN, 0 }; char ev[128];
        if (poll(&p, 1, 200) > 0) (void)!read(drm.fd, ev, sizeof ev);
        drm.pending = 0;
    }
    int b = 1 - drm.cur;
    for (unsigned y = 0; y < drm.H; y++) memcpy(drm.map[b] + (size_t)y * drm.pitch[b], &px[y * drm.W], (size_t)drm.W * 4);
    struct drm_mode_crtc_page_flip pf; memset(&pf, 0, sizeof pf);
    pf.crtc_id = drm.crtc; pf.fb_id = drm.fb[b]; pf.flags = DRM_MODE_PAGE_FLIP_EVENT;
    if (!ioctl(drm.fd, DRM_IOCTL_MODE_PAGE_FLIP, &pf)) { drm.pending = 1; drm.cur = b; return; }
    struct drm_mode_crtc sc; memset(&sc, 0, sizeof sc);            /* repli : modeset direct */
    uint32_t conn = drm.conn;
    sc.set_connectors_ptr = P64(&conn); sc.count_connectors = 1; sc.crtc_id = drm.crtc; sc.fb_id = drm.fb[b];
    sc.mode_valid = 1; sc.mode = drm.mode;
    if (!ioctl(drm.fd, DRM_IOCTL_MODE_SETCRTC, &sc)) drm.cur = b;
}

/* ===== Rendu 2 : fbdev (repli quand le noyau n'a pas de DRM) ===== */
static struct { int fd, swap; unsigned char *map; unsigned line; int W, H; } fbd;
static int fbdev_open(int *W, int *H)
{
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return -1;
    struct fb_var_screeninfo v; struct fb_fix_screeninfo fx;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &v) || ioctl(fd, FBIOGET_FSCREENINFO, &fx) || v.bits_per_pixel != 32) { close(fd); return -1; }
    void *m = mmap(0, fx.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) { close(fd); return -1; }
    fbd.fd = fd; fbd.map = m; fbd.line = fx.line_length; fbd.swap = v.red.offset != 16; fbd.W = (int)v.xres; fbd.H = (int)v.yres;
    *W = fbd.W; *H = fbd.H;
    return 0;
}
static void fbdev_present(const unsigned *px)
{
    for (int y = 0; y < fbd.H; y++) {
        unsigned char *dst = fbd.map + (size_t)y * fbd.line;
        if (!fbd.swap) memcpy(dst, &px[y * fbd.W], (size_t)fbd.W * 4);
        else for (int x = 0; x < fbd.W; x++) { unsigned c = px[y * fbd.W + x]; unsigned o = (c & 0xFF00FF00u) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF); memcpy(dst + x * 4, &o, 4); }
    }
}

int main(int argc, char **argv)
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);     /* heure de Paris, sans base de fuseaux */
    tzset();
    if (argc >= 5 && !strcmp(argv[1], "--render"))
        return render_file(argv[2], atoi(argv[3]), atoi(argv[4]), argc > 5 ? atoi(argv[5]) : -1, argc > 6 ? argv[6] : 0);

    int mode = 0, W = 0, H = 0;                          /* 1 = DRM/KMS, 2 = fbdev */
    for (int i = 0; i < 30 && !mode; i++) {
        if (drm_open(&W, &H) == 0) mode = 1;
        else if (fbdev_open(&W, &H) == 0) mode = 2;
        else sleep(1);
    }
    if (!mode) { fprintf(stderr, "talco-ui: pas d'ecran (ni DRM ni fbdev)\n"); return 0; }
    int dpi = cmdline_dpi();
    if (dpi < 100 || dpi > 800) dpi = dpi_for(W, H);
    fprintf(stderr, "talco-ui: ecran %dx%d, %s (dpi %d, rendu %s)\n", W, H,
            ui_form_factor(W, H, dpi) == FF_TABLET ? "tablette" : "telephone", dpi, mode == 1 ? "drm" : "fbdev");
    int tty = open("/dev/tty0", O_WRONLY);                 /* cache le curseur texte de la console */
    if (tty >= 0) { (void)!write(tty, "\033[?25l", 6); close(tty); }
    fb_t f = { calloc((size_t)W * H, 4), W, H };
    int app = -1, lastmin = -1, first = 1;

    for (;;) {
        time_t t = time(0); struct tm *tm = localtime(&t);
        int redraw = first || (tm && tm->tm_min != lastmin);
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
        if (redraw) {
            first = 0;
            compose(&f, dpi, app);
            if (mode == 1) drm_present(f.px); else fbdev_present(f.px);
        }
    }
}
