#include <stdio.h>
#include <stdlib.h>
#include "../ui/launcher.h"
static int render(const char *file, int w, int h, int dpi, int expect_tablet)
{
    fb_t f = { calloc((size_t)w * h, 4), w, h };
    form_t ff = ui_form_factor(w, h, dpi);
    if ((ff == FF_TABLET) != expect_tablet) { printf("ECHEC detection form factor %s\n", file); return 1; }
    launcher_draw(&f, dpi, 9, 41, 78);
    FILE *o = fopen(file, "wb");
    fprintf(o, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) { unsigned c = f.px[i]; fputc(c >> 16, o); fputc(c >> 8, o); fputc(c, o); }
    fclose(o); free(f.px);
    printf("%s : %dx%d @%d dpi -> %s\n", file, w, h, dpi, ff == FF_TABLET ? "tablette" : "telephone");
    return 0;
}
int main(void)
{
    int e = 0;
    e |= render("phone.ppm", 720, 1560, 320, 0);
    e |= render("tablet.ppm", 1600, 1000, 240, 1);
    return e;
}
