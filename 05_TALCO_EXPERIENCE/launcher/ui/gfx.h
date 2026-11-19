#ifndef GFX_H
#define GFX_H
/* Mini bibliotheque graphique portable (aucune dependance libc) : noyau, host, emulateur. */
typedef unsigned int u32;
typedef struct { u32 *px; int w, h; } fb_t;     /* pixels 0xAARRGGBB, stride = w */
#define ARGB(a, r, g, b) (((u32)(a) << 24) | ((u32)(r) << 16) | ((u32)(g) << 8) | (u32)(b))
void fb_blend(fb_t *f, int x, int y, u32 c);
void fb_fill(fb_t *f, int x, int y, int w, int h, u32 c);
void fb_rrect(fb_t *f, int x, int y, int w, int h, int r, u32 c);
void fb_text(fb_t *f, int x, int y, const char *s, int mult, u32 c);
int  fb_text_w(const char *s, int mult);
#endif
