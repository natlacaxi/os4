#include "gfx.h"
#include "font8x16.h"

void fb_blend(fb_t *f, int x, int y, u32 c)
{
    if (x < 0 || y < 0 || x >= f->w || y >= f->h) return;
    u32 a = c >> 24; u32 *p = &f->px[y * f->w + x];
    if (a >= 255) { *p = c | 0xFF000000u; return; }
    u32 d = *p, na = 255 - a;
    u32 r = (((c >> 16) & 255) * a + ((d >> 16) & 255) * na) / 255;
    u32 g = (((c >> 8) & 255) * a + ((d >> 8) & 255) * na) / 255;
    u32 b = ((c & 255) * a + (d & 255) * na) / 255;
    *p = 0xFF000000u | (r << 16) | (g << 8) | b;
}
void fb_fill(fb_t *f, int x, int y, int w, int h, u32 c)
{
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) fb_blend(f, i, j, c);
}
/* rectangle arrondi, coins anti-aliases (4 sous-echantillons) */
void fb_rrect(fb_t *f, int x, int y, int w, int h, int r, u32 c)
{
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
        int cx = i < r ? r : (i >= w - r ? w - r - 1 : -1);
        int cy = j < r ? r : (j >= h - r ? h - r - 1 : -1);
        if (cx < 0 || cy < 0) { fb_blend(f, x + i, y + j, c); continue; }
        int in = 0;                                /* coordonnees x2 pour les demi-pixels */
        for (int sy = 0; sy < 2; sy++) for (int sx = 0; sx < 2; sx++) {
            int dx = (2 * i + 2 * sx + 1) - (2 * cx + 2), dy = (2 * j + 2 * sy + 1) - (2 * cy + 2);
            if (dx * dx + dy * dy <= 4 * r * r) in++;
        }
        if (!in) continue;
        u32 a = ((c >> 24) * (u32)in) / 4;
        fb_blend(f, x + i, y + j, (a << 24) | (c & 0xFFFFFF));
    }
}
int fb_text_w(const char *s, int mult) { int n = 0; while (s[n]) n++; return n * 8 * mult; }
void fb_text(fb_t *f, int x, int y, const char *s, int mult, u32 c)
{
    for (; *s; s++, x += 8 * mult) {
        if (*s < 32 || *s > 126) continue;
        const unsigned char *g = font8x16[*s - 32];
        for (int j = 0; j < 16; j++) for (int i = 0; i < 8; i++)
            if (g[j] & (0x80 >> i))
                for (int dy = 0; dy < mult; dy++) for (int dx = 0; dx < mult; dx++)
                    fb_blend(f, x + i * mult + dx, y + j * mult + dy, c);
    }
}
