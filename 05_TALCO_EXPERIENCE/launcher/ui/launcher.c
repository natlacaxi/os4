/* TALCO Glass : ecran d'accueil, calculatrice et notes. Style « verre depoli » (flou, transparence, reflets de bord),
 * dessine en logiciel. Identite propre a TALCO (icones et fond d'ecran originaux, police Inter). */
#include <stdlib.h>
#include <string.h>
#include "launcher.h"

static int dpi_;
void bg_ready_reset(void);
static int D(int dp) { return dp * dpi_ / 160; }                      /* dp -> pixels */

form_t ui_form_factor(int w, int h, int dpi)
{
    int sw = (w < h ? w : h) * 160 / dpi;
    return sw >= 600 ? FF_TABLET : FF_PHONE;
}

typedef struct { const char *name; u32 top, bot; } app_t;
static const app_t apps[] = {
    {"Téléphone", 0xFF6CE07A, 0xFF1FAE4B}, {"Messages", 0xFF68D6FF, 0xFF1E8CFF}, {"Contacts", 0xFFFFC26A, 0xFFFF8A1E},
    {"Navigateur", 0xFF7B9BFF, 0xFF4056E6}, {"Appareil", 0xFFA4A8B4, 0xFF4E505C}, {"Galerie", 0xFFD58CFF, 0xFF8E45DC},
    {"Fichiers", 0xFF74CCFF, 0xFF2488E8}, {"Calendrier", 0xFFFFFFFF, 0xFFE9E9F0}, {"Mail", 0xFF58B2FF, 0xFF0C6CF2},
    {"Musique", 0xFFFF6B8B, 0xFFE81F55}, {"Calc", 0xFF55565E, 0xFF26272D}, {"Notes", 0xFFFFE45E, 0xFFFFB800},
    {"Plans", 0xFF72E39A, 0xFF1FA85F}, {"Réglages", 0xFFB4B6C0, 0xFF6B6D78}, {"Boutique", 0xFF8C84FF, 0xFF5238DE},
    {"Assistant", 0xFF47E6D4, 0xFF00A4C0},
};
#define NAPPS ((int)(sizeof apps / sizeof *apps))

/* zones tactiles enregistrees pendant le dessin */
static int hit_n, hit_x[40], hit_y[40], hit_s[40], hit_i[40];

/* ---------------- date ---------------- */
static char date_str[24] = "jeu. 19 novembre", cal_wd[8] = "JEU.";
static int cal_day = 19;
void launcher_set_date(int wday, int mday, int mon)
{
    static const char *wd[] = {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."};
    static const char *mo[] = {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"};
    if (wday < 0 || wday > 6 || mon < 0 || mon > 11 || mday < 1 || mday > 31) return;
    int n = 0;
    for (const char *c = wd[wday]; *c; c++) date_str[n++] = *c;
    date_str[n++] = ' ';
    if (mday >= 10) date_str[n++] = (char)('0' + mday / 10);
    date_str[n++] = (char)('0' + mday % 10); date_str[n++] = ' ';
    for (const char *c = mo[mon]; *c && n < 22; c++) date_str[n++] = *c;
    date_str[n] = 0;
    for (int i = 0; i < 5; i++) cal_wd[i] = wd[wday][i] >= 'a' && wd[wday][i] <= 'z' ? (char)(wd[wday][i] - 32) : wd[wday][i];
    cal_day = mday;
}
static void num2(char *o, int v) { o[0] = (char)('0' + v / 10 % 10); o[1] = (char)('0' + v % 10); }

/* texte centre autour de (cx, cy) */
static void ctext(fb_t *f, int cx, int cy, const char *s, int size, u32 c)
{
    fb_text(f, cx - fb_text_w(s, size) / 2, cy - size * 435 / 1000, s, size, c);
}
#define WHITE ARGB(255, 255, 255, 255)

/* ---------------- fond d'ecran (mis en cache) ---------------- */
static u32 *wall; static int wall_w, wall_h;
static void wallpaper(fb_t *f)
{
    size_t sz = (size_t)f->w * f->h * 4;
    if (wall && wall_w == f->w && wall_h == f->h) { memcpy(f->px, wall, sz); return; }
    for (int y = 0; y < f->h; y++) {
        u32 t = (u32)(y * 255 / f->h);
        u32 r = 12 + (40 - 12) * t / 255, g = 12 + (20 - 12) * t / 255, b = 38 + (86 - 38) * t / 255;
        for (int x = 0; x < f->w; x++) f->px[y * f->w + x] = ARGB(255, r, g, b);
    }
    int big = f->w > f->h ? f->w : f->h;
    fb_blob(f, f->w * 20 / 100, f->h * 16 / 100, big * 62 / 100, 0x3C78FF, 190);
    fb_blob(f, f->w * 86 / 100, f->h * 44 / 100, big * 56 / 100, 0xE8469E, 165);
    fb_blob(f, f->w * 16 / 100, f->h * 80 / 100, big * 58 / 100, 0xFF8A3C, 125);
    fb_blob(f, f->w * 78 / 100, f->h * 94 / 100, big * 50 / 100, 0x3CD8E6, 105);
    free(wall); wall = malloc(sz);
    if (wall) { memcpy(wall, f->px, sz); wall_w = f->w; wall_h = f->h; }
}

/* ---------------- icones ---------------- */
static void glyph(fb_t *f, int idx, int x, int y, int s)
{
    float S = (float)s;
#define PX(v) ((float)x + (v) * S)
#define PY(v) ((float)y + (v) * S)
#define RC(a, b, c, d, r, col) fb_rrect(f, (int)PX(a), (int)PY(b), (int)((c) * S), (int)((d) * S), (int)((r) * S), col)
    u32 W = WHITE, W2 = ARGB(235, 255, 255, 255), dark = ARGB(255, 60, 62, 72), top = apps[idx].top | 0xFF000000u, bot = apps[idx].bot | 0xFF000000u;
    switch (idx) {
    case 0: fb_ring_q(f, PX(.64f), PY(.36f), .30f * S, .15f * S, 4, W); fb_disc(f, PX(.345f), PY(.36f), .105f * S, W); fb_disc(f, PX(.64f), PY(.655f), .105f * S, W); break;
    case 1: RC(.20f, .24f, .60f, .42f, .16f, W); fb_tri(f, PX(.34f), PY(.62f), PX(.29f), PY(.79f), PX(.52f), PY(.64f), W); break;
    case 2: fb_disc(f, PX(.5f), PY(.37f), .14f * S, W); RC(.26f, .56f, .48f, .26f, .21f, W); break;
    case 3: fb_ring(f, PX(.5f), PY(.5f), .26f * S, .07f * S, W);
            fb_line(f, PX(.37f), PY(.63f), PX(.63f), PY(.37f), .10f * S, W); fb_disc(f, PX(.5f), PY(.5f), .05f * S, bot); break;
    case 4: RC(.19f, .34f, .62f, .40f, .09f, W); RC(.35f, .27f, .22f, .11f, .035f, W);
            fb_ring(f, PX(.5f), PY(.54f), .105f * S, .06f * S, dark); fb_disc(f, PX(.5f), PY(.54f), .045f * S, dark); break;
    case 5: RC(.22f, .26f, .56f, .48f, .07f, W); fb_disc(f, PX(.38f), PY(.42f), .06f * S, top);
            fb_tri(f, PX(.24f), PY(.72f), PX(.44f), PY(.48f), PX(.62f), PY(.72f), bot); fb_tri(f, PX(.46f), PY(.72f), PX(.62f), PY(.54f), PX(.76f), PY(.72f), top); break;
    case 6: RC(.22f, .27f, .26f, .12f, .045f, W); RC(.20f, .34f, .60f, .42f, .07f, W); break;
    case 7: { char dn[4] = { 0 }; int n = 0; if (cal_day >= 10) dn[n++] = (char)('0' + cal_day / 10); dn[n++] = (char)('0' + cal_day % 10);
            fb_text(f, x + (s - fb_text_w(cal_wd, s * 15 / 100)) / 2, y + s * 11 / 100, cal_wd, s * 15 / 100, ARGB(255, 255, 59, 48));
            ctext(f, x + s / 2, y + s * 62 / 100, dn, s * 46 / 100, ARGB(255, 28, 28, 32)); break; }
    case 8: RC(.20f, .32f, .60f, .40f, .06f, W); fb_line(f, PX(.23f), PY(.37f), PX(.5f), PY(.56f), .05f * S, bot); fb_line(f, PX(.77f), PY(.37f), PX(.5f), PY(.56f), .05f * S, bot); break;
    case 9: fb_disc(f, PX(.40f), PY(.68f), .10f * S, W); fb_line(f, PX(.49f), PY(.68f), PX(.49f), PY(.30f), .07f * S, W); fb_line(f, PX(.49f), PY(.30f), PX(.70f), PY(.39f), .09f * S, W); break;
    case 10: RC(.25f, .25f, .22f, .22f, .06f, ARGB(255, 255, 159, 10)); RC(.53f, .25f, .22f, .22f, .06f, W2);
             RC(.25f, .53f, .22f, .22f, .06f, W2); RC(.53f, .53f, .22f, .22f, .06f, W2); break;
    case 11: RC(.24f, .22f, .52f, .56f, .07f, W); fb_line(f, PX(.32f), PY(.40f), PX(.68f), PY(.40f), .04f * S, bot);
             fb_line(f, PX(.32f), PY(.52f), PX(.68f), PY(.52f), .04f * S, bot); fb_line(f, PX(.32f), PY(.64f), PX(.55f), PY(.64f), .04f * S, bot); break;
    case 12: fb_disc(f, PX(.5f), PY(.42f), .18f * S, W); fb_tri(f, PX(.34f), PY(.53f), PX(.66f), PY(.53f), PX(.5f), PY(.81f), W); fb_disc(f, PX(.5f), PY(.42f), .075f * S, bot); break;
    case 13: { static const float c8[8][2] = {{1,0},{.707f,.707f},{0,1},{-.707f,.707f},{-1,0},{-.707f,-.707f},{0,-1},{.707f,-.707f}};
             fb_ring(f, PX(.5f), PY(.5f), .19f * S, .10f * S, W);
             for (int i = 0; i < 8; i++) { fb_line(f, PX(.5f + .25f * c8[i][0]), PY(.5f + .25f * c8[i][1]), PX(.5f + .33f * c8[i][0]), PY(.5f + .33f * c8[i][1]), .10f * S, W); }
             break; }
    case 14: RC(.26f, .40f, .48f, .38f, .07f, W); fb_ring_q(f, PX(.5f), PY(.40f), .13f * S, .06f * S, 3, W); break;
    case 15: fb_line(f, PX(.5f), PY(.22f), PX(.5f), PY(.78f), .07f * S, W); fb_line(f, PX(.22f), PY(.5f), PX(.78f), PY(.5f), .07f * S, W);
             fb_line(f, PX(.33f), PY(.33f), PX(.67f), PY(.67f), .05f * S, W2); fb_line(f, PX(.67f), PY(.33f), PX(.33f), PY(.67f), .05f * S, W2); fb_disc(f, PX(.5f), PY(.5f), .09f * S, W); break;
    }
#undef PX
#undef PY
#undef RC
}
static void app_icon(fb_t *f, int idx, int x, int y, int s, int label)
{
    if (hit_n < 40) { hit_x[hit_n] = x; hit_y[hit_n] = y; hit_s[hit_n] = s; hit_i[hit_n] = idx; hit_n++; }
    int r = s * 28 / 100, th = dpi_ >= 300 ? 2 : 1;
    fb_shadow(f, x, y + D(3), s, s, r, D(7), 80);
    fb_rrect_grad(f, x, y, s, s, r, apps[idx].top | 0xFF000000u, apps[idx].bot | 0xFF000000u);
    fb_rrect_outline(f, x, y, s, s, r, th, ARGB(120, 255, 255, 255), ARGB(8, 255, 255, 255));
    glyph(f, idx, x, y, s);
    if (label) ctext(f, x + s / 2, y + s + D(13), apps[idx].name, D(12), ARGB(240, 255, 255, 255));
}

/* ---------------- barre d'etat ---------------- */
static void status_bar(fb_t *f, int hh, int mm, int bat)
{
    char t[6] = "00:00"; num2(t, hh); num2(t + 3, mm);
    fb_text(f, D(18), D(9), t, D(15), WHITE);
    int bw = D(26), bh = D(12), bx = f->w - D(20) - bw, by = D(12);
    fb_rrect_outline(f, bx, by, bw, bh, D(4), 1 + (dpi_ >= 300), ARGB(200, 255, 255, 255), ARGB(200, 255, 255, 255));
    int pad = D(2) + 1;
    fb_rrect(f, bx + pad, by + pad, (bw - 2 * pad) * bat / 100, bh - 2 * pad, D(2), ARGB(255, 90, 220, 120));
    fb_rrect(f, bx + bw + D(1), by + bh / 2 - D(2), D(2), D(4), D(1), ARGB(200, 255, 255, 255));
}
static void card(fb_t *f, int x, int y, int w, int h, int r)
{
    fb_shadow(f, x, y + D(4), w, h, r, D(10), 70);
    fb_glass(f, x, y, w, h, r, D(10), ARGB(62, 255, 255, 255));
}

static int last_mm;
static void clock_card(fb_t *f, int x, int y, int w, int h, int hh, int mm, int bat, int big)
{
    char t[6] = "00:00"; num2(t, hh); num2(t + 3, mm);
    card(f, x, y, w, h, D(28));
    if (big) {                                          /* telephone : horloge a gauche, batterie a droite */
        fb_text(f, x + D(22), y + D(8), t, D(78), WHITE);
        fb_text(f, x + D(24), y + h - D(40), date_str, D(17), ARGB(225, 255, 255, 255));
        char b[8] = { 0 }; int n = 0; if (bat >= 100) b[n++] = '1'; if (bat >= 10) b[n++] = (char)('0' + bat / 10 % 10); b[n++] = (char)('0' + bat % 10); b[n++] = ' '; b[n++] = '%';
        fb_text(f, x + w - D(22) - fb_text_w(b, D(24)), y + D(26), b, D(24), WHITE);
        fb_text(f, x + w - D(22) - fb_text_w("Batterie", D(13)), y + D(56), "Batterie", D(13), ARGB(190, 255, 255, 255));
    } else {
        ctext(f, x + w / 2, y + h * 40 / 100, t, D(64), WHITE);
        ctext(f, x + w / 2, y + h * 78 / 100, date_str, D(16), ARGB(225, 255, 255, 255));
    }
}

void launcher_draw(fb_t *f, int dpi, int hh, int mm, int bat)
{
    dpi_ = dpi; hit_n = 0; last_mm = mm; bg_ready_reset();
    form_t ff = ui_form_factor(f->w, f->h, dpi);
    wallpaper(f);
    status_bar(f, hh, mm, bat);
    if (ff == FF_PHONE) {
        int m = D(16);
        clock_card(f, m, D(46), f->w - 2 * m, D(148), hh, mm, bat, 1);
        static const int grid[12] = {2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        int pitch = f->w / 4, isz = D(60), top = D(224);
        for (int i = 0; i < 12; i++) app_icon(f, grid[i], (i % 4) * pitch + (pitch - isz) / 2, top + (i / 4) * D(100), isz, 1);
        int dh = D(86), dy = f->h - dh - D(30), sh = D(46), sy = dy - sh - D(18), sx = D(36);
        for (int i = 0; i < 3; i++) fb_disc(f, (float)f->w / 2 + (float)(i - 1) * (float)D(14), (float)(sy - D(14)), (float)D(3), ARGB(i == 0 ? 235 : 110, 255, 255, 255));
        card(f, sx, sy, f->w - 2 * sx, sh, sh / 2);
        fb_ring(f, (float)(sx + D(24)), (float)(sy + sh / 2 - D(1)), (float)D(6), (float)D(2), ARGB(210, 255, 255, 255));
        fb_line(f, (float)(sx + D(28)), (float)(sy + sh / 2 + D(3)), (float)(sx + D(33)), (float)(sy + sh / 2 + D(8)), (float)D(2), ARGB(210, 255, 255, 255));
        ctext(f, f->w / 2, sy + sh / 2, "Rechercher", D(16), ARGB(205, 255, 255, 255));
        card(f, D(16), dy, f->w - D(32), dh, D(32));
        static const int dock[4] = {0, 1, 3, 4};
        int ds = D(58), dw = f->w - D(32);
        for (int i = 0; i < 4; i++) app_icon(f, dock[i], D(16) + i * dw / 4 + (dw / 4 - ds) / 2, dy + (dh - ds) / 2, ds, 0);
        fb_rrect(f, (f->w - D(110)) / 2, f->h - D(14), D(110), D(5), D(3), ARGB(220, 255, 255, 255));
    } else {
        int lx = D(24), lw = D(300);
        clock_card(f, lx, D(46), lw, D(176), hh, mm, bat, 0);
        card(f, lx, D(238), lw, D(104), D(28));
        app_icon(f, 15, lx + D(18), D(238) + D(24), D(56), 0);
        fb_text(f, lx + D(86), D(238) + D(26), "Assistant", D(18), WHITE);
        fb_text(f, lx + D(86), D(238) + D(52), "Que puis-je faire ?", D(14), ARGB(200, 255, 255, 255));
        card(f, lx, D(358), lw, D(92), D(28));
        fb_text(f, lx + D(22), D(358) + D(16), "Batterie", D(16), WHITE);
        int bwid = lw - D(44), by = D(358) + D(54);
        fb_rrect(f, lx + D(22), by, bwid, D(12), D(6), ARGB(70, 255, 255, 255));
        fb_rrect_grad(f, lx + D(22), by, bwid * bat / 100, D(12), D(6), ARGB(255, 120, 235, 150), ARGB(255, 40, 190, 90));
        static const int grid[10] = {2, 5, 6, 7, 9, 10, 11, 12, 14, 15};
        int gx = lx + lw + D(36), pitch = (f->w - gx - D(24)) / 6, isz = D(60);
        for (int i = 0; i < 10; i++) app_icon(f, grid[i], gx + (i % 6) * pitch + (pitch - isz) / 2, D(62) + (i / 6) * D(112), isz, 1);
        int dw = D(560), dh = D(84), dy = f->h - dh - D(18), ds = D(58);
        card(f, (f->w - dw) / 2, dy, dw, dh, D(32));
        static const int dock[6] = {0, 1, 3, 4, 8, 13};
        for (int i = 0; i < 6; i++) app_icon(f, dock[i], (f->w - dw) / 2 + i * dw / 6 + (dw / 6 - ds) / 2, dy + (dh - ds) / 2, ds, 0);
    }
}

int launcher_hit(int x, int y)
{
    for (int k = hit_n - 1; k >= 0; k--)
        if (x >= hit_x[k] && x < hit_x[k] + hit_s[k] && y >= hit_y[k] && y < hit_y[k] + hit_s[k]) return hit_i[k];
    return -1;
}
const char *launcher_app_name(int i) { return (i >= 0 && i < NAPPS) ? apps[i].name : ""; }
int launcher_hit_count(void) { return hit_n; }
int launcher_hit_center(int k, int *x, int *y, int *app)
{
    if (k < 0 || k >= hit_n) return 0;
    *x = hit_x[k] + hit_s[k] / 2; *y = hit_y[k] + hit_s[k] / 2; *app = hit_i[k];
    return 1;
}

/* ---------------- Calculatrice ---------------- */
static double c_acc; static char c_op, c_in[20] = "0", c_disp[24] = "0"; static int c_fresh = 1, c_err;
static int c_x[17], c_y[17], c_w[17], c_h[17]; static char c_ch[17]; static int c_n;
static int rb_x, rb_y, rb_w, rb_h;

static double c_parse(const char *s)
{
    double v = 0, f = 0.1; int dot = 0;
    for (; *s; s++) {
        if (*s == '.') dot = 1;
        else if (!dot) v = v * 10 + (*s - '0');
        else { v += (*s - '0') * f; f /= 10; }
    }
    return v;
}
static void c_fmt(double v, char *o)
{
    int n = 0;
    if (v < 0) { o[n++] = '-'; v = -v; }
    if (v >= 1e12) { o[0] = 'E'; o[1] = 'r'; o[2] = 'r'; o[3] = 0; c_err = 1; return; }
    long long ip = (long long)v, sc = (long long)((v - (double)ip) * 1000000.0 + 0.5);
    if (sc >= 1000000) { ip++; sc -= 1000000; }
    char t[24]; int k = 0;
    do { t[k++] = (char)('0' + ip % 10); ip /= 10; } while (ip);
    while (k) o[n++] = t[--k];
    if (sc) {
        o[n++] = '.';
        char fr[7]; for (int i = 5; i >= 0; i--) { fr[i] = (char)('0' + sc % 10); sc /= 10; }
        int e = 6; while (e > 1 && fr[e - 1] == '0') e--;
        for (int i = 0; i < e; i++) o[n++] = fr[i];
    }
    o[n] = 0;
}
static void c_show_err(void)
{
    const char *m = "Erreur"; int i = 0;
    for (; m[i]; i++) c_disp[i] = m[i];
    c_disp[i] = 0; c_acc = 0; c_op = 0; c_in[0] = '0'; c_in[1] = 0; c_fresh = 1;
}
static void c_apply(void)
{
    double b = c_parse(c_in);
    if (c_op == '+') c_acc += b; else if (c_op == '-') c_acc -= b; else if (c_op == '*') c_acc *= b;
    else if (c_op == '/') { if (b == 0) { c_show_err(); c_err = 1; return; } c_acc /= b; }
}
void calc_press(char c)
{
    if (c_err) { c_err = 0; c_acc = 0; c_op = 0; c_in[0] = '0'; c_in[1] = 0; c_fresh = 1; c_disp[0] = '0'; c_disp[1] = 0; }
    int len = 0; while (c_in[len]) len++;
    if (c == 'C') { c_acc = 0; c_op = 0; c_in[0] = '0'; c_in[1] = 0; c_fresh = 1; c_disp[0] = '0'; c_disp[1] = 0; return; }
    if ((c >= '0' && c <= '9') || c == '.') {
        if (c_fresh) { c_in[0] = 0; len = 0; }
        if (c == '.') {
            for (int i = 0; i < len; i++) if (c_in[i] == '.') return;
            if (!len) { c_in[0] = '0'; len = 1; }
        } else if (len == 1 && c_in[0] == '0') len = 0;
        if (len < 12) { c_in[len] = c; c_in[len + 1] = 0; }
        c_fresh = 0;
        int i = 0; for (; c_in[i]; i++) c_disp[i] = c_in[i]; c_disp[i] = 0;
        return;
    }
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=') {
        if (c_op && !c_fresh) { c_apply(); if (c_err) return; }
        else if (!c_op) c_acc = c_parse(c_in);
        c_fmt(c_acc, c_disp);
        if (c_err) { c_show_err(); return; }
        if (c == '=') { c_op = 0; int i = 0; for (; c_disp[i]; i++) c_in[i] = c_disp[i]; c_in[i] = 0; }
        else c_op = c;
        c_fresh = 1;
    }
}
const char *calc_display(void) { return c_disp; }
static int name_is(int idx, const char *w) { const char *a = apps[idx].name; while (*a && *a == *w) { a++; w++; } return *a == *w; }

/* ---------------- fenetres d'applications ---------------- */
static u32 *bgc; static int bg_w, bg_h, bg_dpi, bg_min, bg_ready;
void bg_ready_reset(void) { bg_ready = 0; bg_w = 0; }

static void retour(fb_t *f, int x, int y, int w, int h)
{
    rb_x = x; rb_y = y; rb_w = w; rb_h = h;
    fb_rrect_grad(f, x, y, w, h, h / 2, ARGB(255, 120, 138, 255), ARGB(255, 70, 86, 240));
    fb_rrect_outline(f, x, y, w, h, h / 2, 1 + (dpi_ >= 300), ARGB(110, 255, 255, 255), ARGB(10, 255, 255, 255));
    ctext(f, x + w / 2, y + h / 2, "Retour", D(17), WHITE);
}
static void win_frame(fb_t *f, int idx, int *ox, int *oy, int *ow, int *oh, int hpct_phone, int hpct_tab, int wpct_tab, const char *title)
{
    int phone = ui_form_factor(f->w, f->h, dpi_) == FF_PHONE;
    int w = phone ? f->w - D(32) : f->w * wpct_tab / 100, h = f->h * (phone ? hpct_phone : hpct_tab) / 100;
    int x = (f->w - w) / 2, y = (f->h - h) / 2;
    fb_shadow(f, x, y + D(6), w, h, D(30), D(14), 110);
    fb_glass(f, x, y, w, h, D(30), 0, ARGB(190, 24, 26, 42));
    int s = D(44), r = s * 28 / 100;
    fb_rrect_grad(f, x + D(20), y + D(16), s, s, r, apps[idx].top | 0xFF000000u, apps[idx].bot | 0xFF000000u);
    glyph(f, idx, x + D(20), y + D(16), s);
    fb_text(f, x + D(76), y + D(16) + (s - D(26)) / 2, title, D(26), WHITE);
    *ox = x; *oy = y; *ow = w; *oh = h;
}

static void calc_window(fb_t *f, int idx)
{
    int x, y, w, h; win_frame(f, idx, &x, &y, &w, &h, 80, 86, 56, "Calculatrice");
    fb_rrect(f, x + D(20), y + D(72), w - D(40), D(64), D(18), ARGB(150, 0, 0, 0));
    int ds = D(40); if (fb_text_w(c_disp, ds) > w - D(72)) ds = D(26);
    fb_text(f, x + w - D(34) - fb_text_w(c_disp, ds), y + D(72) + (D(64) - ds) / 2 - D(1), c_disp, ds, WHITE);
    int ky = y + D(150), by = y + h - D(66), gap = D(8), avail = by - D(12) - ky;
    int kh = (avail - 4 * gap) / 5, kw = (w - D(40) - 3 * gap) / 4;
    const char *keys = "789/456*123-0.=+";
    c_n = 0;
    for (int i = 0; i < 16; i++) {
        int kx = x + D(20) + (i % 4) * (kw + gap), kyy = ky + (i / 4) * (kh + gap);
        char c = keys[i]; int op = (c == '/' || c == '*' || c == '-' || c == '+' || c == '=');
        if (op) fb_rrect_grad(f, kx, kyy, kw, kh, kh / 2 < D(20) ? kh / 2 : D(20), ARGB(255, 255, 178, 56), ARGB(255, 255, 140, 0));
        else { fb_rrect(f, kx, kyy, kw, kh, kh / 2 < D(20) ? kh / 2 : D(20), ARGB(72, 255, 255, 255)); fb_rrect_outline(f, kx, kyy, kw, kh, kh / 2 < D(20) ? kh / 2 : D(20), 1, ARGB(70, 255, 255, 255), ARGB(10, 255, 255, 255)); }
        const char lab[2] = { c, 0 }; const char *l = c == '*' ? "×" : c == '/' ? "÷" : c == '-' ? "−" : lab;
        ctext(f, kx + kw / 2, kyy + kh / 2, l, D(26), WHITE);
        c_x[c_n] = kx; c_y[c_n] = kyy; c_w[c_n] = kw; c_h[c_n] = kh; c_ch[c_n++] = c;
    }
    int cy = ky + 4 * (kh + gap);
    fb_rrect_grad(f, x + D(20), cy, w - D(40), kh, kh / 2 < D(20) ? kh / 2 : D(20), ARGB(255, 255, 110, 100), ARGB(255, 235, 60, 55));
    ctext(f, x + w / 2, cy + kh / 2, "C", D(26), WHITE);
    c_x[c_n] = x + D(20); c_y[c_n] = cy; c_w[c_n] = w - D(40); c_h[c_n] = kh; c_ch[c_n++] = 'C';
    retour(f, x + D(20), by, w - D(40), D(46));
}

/* ---------------- Notes + clavier AZERTY ---------------- */
static char n_buf[400]; static int n_len;
static struct { int x, y, w, h; char ch; } nk[60]; static int nk_n;
void notes_press(char c)
{
    if (c == '<') { if (n_len) n_buf[--n_len] = 0; return; }
    if (n_len < (int)sizeof n_buf - 1) { n_buf[n_len++] = c; n_buf[n_len] = 0; }
}
const char *notes_text(void) { return n_buf; }
static void nkey(fb_t *f, int x, int y, int w, int h, char c, const char *label, int size, int kind)
{
    int r = h / 3;
    if (kind == 1) fb_rrect_grad(f, x, y, w, h, r, ARGB(255, 255, 178, 56), ARGB(255, 255, 140, 0));
    else if (kind == 2) fb_rrect_grad(f, x, y, w, h, r, ARGB(255, 255, 110, 100), ARGB(255, 235, 60, 55));
    else { fb_rrect(f, x, y, w, h, r, ARGB(72, 255, 255, 255)); fb_rrect_outline(f, x, y, w, h, r, 1, ARGB(70, 255, 255, 255), ARGB(10, 255, 255, 255)); }
    const char l[2] = { c, 0 };
    if (c == '<') {                                    /* touche effacer : fleche vers la gauche dessinee en vecteur */
        float cx = (float)x + (float)w / 2, cy = (float)y + (float)h / 2, u = (float)h * 0.17f;
        fb_tri(f, cx - 2.2f * u, cy, cx - 0.3f * u, cy - 1.3f * u, cx - 0.3f * u, cy + 1.3f * u, WHITE);
        fb_rrect(f, (int)(cx - 0.4f * u), (int)(cy - 1.3f * u), (int)(2.4f * u), (int)(2.6f * u), (int)(0.4f * u), WHITE);
        fb_line(f, cx + 0.0f * u, cy - 0.5f * u, cx + 1.4f * u, cy + 0.5f * u, 0.4f * u, ARGB(255, 235, 60, 55));
        fb_line(f, cx + 1.4f * u, cy - 0.5f * u, cx + 0.0f * u, cy + 0.5f * u, 0.4f * u, ARGB(255, 235, 60, 55));
    } else ctext(f, x + w / 2, y + h / 2, label ? label : l, size, WHITE);
    if (nk_n < 60) { nk[nk_n].x = x; nk[nk_n].y = y; nk[nk_n].w = w; nk[nk_n].h = h; nk[nk_n].ch = c; nk_n++; }
}
static void notes_window(fb_t *f, int idx)
{
    int x, y, w, h; win_frame(f, idx, &x, &y, &w, &h, 85, 90, 70, "Notes");
    int by = y + h - D(66), kh = D(40), gap = D(6), ky = by - D(12) - (5 * kh + 4 * gap);
    int tx = x + D(20), ty = y + D(72), tw = w - D(40), th = ky - D(12) - ty, sz = D(18);
    fb_rrect(f, tx, ty, tw, th, D(18), ARGB(150, 0, 0, 0));
    int cx = tx + D(14), cy = ty + D(12), lh = sz + D(6), maxx = tx + tw - D(14), bot = ty + th - D(8);
    if (!n_len) fb_text(f, cx, cy, "Tapez votre note…", sz, ARGB(110, 255, 255, 255));
    for (int i = 0; i < n_len; i++) {
        char c = n_buf[i];
        if (c == '\n') { cx = tx + D(14); cy += lh; continue; }
        char s1[2] = { c, 0 }; int cw = fb_text_w(s1, sz);
        if (cx + cw > maxx) { cx = tx + D(14); cy += lh; }
        if (cy + sz > bot) break;
        fb_text(f, cx, cy, s1, sz, WHITE);
        cx += cw;
    }
    if (cy + sz <= bot) fb_rrect(f, cx + 1, cy + D(2), D(2), sz - D(2), D(1), ARGB(230, 120, 138, 255));
    nk_n = 0;
    const char *rows[4] = { "1234567890", "azertyuiop", "qsdfghjklm", "wxcvbn,.'<" };
    int kw = (w - D(40) - 9 * gap) / 10;
    for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) {
        char ch = rows[r][c];
        nkey(f, x + D(20) + c * (kw + gap), ky + r * (kh + gap), kw, kh, ch, 0, D(20), ch == '<' ? 2 : 0);
    }
    int r5 = ky + 4 * (kh + gap);
    nkey(f, x + D(20), r5, 7 * kw + 6 * gap, kh, ' ', "espace", D(16), 0);
    nkey(f, x + D(20) + 7 * (kw + gap), r5, 3 * kw + 2 * gap, kh, '\n', "Entrée", D(16), 1);
    retour(f, x + D(20), by, w - D(40), D(46));
}

static void generic_window(fb_t *f, int idx)
{
    int x, y, w, h; win_frame(f, idx, &x, &y, &w, &h, 45, 65, 55, apps[idx].name);
    char line[64] = "Module 07_APPLICATIONS/"; int n = 23;
    for (const char *c = apps[idx].name; *c && n < 62; c++, n++) line[n] = (*c >= 'A' && *c <= 'Z') ? (char)(*c + 32) : *c;
    line[n] = 0;
    fb_text(f, x + D(24), y + D(92), line, D(14), ARGB(200, 255, 255, 255));
    fb_text(f, x + D(24), y + D(118), "Contenu à venir.", D(16), ARGB(170, 255, 255, 255));
    retour(f, x + D(20), y + h - D(66), w - D(40), D(46));
}

void launcher_app_window(fb_t *f, int dpi, int idx)
{
    if (idx < 0 || idx >= NAPPS) return;
    dpi_ = dpi;
    if (bg_ready) bg_ready = 0;                         /* le fond flou est deja la (copie du cache) */
    else {
        fb_blur_rect(f, 0, 0, f->w, f->h, D(8));
        fb_fill(f, 0, 0, f->w, f->h, ARGB(120, 4, 4, 16));
        size_t sz = (size_t)f->w * f->h * 4;
        free(bgc); bgc = malloc(sz);
        if (bgc) { memcpy(bgc, f->px, sz); bg_w = f->w; bg_h = f->h; bg_dpi = dpi; bg_min = last_mm; }
    }
    if (name_is(idx, "Calc")) calc_window(f, idx);
    else if (name_is(idx, "Notes")) notes_window(f, idx);
    else generic_window(f, idx);
}

/* 1 si l'app a sa propre fenetre dans TALCO (Calc, Notes), 0 si elle doit etre ouverte par le systeme hote (Android) */
int launcher_app_internal(int idx) { return idx >= 0 && idx < NAPPS && (name_is(idx, "Calc") || name_is(idx, "Notes")); }

/* un seul appel pour afficher l'accueil ou une application, avec cache du fond flou */
void launcher_render(fb_t *f, int dpi, int hh, int mm, int bat, int app)
{
    if (app >= 0 && bgc && bg_w == f->w && bg_h == f->h && bg_dpi == dpi && bg_min == mm) {
        memcpy(f->px, bgc, (size_t)f->w * f->h * 4); bg_ready = 1; dpi_ = dpi;
    } else launcher_draw(f, dpi, hh, mm, bat);
    if (app >= 0) launcher_app_window(f, dpi, app);
}

/* tap dans la fenetre d'une app : 1 = traite (rester ouvert), 0 = fermer */
int launcher_app_tap(int x, int y, int idx)
{
    if (idx < 0 || idx >= NAPPS || (!name_is(idx, "Calc") && !name_is(idx, "Notes"))) return 0;
    if (x >= rb_x && x < rb_x + rb_w && y >= rb_y && y < rb_y + rb_h) return 0;
    if (name_is(idx, "Notes")) {
        for (int k = 0; k < nk_n; k++)
            if (x >= nk[k].x && x < nk[k].x + nk[k].w && y >= nk[k].y && y < nk[k].y + nk[k].h) { notes_press(nk[k].ch); break; }
        return 1;
    }
    for (int k = 0; k < c_n; k++)
        if (x >= c_x[k] && x < c_x[k] + c_w[k] && y >= c_y[k] && y < c_y[k] + c_h[k]) { calc_press(c_ch[k]); break; }
    return 1;
}
int launcher_calc_key_center(char c, int *x, int *y)
{
    for (int k = 0; k < c_n; k++) if (c_ch[k] == c) { *x = c_x[k] + c_w[k] / 2; *y = c_y[k] + c_h[k] / 2; return 1; }
    return 0;
}
int launcher_notes_key_center(char c, int *x, int *y)
{
    for (int k = 0; k < nk_n; k++) if (nk[k].ch == c) { *x = nk[k].x + nk[k].w / 2; *y = nk[k].y + nk[k].h / 2; return 1; }
    return 0;
}
