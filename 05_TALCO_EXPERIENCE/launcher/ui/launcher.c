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

/* zones tactiles enregistrees pendant le dessin */
static int hit_n, hit_x[40], hit_y[40], hit_s[40], hit_i[40];

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
    if (hit_n < 40) { hit_x[hit_n] = x; hit_y[hit_n] = y; hit_s[hit_n] = size; hit_i[hit_n] = (int)(a - apps); hit_n++; }
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
static char date_str[24] = "jeu. 19 novembre";
void launcher_set_date(int wday, int mday, int mon)
{
    static const char *wd[] = {"dim.", "lun.", "mar.", "mer.", "jeu.", "ven.", "sam."};
    static const char *mo[] = {"janvier", "fevrier", "mars", "avril", "mai", "juin", "juillet", "aout", "septembre", "octobre", "novembre", "decembre"};
    if (wday < 0 || wday > 6 || mon < 0 || mon > 11 || mday < 1 || mday > 31) return;
    int n = 0;
    for (const char *c = wd[wday]; *c; c++) date_str[n++] = *c;
    date_str[n++] = ' ';
    if (mday >= 10) date_str[n++] = (char)('0' + mday / 10);
    date_str[n++] = (char)('0' + mday % 10); date_str[n++] = ' ';
    for (const char *c = mo[mon]; *c; c++) date_str[n++] = *c;
    date_str[n] = 0;
}

static void clock_block(fb_t *f, int cx, int y, int hh, int mm, int mult)
{
    char t[6] = "00:00"; num2(t, hh); num2(t + 3, mm);
    fb_text(f, cx - fb_text_w(t, mult * 3) / 2, y, t, mult * 3, ARGB(255, 255, 255, 255));
    const char *d = date_str;
    fb_text(f, cx - fb_text_w(d, mult) / 2, y + 16 * mult * 3 + D(4), d, mult, ARGB(200, 255, 255, 255));
}

void launcher_draw(fb_t *f, int dpi, int hh, int mm, int bat)
{
    dpi_ = dpi; hit_n = 0;
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
static int is_calc(int idx) { const char *a = apps[idx].name; return a[0] == 'C' && a[1] == 'a' && a[2] == 'l' && a[3] == 'c' && !a[4]; }

static void calc_window(fb_t *f, int mult, int idx)
{
    int phone = ui_form_factor(f->w, f->h, dpi_) == FF_PHONE;
    int w = phone ? f->w - D(32) : f->w * 55 / 100, h = phone ? f->h * 80 / 100 : f->h * 85 / 100;
    int x = (f->w - w) / 2, y = (f->h - h) / 2;
    fb_rrect(f, x, y, w, h, D(28), ARGB(255, 28, 30, 42));
    fb_rrect(f, x + D(20), y + D(14), D(40), D(40), D(12), apps[idx].color);
    fb_text(f, x + D(20) + (D(40) - 8 * mult) / 2, y + D(14) + (D(40) - 16 * mult) / 2, "C", mult, ARGB(255, 255, 255, 255));
    fb_text(f, x + D(72), y + D(14) + (D(40) - 16 * mult * 2) / 2, "Calc", mult * 2, ARGB(255, 255, 255, 255));
    fb_rrect(f, x + D(20), y + D(64), w - D(40), D(56), D(14), ARGB(255, 14, 15, 22));
    int dm = mult * 2, tw = fb_text_w(c_disp, dm);
    if (tw > w - D(56)) dm = mult;
    fb_text(f, x + w - D(32) - fb_text_w(c_disp, dm), y + D(64) + (D(56) - 16 * dm) / 2, c_disp, dm, ARGB(255, 255, 255, 255));
    int ky = y + D(132), by = y + h - D(64), avail = by - D(12) - ky, gap = D(8);
    int kh = (avail - 4 * gap) / 5, kw = (w - D(40) - 3 * gap) / 4;
    const char *keys = "789/456*123-0.=+";
    c_n = 0;
    for (int i = 0; i < 16; i++) {
        int kx = x + D(20) + (i % 4) * (kw + gap), kyy = ky + (i / 4) * (kh + gap);
        char c = keys[i]; int op = (c == '/' || c == '*' || c == '-' || c == '+' || c == '=');
        fb_rrect(f, kx, kyy, kw, kh, D(14), op ? ARGB(255, 255, 159, 10) : ARGB(255, 58, 60, 78));
        char l[2] = { c, 0 };
        fb_text(f, kx + (kw - 8 * mult * 2) / 2, kyy + (kh - 16 * mult * 2) / 2, l, mult * 2, ARGB(255, 255, 255, 255));
        c_x[c_n] = kx; c_y[c_n] = kyy; c_w[c_n] = kw; c_h[c_n] = kh; c_ch[c_n++] = c;
    }
    int cy = ky + 4 * (kh + gap);
    fb_rrect(f, x + D(20), cy, w - D(40), kh, D(14), ARGB(255, 255, 69, 58));
    fb_text(f, x + (w - 8 * mult * 2) / 2, cy + (kh - 16 * mult * 2) / 2, "C", mult * 2, ARGB(255, 255, 255, 255));
    c_x[c_n] = x + D(20); c_y[c_n] = cy; c_w[c_n] = w - D(40); c_h[c_n] = kh; c_ch[c_n++] = 'C';
    rb_x = x + D(20); rb_y = by; rb_w = w - D(40); rb_h = D(44);
    fb_rrect(f, rb_x, rb_y, rb_w, rb_h, D(14), ARGB(255, 91, 108, 255));
    fb_text(f, x + (w - fb_text_w("Retour", mult)) / 2, by + (D(44) - 16 * mult) / 2, "Retour", mult, ARGB(255, 255, 255, 255));
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
static int is_notes(int idx) { const char *a = apps[idx].name; return a[0] == 'N' && a[1] == 'o' && a[2] == 't' && a[3] == 'e' && a[4] == 's' && !a[5]; }

static void nkey(fb_t *f, int x, int y, int w, int h, char c, const char *label, int km, u32 col)
{
    fb_rrect(f, x, y, w, h, D(10), col);
    char l[2] = { c, 0 };
    if (!label) label = l;
    fb_text(f, x + (w - fb_text_w(label, km)) / 2, y + (h - 16 * km) / 2, label, km, ARGB(255, 255, 255, 255));
    if (nk_n < 60) { nk[nk_n].x = x; nk[nk_n].y = y; nk[nk_n].w = w; nk[nk_n].h = h; nk[nk_n].ch = c; nk_n++; }
}
static void notes_window(fb_t *f, int mult, int idx)
{
    int phone = ui_form_factor(f->w, f->h, dpi_) == FF_PHONE;
    int w = phone ? f->w - D(32) : f->w * 70 / 100, h = phone ? f->h * 85 / 100 : f->h * 90 / 100;
    int x = (f->w - w) / 2, y = (f->h - h) / 2, km = mult < 2 ? 2 : mult;
    fb_rrect(f, x, y, w, h, D(28), ARGB(255, 28, 30, 42));
    fb_rrect(f, x + D(20), y + D(14), D(40), D(40), D(12), apps[idx].color);
    fb_text(f, x + D(20) + (D(40) - 8 * mult) / 2, y + D(14) + (D(40) - 16 * mult) / 2, "N", mult, ARGB(255, 255, 255, 255));
    fb_text(f, x + D(72), y + D(14) + (D(40) - 16 * mult * 2) / 2, "Notes", mult * 2, ARGB(255, 255, 255, 255));
    int by = y + h - D(64), kh = D(40), gap = D(6), ky = by - D(12) - (5 * kh + 4 * gap);
    int tx = x + D(20), ty = y + D(64), tw = w - D(40), th = ky - D(10) - ty;
    fb_rrect(f, tx, ty, tw, th, D(14), ARGB(255, 14, 15, 22));
    int cx = tx + D(12), cy = ty + D(12), lh = 16 * mult + D(4), maxx = tx + tw - D(12), bot = ty + th - D(8);
    if (!n_len) fb_text(f, cx, cy, "Tapez votre note...", mult, ARGB(120, 255, 255, 255));
    for (int i = 0; i < n_len; i++) {
        char c = n_buf[i];
        if (c == '\n') { cx = tx + D(12); cy += lh; continue; }
        if (cx + 8 * mult > maxx) { cx = tx + D(12); cy += lh; }
        if (cy + 16 * mult > bot) break;
        char s1[2] = { c, 0 };
        fb_text(f, cx, cy, s1, mult, ARGB(255, 255, 255, 255));
        cx += 8 * mult;
    }
    if (n_len && cx + 8 * mult > maxx) { cx = tx + D(12); cy += lh; }
    if (cy + 16 * mult <= bot) fb_text(f, cx, cy, "_", mult, ARGB(200, 91, 108, 255));
    nk_n = 0;
    const char *rows[4] = { "1234567890", "azertyuiop", "qsdfghjklm", "wxcvbn,.'<" };
    int kw = (w - D(40) - 9 * gap) / 10;
    for (int r = 0; r < 4; r++) for (int c = 0; c < 10; c++) {
        char ch = rows[r][c];
        nkey(f, x + D(20) + c * (kw + gap), ky + r * (kh + gap), kw, kh, ch, 0, km, ch == '<' ? ARGB(255, 255, 69, 58) : ARGB(255, 58, 60, 78));
    }
    int r5 = ky + 4 * (kh + gap);
    nkey(f, x + D(20), r5, 7 * kw + 6 * gap, kh, ' ', "espace", mult, ARGB(255, 58, 60, 78));
    nkey(f, x + D(20) + 7 * (kw + gap), r5, 3 * kw + 2 * gap, kh, '\n', "Entree", mult, ARGB(255, 255, 159, 10));
    rb_x = x + D(20); rb_y = by; rb_w = w - D(40); rb_h = D(44);
    fb_rrect(f, rb_x, rb_y, rb_w, rb_h, D(14), ARGB(255, 91, 108, 255));
    fb_text(f, x + (w - fb_text_w("Retour", mult)) / 2, by + (D(44) - 16 * mult) / 2, "Retour", mult, ARGB(255, 255, 255, 255));
}
int launcher_notes_key_center(char c, int *x, int *y)
{
    for (int k = 0; k < nk_n; k++) if (nk[k].ch == c) { *x = nk[k].x + nk[k].w / 2; *y = nk[k].y + nk[k].h / 2; return 1; }
    return 0;
}

/* tap dans la fenetre d'une app : 1 = traite (rester ouvert), 0 = fermer */
int launcher_app_tap(int x, int y, int idx)
{
    if (idx < 0 || idx >= NAPPS || (!is_calc(idx) && !is_notes(idx))) return 0;
    if (x >= rb_x && x < rb_x + rb_w && y >= rb_y && y < rb_y + rb_h) return 0;
    if (is_notes(idx)) {
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

void launcher_app_window(fb_t *f, int dpi, int idx)
{
    if (idx < 0 || idx >= NAPPS) return;
    dpi_ = dpi;
    int mult = (dpi * 2 + 160) / 320; if (mult < 1) mult = 1;
    int phone = ui_form_factor(f->w, f->h, dpi) == FF_PHONE;
    fb_fill(f, 0, 0, f->w, f->h, ARGB(150, 0, 0, 0));
    if (is_calc(idx)) { calc_window(f, mult, idx); return; }
    if (is_notes(idx)) { notes_window(f, mult, idx); return; }
    int w = phone ? f->w - D(32) : f->w * 55 / 100, h = phone ? f->h * 45 / 100 : f->h * 65 / 100;
    int x = (f->w - w) / 2, y = (f->h - h) / 2;
    fb_rrect(f, x, y, w, h, D(28), ARGB(255, 28, 30, 42));
    fb_rrect(f, x + D(20), y + D(20), D(48), D(48), D(14), apps[idx].color);
    char l[2] = { apps[idx].name[0], 0 };
    fb_text(f, x + D(20) + (D(48) - 8 * mult) / 2, y + D(20) + (D(48) - 16 * mult) / 2, l, mult, ARGB(255, 255, 255, 255));
    fb_text(f, x + D(80), y + D(20) + (D(48) - 16 * mult * 2) / 2, apps[idx].name, mult * 2, ARGB(255, 255, 255, 255));
    char line[64] = "Module 07_APPLICATIONS/"; int n = 23;
    for (const char *c = apps[idx].name; *c && n < 62; c++, n++) line[n] = (*c >= 'A' && *c <= 'Z') ? (char)(*c + 32) : *c;
    line[n] = 0;
    fb_text(f, x + D(20), y + D(96), line, mult, ARGB(210, 255, 255, 255));
    fb_text(f, x + D(20), y + D(96) + 16 * mult + D(8), "Contenu a venir.", mult, ARGB(160, 255, 255, 255));
    int by = y + h - D(64);
    fb_rrect(f, x + D(20), by, w - D(40), D(44), D(14), ARGB(255, 91, 108, 255));
    fb_text(f, x + (w - fb_text_w("Retour", mult)) / 2, by + (D(44) - 16 * mult) / 2, "Retour", mult, ARGB(255, 255, 255, 255));
}
