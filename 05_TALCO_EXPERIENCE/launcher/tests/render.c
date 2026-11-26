#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ui/launcher.h"
static int render(const char *file, int w, int h, int dpi, int expect_tablet)
{
    fb_t f = { .px = calloc((size_t)w * h, 4), .w = w, .h = h };
    form_t ff = ui_form_factor(w, h, dpi);
    if ((ff == FF_TABLET) != expect_tablet) { printf("ECHEC detection form factor %s\n", file); return 1; }
    launcher_draw(&f, dpi, 9, 41, 78);
    int n = launcher_hit_count(), x, y, a, bad = 0;
    for (int k = 0; k < n; k++) { launcher_hit_center(k, &x, &y, &a); if (launcher_hit(x, y) != a) bad++; }
    if (!n || bad) { printf("ECHEC zones tactiles %s (%d/%d)\n", file, bad, n); return 1; }
    printf("%s : %d zones tactiles OK\n", file, n);
    FILE *o = fopen(file, "wb");
    fprintf(o, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) { unsigned c = f.px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o); free(f.px);
    printf("%s : %dx%d @%d dpi -> %s\n", file, w, h, dpi, ff == FF_TABLET ? "tablette" : "telephone");
    return 0;
}
static int calc_tests(void)
{
    struct { const char *keys, *want; } t[] = { {"12+30=", "42"}, {"C7/2=", "3.5"}, {"C5/0=", "Erreur"}, {"C0.1+0.2=", "0.3"},
        {"C9-12=", "-3"}, {"C2*3+4=", "10"}, {"C100*100=", "10000"}, {"C1.5.5", "1.55"} };
    int bad = 0;
    for (unsigned i = 0; i < sizeof t / sizeof *t; i++) {
        for (const char *k = t[i].keys; *k; k++) calc_press(*k);
        if (strcmp(calc_display(), t[i].want)) { printf("ECHEC calc %s -> %s (attendu %s)\n", t[i].keys, calc_display(), t[i].want); bad++; }
    }
    /* taps simules sur les touches dessinees */
    fb_t f = { .px = calloc(720 * 1560, 4), .w = 720, .h = 1560 };
    calc_press('C'); launcher_draw(&f, 320, 9, 41, 78); launcher_app_window(&f, 320, 10);
    for (const char *k = "8*7="; *k; k++) { int x, y; if (!launcher_calc_key_center(*k, &x, &y) || !launcher_app_tap(x, y, 10)) bad++; }
    if (strcmp(calc_display(), "56")) { printf("ECHEC taps calc -> %s\n", calc_display()); bad++; }
    launcher_app_window(&f, 320, 10);
    FILE *o = fopen("calc.ppm", "wb"); fprintf(o, "P6\n720 1560\n255\n");
    for (int i = 0; i < 720 * 1560; i++) { unsigned c = f.px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o); free(f.px);
    printf("calculatrice : %s\n", bad ? "ECHEC" : "OK (8 calculs + taps)");
    /* notes : taps sur le clavier */
    f.px = calloc(720 * 1560, 4); int nb = 0;
    launcher_draw(&f, 320, 9, 41, 78); launcher_app_window(&f, 320, 11);
    const char *seq = "bonjour le monde<<<e\n1";
    for (const char *k = seq; *k; k++) {
        int x, y; if (!launcher_notes_key_center(*k, &x, &y) || !launcher_app_tap(x, y, 11)) nb++;
        launcher_app_window(&f, 320, 11);
    }
    if (strcmp(notes_text(), "bonjour le moe\n1")) { printf("ECHEC notes -> [%s]\n", notes_text()); nb++; }
    launcher_app_window(&f, 320, 11);
    o = fopen("notes.ppm", "wb"); fprintf(o, "P6\n720 1560\n255\n");
    for (int i = 0; i < 720 * 1560; i++) { unsigned c = f.px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o); free(f.px);
    printf("notes : %s\n", nb ? "ECHEC" : "OK (clavier AZERTY, effacer, entree)");
    return bad + nb;
}
static void ppm(const char *file, fb_t *f)
{
    FILE *o = fopen(file, "wb"); fprintf(o, "P6\n%d %d\n255\n", f->w, f->h);
    for (int i = 0; i < f->w * f->h; i++) { unsigned c = f->px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o);
}
static int mgr_tests(void)
{
    int bad = 0, ev, arg, x, y;
#define CK(c) do { if (!(c)) { printf("ECHEC ligne %d : %s\n", __LINE__, #c); bad++; } } while (0)
    launcher_set_host_apps(1);
    CK(launcher_app_internal(6) && launcher_app_internal(13));
    launcher_app_opened(6); ev = launcher_event(&arg); CK(ev == EV_FM_ENTER);
    launcher_fm_set_volumes("Stockage interne\t128000000000\t41500000000\nCarte SD\t64000000000\t12300000000\n", 1);
    char data[16384]; data[0] = 0; char ln[200];
    const char *dirs[] = {"Download", "DCIM", "Documents", "Musique", "Android", "Films", "Écoles"};
    for (int i = 0; i < 7; i++) { snprintf(ln, sizeof ln, "%s\t0\t%d\t1\n", dirs[i], 1790000000 + i * 40000); strcat(data, ln); }
    const char *ex[] = {"jpg", "png", "mp4", "pdf", "mp3", "zip", "txt", "apk"};
    for (int i = 0; i < 40; i++) {
        snprintf(ln, sizeof ln, "%s_%03d.%s\t%d\t%d\t0\n", i % 3 ? "photo" : "Rapport", i, ex[i % 8], (i * 7919) % 90000000 + 1200, 1780000000 + i * 123457);
        strcat(data, ln);
    }
    launcher_fm_set_dir("/DCIM/Camera", data);
    CK(launcher_fm_count() == 47);
    CK(!strcmp(launcher_fm_name(0), "Android") && !strcmp(launcher_fm_name(6), "Écoles"));        /* dossiers d'abord, tri par nom */
    launcher_fm_sort(1, 1);
    int okz = 1; for (int i = 8; i < 47; i++) if (launcher_fm_size(i) > launcher_fm_size(i - 1)) okz = 0;
    CK(okz && launcher_fm_size(7) >= launcher_fm_size(8));                                          /* taille decroissante */
    launcher_fm_sort(2, 1);
    int okd = 1; for (int i = 8; i < 47; i++) if (launcher_fm_mtime(i) > launcher_fm_mtime(i - 1)) okd = 0;
    CK(okd);                                                                                         /* date decroissante */
    launcher_fm_sort(3, 0);
    const char *e1 = strrchr(launcher_fm_name(7), '.'), *e2 = strrchr(launcher_fm_name(46), '.');
    CK(e1 && e2 && strcmp(e1, e2) <= 0);                                                             /* type croissant */
    launcher_fm_sort(0, 0);
    fb_t f = { .px = calloc(1080 * 2340, 4), .w = 1080, .h = 2340 };
    launcher_render(&f, 420, 9, 41, 78, 6);
    CK(launcher_fm_row_center(0, &x, &y) && launcher_app_tap(x, y, 6) == 1);
    ev = launcher_event(&arg); CK(ev == EV_FM_OPEN && arg == launcher_fm_order(0));                 /* tap ligne -> ouvrir */
    CK(launcher_long_press(x, y, 6) && launcher_event(&arg) == EV_FM_MENU && arg == launcher_fm_order(0));
    CK(launcher_fm_chip_center(1, &x, &y) && launcher_app_tap(x, y, 6) == 1);                        /* tap sur « Taille » */
    CK(launcher_fm_size(7) >= launcher_fm_size(8) || launcher_fm_size(7) == launcher_fm_size(8));
    launcher_scroll(6, 900); launcher_render(&f, 420, 9, 41, 78, 6);
    int y2; CK(launcher_fm_row_center(8, &x, &y2));                                                 /* defilement */
    launcher_scroll(6, -2000); launcher_fm_sort(0, 0); launcher_render(&f, 420, 9, 41, 78, 6);
    ppm("files.ppm", &f);
    launcher_set_clipboard(1); launcher_render(&f, 420, 9, 41, 78, 6);
    /* reglages */
    launcher_app_opened(13); ev = launcher_event(&arg); CK(ev == EV_SET_ENTER);
    launcher_info_set("Appareil\tSamsung Galaxy A34 5G\t-1\nAndroid\t14 (API 34)\t-1\nStockage interne\t41,5 Go libres sur 128 Go\t68\nCarte SD\t12,3 Go libres sur 64 Go\t81\n"
                      "Mémoire (RAM)\t2,1 Go libres sur 6 Go\t65\nBatterie\t78 % · en charge · 31 °C\t78\nWi-Fi\tActivé\t-1\nCarte SIM\tPrête · Orange F\t-1\nLuminosité\t55 %\t-1\n");
    launcher_set_volume_pct(40);
    launcher_render(&f, 420, 9, 41, 78, 13);
    for (int i = 0; i < 10; i++) { if (launcher_sc_button_center(i, &x, &y)) { CK(launcher_app_tap(x, y, 13) == 1); ev = launcher_event(&arg); CK(ev == EV_SET_OPEN && arg == i); } }
    launcher_scroll(13, 700); launcher_render(&f, 420, 9, 41, 78, 13);
    for (int i = 0; i < 10; i++) { if (launcher_sc_button_center(i, &x, &y)) { CK(launcher_app_tap(x, y, 13) == 1); ev = launcher_event(&arg); CK(ev == EV_SET_OPEN && arg == i); } }
    launcher_scroll(13, -2000); launcher_render(&f, 420, 9, 41, 78, 13);
    { int sx, sy, sw; CK(launcher_sc_slider_rect(&sx, &sy, &sw)); CK(launcher_app_tap(sx + sw / 2, sy, 13) == 1); ev = launcher_event(&arg); CK(ev == EV_SET_VOLUME && arg >= 49 && arg <= 51); }
    launcher_render(&f, 420, 9, 41, 78, 13);
    ppm("settings.ppm", &f); free(f.px);
    printf("fichiers et reglages : %s\n", bad ? "ECHEC" : "OK (tri, defilement, evenements, reglages)");
    launcher_set_host_apps(0);
    return bad;
}
int main(void)
{
    int e = calc_tests() + mgr_tests();
    if (!launcher_app_internal(10) || !launcher_app_internal(11) || launcher_app_internal(0) || launcher_app_internal(13)) { printf("ECHEC apps internes/externes\n"); e = 1; }
    else printf("apps internes (Calc, Notes) / externes (Android) : OK\n");
    e |= render("phone.ppm", 720, 1560, 320, 0);
    e |= render("tablet.ppm", 1600, 1000, 240, 1);
    e |= render("a34.ppm", 1080, 2340, 420, 0);        /* Samsung Galaxy A34 5G : 1080x2340, ~420 dpi */
    return e;
}
