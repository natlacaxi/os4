#include "launcher.h"

static int dpi_;
static int D(int dp) { return dp * dpi_ / 160; }                      /* dp -> pixels */

form_t ui_form_factor(int w, int h, int dpi)
{
    int sw = (w < h ? w : h) * 160 / dpi;
    return sw >= 600 ? FF_TABLET : FF_PHONE;
}

typedef struct { const char *name; u32 color; } app_t;
static const app_t apps[] = {
    {"Phone", 0xFF34C759}, {"Messages", 0xFF30B0FF}, {"Contacts", 0xFFFF9F0A}, {"Browser", 0xFF5E7BFF},
    {"Camera", 0xFFFF453A}, {"Gallery", 0xFFBF5AF2}, {"Files", 0xFF64D2FF}, {"Calendar", 0xFFFF6482},
    {"Mail", 0xFF0A84FF}, {"Music", 0xFFFF375F}, {"Calc", 0xFF8E8E93}, {"Notes", 0xFFFFD60A},
    {"Maps", 0xFF30D158}, {"Settings", 0xFF636366}, {"Store", 0xFF5856D6}, {"Assistant", 0xFF00C7BE},
};
#define NAPPS ((int)(sizeof apps / sizeof *apps))

static void num2(char *o, int v) { o[0] = (char)('0' + v / 10 % 10); o[1] = (char)('0' + v % 10); }

static void background(fb_t *f)
{
    for (int y = 0; y < f->h; y++) {
        u32 t = (u32)(y * 255 / f->h);
        u32 r = 14 + (60 - 14) * t / 255, g = 16 + (28 - 16) * t / 255, b = 40 + (110 - 40) * t / 255;
        for (int x = 0; x < f->w; x++) f->px[y * f->w + x] = ARGB(255, r, g, b);
    }
}
static void status_bar(fb_t *f, int hh, int mm, int bat, int mult)
{
    char t[6] = "00:00"; num2(t, hh); num2(t + 3, mm);
    int bh = D(24);
    fb_text(f, D(16), (bh - 16 * mult) / 2, t, mult, ARGB(255, 255, 255, 255));
    int bw = D(26), bx = f->w - D(16) - bw, by = D(8), bhh = D(11);
    fb_rrect(f, bx, by, bw, bhh, D(3), ARGB(255, 255, 255, 255));
    fb_rrect(f, bx + D(1), by + D(1), (bw - D(2)) * bat / 100, bhh - D(2), D(2), ARGB(255, 52, 199, 89));
}
static void icon(fb_t *f, int x, int y, int size, const app_t *a, int mult, int label)
{
    fb_rrect(f, x, y, size, size, size * 28 / 100, a->color);
    char l[2] = { a->name[0], 0 };
    fb_text(f, x + (size - 8 * mult * 2) / 2, y + (size - 16 * mult * 2) / 2, l, mult * 2, ARGB(255, 255, 255, 255));
    if (label) {
        char buf[10]; int n = 0;
        while (a->name[n] && n < 8) { buf[n] = a->name[n]; n++; }
        buf[n] = 0;
        fb_text(f, x + (size - fb_text_w(buf, mult)) / 2, y + size + D(6), buf, mult, ARGB(255, 255, 255, 255));
    }
}
static void clock_block(fb_t *f, int cx, int y, int hh, int mm, int mult)
{
    char t[6] = "00:00"; num2(t, hh); num2(t + 3, mm);
    fb_text(f, cx - fb_text_w(t, mult * 3) / 2, y, t, mult * 3, ARGB(255, 255, 255, 255));
    const char *d = "jeu. 19 novembre";
    fb_text(f, cx - fb_text_w(d, mult) / 2, y + 16 * mult * 3 + D(4), d, mult, ARGB(200, 255, 255, 255));
}

void launcher_draw(fb_t *f, int dpi, int hh, int mm, int bat)
{
    dpi_ = dpi;
    int mult = (dpi * 2 + 160) / 320; if (mult < 1) mult = 1;
    form_t ff = ui_form_factor(f->w, f->h, dpi);
    background(f);
    status_bar(f, hh, mm, bat, mult);
    int isz = D(56);

    if (ff == FF_PHONE) {
        clock_block(f, f->w / 2, D(56), hh, mm, mult);
        int cols = 4, pitch = f->w / cols, top = D(190);
        for (int i = 0; i < NAPPS - 4 && i < 16; i++)
            icon(f, (i % cols) * pitch + (pitch - isz) / 2, top + (i / cols) * D(96), isz, &apps[i], mult, 1);
        int dh = D(84), dm = D(12);
        fb_rrect(f, dm, f->h - dh - D(28), f->w - 2 * dm, dh, D(28), ARGB(70, 255, 255, 255));
        int dock[4] = {0, 1, 3, 4};
        for (int i = 0; i < 4; i++)
            icon(f, (i * (f->w - 2 * dm)) / 4 + dm + ((f->w - 2 * dm) / 4 - isz) / 2, f->h - dh - D(28) + (dh - isz) / 2, isz, &apps[dock[i]], mult, 0);
        fb_rrect(f, (f->w - D(110)) / 2, f->h - D(14), D(110), D(5), D(3), ARGB(220, 255, 255, 255));
    } else {
        /* tablette : colonne de widgets a gauche, grille 6 colonnes, dock centre */
        int lw = D(300);
        fb_rrect(f, D(24), D(48), lw, D(200), D(24), ARGB(60, 255, 255, 255));
        clock_block(f, D(24) + lw / 2, D(72), hh, mm, mult);
        fb_rrect(f, D(24), D(264), lw, D(110), D(24), ARGB(60, 255, 255, 255));
        fb_text(f, D(44), D(280), "Assistant", mult, ARGB(255, 255, 255, 255));
        fb_text(f, D(44), D(280) + 16 * mult + D(6), "Que puis-je faire ?", mult, ARGB(190, 255, 255, 255));
        fb_rrect(f, D(24), D(390), lw, D(80), D(24), ARGB(60, 255, 255, 255));
        fb_text(f, D(44), D(406), "Batterie", mult, ARGB(255, 255, 255, 255));
        fb_rrect(f, D(44), D(406) + 16 * mult + D(8), lw - D(40), D(10), D(5), ARGB(70, 255, 255, 255));
        fb_rrect(f, D(44), D(406) + 16 * mult + D(8), (lw - D(40)) * bat / 100, D(10), D(5), ARGB(255, 52, 199, 89));
        int gx = D(24) + lw + D(40), cols = 6, pitch = (f->w - gx - D(24)) / cols;
        for (int i = 0; i < NAPPS - 4 && i < 12; i++)
            icon(f, gx + (i % cols) * pitch + (pitch - isz) / 2, D(60) + (i / cols) * D(110), isz, &apps[i], mult, 1);
        int dw = D(520), dh = D(76);
        fb_rrect(f, (f->w - dw) / 2, f->h - dh - D(14), dw, dh, D(26), ARGB(70, 255, 255, 255));
        for (int i = 0; i < 6; i++)
            icon(f, (f->w - dw) / 2 + D(20) + i * D(82), f->h - dh - D(14) + (dh - isz) / 2, isz, &apps[i == 5 ? 13 : i], mult, 0);
    }
}
