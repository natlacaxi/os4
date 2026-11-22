#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ui/launcher.h"
static int render(const char *file, int w, int h, int dpi, int expect_tablet)
{
    fb_t f = { calloc((size_t)w * h, 4), w, h };
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
    fb_t f = { calloc(720 * 1560, 4), 720, 1560 };
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
int main(void)
{
    int e = calc_tests();
    e |= render("phone.ppm", 720, 1560, 320, 0);
    e |= render("tablet.ppm", 1600, 1000, 240, 1);
    return e;
}
