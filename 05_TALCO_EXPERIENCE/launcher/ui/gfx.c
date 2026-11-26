#include <stdlib.h>
#include <string.h>
#include "gfx.h"
#include "font_inter.h"

static float fsq(float x)                                   /* racine carree sans libm */
{
    if (x <= 0) return 0;
    union { float f; unsigned u; } v = { x };
    v.u = (v.u >> 1) + 0x1fbd1df5u;
    float y = v.f;
    y = 0.5f * (y + x / y); y = 0.5f * (y + x / y); y = 0.5f * (y + x / y);
    return y;
}
static float fclampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

void fb_clip_set(fb_t *f, int x, int y, int w, int h) { f->cx0 = x; f->cy0 = y; f->cx1 = x + w; f->cy1 = y + h; f->clip = 1; }
void fb_clip_clear(fb_t *f) { f->clip = 0; }

void fb_blend(fb_t *f, int x, int y, u32 c)
{
    if (x < 0 || y < 0 || x >= f->w || y >= f->h) return;
    if (f->clip && (x < f->cx0 || x >= f->cx1 || y < f->cy0 || y >= f->cy1)) return;
    u32 a = c >> 24; u32 *p = &f->px[y * f->w + x];
    if (a == 0) return;
    if (a >= 255) { *p = c | 0xFF000000u; return; }
    u32 d = *p, na = 255 - a;
    u32 r = (((c >> 16) & 255) * a + ((d >> 16) & 255) * na) / 255;
    u32 g = (((c >> 8) & 255) * a + ((d >> 8) & 255) * na) / 255;
    u32 b = ((c & 255) * a + (d & 255) * na) / 255;
    *p = 0xFF000000u | (r << 16) | (g << 8) | b;
}
static void putc_(fb_t *f, int x, int y, u32 c, float cov)
{
    int a = (int)((float)(c >> 24) * cov + 0.5f);
    if (a > 0) fb_blend(f, x, y, ((u32)a << 24) | (c & 0xFFFFFF));
}
void fb_fill(fb_t *f, int x, int y, int w, int h, u32 c)
{
    for (int j = y < 0 ? 0 : y; j < y + h && j < f->h; j++)
        for (int i = x < 0 ? 0 : x; i < x + w && i < f->w; i++) fb_blend(f, i, j, c);
}

/* couverture 0..255 d'un rectangle arrondi (coins echantillonnes en 2x2) */
static int rr_cov(int i, int j, int w, int h, int r)
{
    if (i < 0 || j < 0 || i >= w || j >= h) return 0;
    int cx = i < r ? r : (i >= w - r ? w - r - 1 : -1);
    int cy = j < r ? r : (j >= h - r ? h - r - 1 : -1);
    if (cx < 0 || cy < 0) return 255;
    int in = 0;
    for (int sy = 0; sy < 2; sy++) for (int sx = 0; sx < 2; sx++) {
        int dx = (2 * i + 2 * sx + 1) - (2 * cx + 2), dy = (2 * j + 2 * sy + 1) - (2 * cy + 2);
        if (dx * dx + dy * dy <= 4 * r * r) in++;
    }
    return in * 255 / 4;
}
static int clampr(int r, int w, int h) { if (r * 2 > h) r = h / 2; if (r * 2 > w) r = w / 2; return r < 0 ? 0 : r; }
static u32 lerp_c(u32 a, u32 b, int t, int tmax)                 /* melange ARGB */
{
    u32 o = 0;
    for (int s = 24; s >= 0; s -= 8) {
        int ca = (int)((a >> s) & 255), cb = (int)((b >> s) & 255);
        o |= (u32)(ca + (cb - ca) * t / tmax) << s;
    }
    return o;
}
void fb_rrect_grad(fb_t *f, int x, int y, int w, int h, int r, u32 top, u32 bot)
{
    r = clampr(r, w, h);
    for (int j = 0; j < h; j++) {
        u32 c = lerp_c(top, bot, j, h > 1 ? h - 1 : 1);
        int edge_row = j < r || j >= h - r;
        if ((c >> 24) == 0) continue;
        for (int i = 0; i < w; i++) {
            if (!edge_row && i >= r && i < w - r) { int e = w - r; for (; i < e; i++) fb_blend(f, x + i, y + j, c); i--; continue; }
            int k = rr_cov(i, j, w, h, r);
            if (k) putc_(f, x + i, y + j, c, (float)k / 255.0f);
        }
    }
}
void fb_rrect(fb_t *f, int x, int y, int w, int h, int r, u32 c) { fb_rrect_grad(f, x, y, w, h, r, c, c); }
void fb_rrect_outline(fb_t *f, int x, int y, int w, int h, int r, int th, u32 top, u32 bot)
{
    r = clampr(r, w, h);
    int ri = r > th ? r - th : 0;
    for (int j = 0; j < h; j++) {
        u32 c = lerp_c(top, bot, j, h > 1 ? h - 1 : 1);
        int band = r + th;
        for (int i = 0; i < w; i++) {
            int in_x = i >= th && i < w - th, in_y = j >= th && j < h - th;
            if (in_x && in_y && j >= band && j < h - band) { i = w - th - 1; continue; }     /* milieu des cotes : rien a faire */
            if (in_x && in_y && i >= band && i < w - band && j >= th && j < h - th) { continue; }
            int o = rr_cov(i, j, w, h, r);
            if (!o) continue;
            int in = (in_x && in_y) ? rr_cov(i - th, j - th, w - 2 * th, h - 2 * th, clampr(ri, w - 2 * th, h - 2 * th)) : 0;
            int k = o - in; if (k > 0) putc_(f, x + i, y + j, c, (float)k / 255.0f);
        }
    }
}
void fb_shadow(fb_t *f, int x, int y, int w, int h, int r, int spread, int alpha)
{
    int n = spread < 6 ? spread : 6;                          /* peu de couches : rapide sur telephone */
    for (int i = n; i >= 1; i--) {
        int k = spread * i / n, j = n - i + 1;
        int a = alpha * j * j * spread / (n * n * n * 3);
        if (a > 255) a = 255;
        if (a > 0) fb_rrect(f, x - k, y - k, w + 2 * k, h + 2 * k, r + k, ARGB(a, 0, 0, 0));
    }
}

/* flou par boites glissantes (2 passes H+V), bords repliques */
static void box_pass(const u32 *src, u32 *dst, int w, int h, int r, int vertical)
{
    int n = vertical ? h : w, lines = vertical ? w : h, win = 2 * r + 1;
    for (int l = 0; l < lines; l++) {
        #define PX(k) src[vertical ? (k) * w + l : l * w + (k)]
        #define OUT(k) dst[vertical ? (k) * w + l : l * w + (k)]
        int sa = 0, sr = 0, sg = 0, sb = 0;
        for (int k = -r; k <= r; k++) {
            u32 p = PX(k < 0 ? 0 : k >= n ? n - 1 : k);
            sr += (p >> 16) & 255; sg += (p >> 8) & 255; sb += p & 255;
        }
        (void)sa;
        for (int k = 0; k < n; k++) {
            OUT(k) = 0xFF000000u | ((u32)(sr / win) << 16) | ((u32)(sg / win) << 8) | (u32)(sb / win);
            u32 add = PX(k + r + 1 >= n ? n - 1 : k + r + 1), rem = PX(k - r < 0 ? 0 : k - r);
            sr += (int)((add >> 16) & 255) - (int)((rem >> 16) & 255);
            sg += (int)((add >> 8) & 255) - (int)((rem >> 8) & 255);
            sb += (int)(add & 255) - (int)(rem & 255);
        }
        #undef PX
        #undef OUT
    }
}
static u32 *blurred_copy(fb_t *f, int x, int y, int w, int h, int radius)
{
    u32 *a = malloc((size_t)w * h * 4), *b = malloc((size_t)w * h * 4);
    if (!a || !b) { free(a); free(b); return 0; }
    for (int j = 0; j < h; j++) memcpy(a + (size_t)j * w, f->px + (size_t)(y + j) * f->w + x, (size_t)w * 4);
    if (radius > 0)
        for (int it = 0; it < 2; it++) { box_pass(a, b, w, h, radius, 0); box_pass(b, a, w, h, radius, 1); }
    free(b);
    return a;
}
static int clip_rect(fb_t *f, int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > f->w) *w = f->w - *x;
    if (*y + *h > f->h) *h = f->h - *y;
    return *w > 0 && *h > 0;
}
void fb_blur_rect(fb_t *f, int x, int y, int w, int h, int radius)
{
    if (!clip_rect(f, &x, &y, &w, &h)) return;
    u32 *a = blurred_copy(f, x, y, w, h, radius);
    if (!a) return;
    for (int j = 0; j < h; j++) memcpy(f->px + (size_t)(y + j) * f->w + x, a + (size_t)j * w, (size_t)w * 4);
    free(a);
}
void fb_glass(fb_t *f, int x, int y, int w, int h, int r, int blur, u32 tint)
{
    int ox = x, oy = y, ow = w, oh = h;
    if (!clip_rect(f, &x, &y, &w, &h)) return;
    r = clampr(r, ow, oh);
    u32 *a = blur > 0 ? blurred_copy(f, x, y, w, h, blur) : 0;
    if (a) {
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
            int k = rr_cov(i + x - ox, j + y - oy, ow, oh, r);
            if (!k) continue;
            u32 s = a[(size_t)j * w + i];
            fb_blend(f, x + i, y + j, ((u32)k << 24) | (s & 0xFFFFFF));
        }
        free(a);
    }
    u32 ta = tint >> 24, tb = ta + ta / 2 > 255 ? 255 : ta + ta / 2;
    fb_rrect_grad(f, ox, oy, ow, oh, r, (tb << 24) | (tint & 0xFFFFFF), tint);
    int th = f->w >= 900 ? 2 : 1;
    fb_rrect_outline(f, ox, oy, ow, oh, r, th, ARGB(140, 255, 255, 255), ARGB(25, 255, 255, 255));
}
void fb_blob(fb_t *f, int cx, int cy, int r, u32 rgb, int amax)
{
    long r2 = (long)r * r;
    for (int y = cy - r; y <= cy + r; y++) {
        if (y < 0 || y >= f->h) continue;
        for (int x = cx - r; x <= cx + r; x++) {
            if (x < 0 || x >= f->w) continue;
            long d2 = (long)(x - cx) * (x - cx) + (long)(y - cy) * (y - cy);
            if (d2 >= r2) continue;
            long t = 256 - d2 * 256 / r2;
            int a = (int)(amax * t * t >> 16);
            if (a > 0) fb_blend(f, x, y, ((u32)a << 24) | (rgb & 0xFFFFFF));
        }
    }
}

void fb_disc(fb_t *f, float cx, float cy, float r, u32 c)
{
    for (int y = (int)(cy - r - 1); y <= (int)(cy + r + 1); y++) for (int x = (int)(cx - r - 1); x <= (int)(cx + r + 1); x++) {
        float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
        float cov = fclampf(r - fsq(dx * dx + dy * dy) + 0.5f, 0, 1);
        if (cov > 0) putc_(f, x, y, c, cov);
    }
}
void fb_ring_q(fb_t *f, float cx, float cy, float r, float th, int quad, u32 c)
{
    float R = r + th;
    for (int y = (int)(cy - R - 1); y <= (int)(cy + R + 1); y++) for (int x = (int)(cx - R - 1); x <= (int)(cx + R + 1); x++) {
        float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
        int q = dx < 0 ? (dy < 0 ? 1 : 4) : (dy < 0 ? 2 : 8);
        if (!(quad & q)) continue;
        float d = fsq(dx * dx + dy * dy), cov = fclampf(th / 2 - (d > r ? d - r : r - d) + 0.5f, 0, 1);
        if (cov > 0) putc_(f, x, y, c, cov);
    }
}
void fb_ring(fb_t *f, float cx, float cy, float r, float th, u32 c) { fb_ring_q(f, cx, cy, r, th, 15, c); }
void fb_line(fb_t *f, float x0, float y0, float x1, float y1, float th, u32 c)
{
    float minx = x0 < x1 ? x0 : x1, maxx = x0 < x1 ? x1 : x0, miny = y0 < y1 ? y0 : y1, maxy = y0 < y1 ? y1 : y0;
    float dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
    for (int y = (int)(miny - th - 1); y <= (int)(maxy + th + 1); y++) for (int x = (int)(minx - th - 1); x <= (int)(maxx + th + 1); x++) {
        float px = (float)x + 0.5f - x0, py = (float)y + 0.5f - y0;
        float t = l2 > 0 ? fclampf((px * dx + py * dy) / l2, 0, 1) : 0;
        float ex = px - t * dx, ey = py - t * dy;
        float cov = fclampf(th / 2 - fsq(ex * ex + ey * ey) + 0.5f, 0, 1);
        if (cov > 0) putc_(f, x, y, c, cov);
    }
}
static float edge(float ax, float ay, float bx, float by, float px, float py) { return (px - ax) * (by - ay) - (py - ay) * (bx - ax); }
void fb_tri(fb_t *f, float x0, float y0, float x1, float y1, float x2, float y2, u32 c)
{
    float minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    float miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    float area = edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;
    for (int y = (int)miny - 1; y <= (int)maxy + 1; y++) for (int x = (int)minx - 1; x <= (int)maxx + 1; x++) {
        int in = 0;
        for (int s = 0; s < 4; s++) {
            float px = (float)x + (s & 1 ? 0.75f : 0.25f), py = (float)y + (s & 2 ? 0.75f : 0.25f);
            float w0 = edge(x1, y1, x2, y2, px, py), w1 = edge(x2, y2, x0, y0, px, py), w2 = edge(x0, y0, x1, y1, px, py);
            if (area > 0 ? (w0 >= 0 && w1 >= 0 && w2 >= 0) : (w0 <= 0 && w1 <= 0 && w2 <= 0)) in++;
        }
        if (in) putc_(f, x, y, c, (float)in / 4.0f);
    }
}

/* ---------------- texte anti-alias (atlas Inter) ---------------- */
static unsigned next_cp(const char **s)
{
    unsigned char c = (unsigned char)**s; (*s)++;
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && **s) { unsigned c2 = (unsigned char)**s & 0x3F; (*s)++; return ((c & 0x1Fu) << 6) | c2; }
    if ((c & 0xF0) == 0xE0 && **s && (*s)[1]) {
        unsigned c2 = (unsigned char)(*s)[0] & 0x3F, c3 = (unsigned char)(*s)[1] & 0x3F; *s += 2;
        return ((c & 0x0Fu) << 12) | (c2 << 6) | c3;
    }
    return '?';
}
static const fglyph_t *find(const fatlas_t *a, unsigned cp)
{
    for (int i = 0; i < a->n; i++) if (a->g[i].cp == cp) return &a->g[i];
    if (cp != '?') return find(a, '?');
    return &a->g[0];
}
static const fatlas_t *pick_atlas(const char *s, int size)
{
    if (size < 70) return &atlas_text;
    for (; *s; s++) if (!((*s >= '0' && *s <= '9') || *s == ':')) return &atlas_text;
    return &atlas_big;
}
int fb_text_w(const char *s, int size)
{
    const fatlas_t *a = pick_atlas(s, size); float w = 0;
    while (*s) w += (float)find(a, next_cp(&s))->adv16 / 16.0f;
    return (int)(w * (float)size / (float)a->size + 0.5f);
}
static float fetch(const unsigned char *d, int w, int h, int ix, int iy) { return (ix < 0 || iy < 0 || ix >= w || iy >= h) ? 0 : (float)d[iy * w + ix]; }
static float bilin(const unsigned char *d, int w, int h, float sx, float sy)
{
    float fx = sx - 0.5f, fy = sy - 0.5f; int ix = (int)(fx < 0 ? fx - 1 : fx), iy = (int)(fy < 0 ? fy - 1 : fy);
    float tx = fx - (float)ix, ty = fy - (float)iy;
    float a = fetch(d, w, h, ix, iy), b = fetch(d, w, h, ix + 1, iy), c = fetch(d, w, h, ix, iy + 1), e = fetch(d, w, h, ix + 1, iy + 1);
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + e * tx) * ty;
}
void fb_text(fb_t *f, int x, int y, const char *s, int size, u32 c)
{
    const fatlas_t *a = pick_atlas(s, size);
    float sc = (float)size / (float)a->size, pen = (float)x, base = (float)y + (float)size * 0.80f;
    int nt = sc < 1 ? (int)(1.0f / sc + 0.99f) : 1; if (nt > 4) nt = 4;
    while (*s) {
        const fglyph_t *g = find(a, next_cp(&s));
        if (g->w && g->h) {
            const unsigned char *d = a->data + g->off;
            float x0 = pen + (float)g->xo * sc, y0 = base + (float)g->yo * sc;
            int bx = (int)x0 - 1, by = (int)y0 - 1, dw = (int)((float)g->w * sc) + 3, dh = (int)((float)g->h * sc) + 3;
            for (int dy = 0; dy < dh; dy++) for (int dx = 0; dx < dw; dx++) {
                float sumv = 0;
                for (int ty = 0; ty < nt; ty++) for (int tx = 0; tx < nt; tx++) {
                    float px = ((float)(bx + dx) + (tx + 0.5f) / (float)nt - x0) / sc, py = ((float)(by + dy) + (ty + 0.5f) / (float)nt - y0) / sc;
                    sumv += bilin(d, g->w, g->h, px, py);
                }
                float cov = sumv / (float)(nt * nt) / 255.0f;
                if (cov > 0.004f) putc_(f, bx + dx, by + dy, c, cov > 1 ? 1 : cov);
            }
        }
        pen += (float)g->adv16 / 16.0f * sc;
    }
}
