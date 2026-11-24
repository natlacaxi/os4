#ifndef GFX_H
#define GFX_H
/* Bibliotheque graphique portable (noyau Linux userspace, Android JNI, hote) : pixels 0xAARRGGBB, stride = w. Necessite malloc. */
typedef unsigned int u32;
typedef struct { u32 *px; int w, h; } fb_t;
#define ARGB(a, r, g, b) (((u32)(a) << 24) | ((u32)(r) << 16) | ((u32)(g) << 8) | (u32)(b))

void fb_blend(fb_t *f, int x, int y, u32 c);
void fb_fill(fb_t *f, int x, int y, int w, int h, u32 c);
void fb_rrect(fb_t *f, int x, int y, int w, int h, int r, u32 c);
void fb_rrect_grad(fb_t *f, int x, int y, int w, int h, int r, u32 top, u32 bot);
void fb_rrect_outline(fb_t *f, int x, int y, int w, int h, int r, int th, u32 top, u32 bot);
void fb_shadow(fb_t *f, int x, int y, int w, int h, int r, int spread, int alpha);
void fb_blur_rect(fb_t *f, int x, int y, int w, int h, int radius);
void fb_glass(fb_t *f, int x, int y, int w, int h, int r, int blur, u32 tint);   /* verre depoli : flou + teinte + reflet de bord */
void fb_blob(fb_t *f, int cx, int cy, int r, u32 rgb, int amax);                 /* tache de couleur douce (fond d'ecran) */
void fb_disc(fb_t *f, float cx, float cy, float r, u32 c);
void fb_ring(fb_t *f, float cx, float cy, float r, float th, u32 c);
void fb_ring_q(fb_t *f, float cx, float cy, float r, float th, int quad, u32 c); /* quad : 1=HG 2=HD 4=BG 8=BD */
void fb_line(fb_t *f, float x0, float y0, float x1, float y1, float th, u32 c);
void fb_tri(fb_t *f, float x0, float y0, float x1, float y1, float x2, float y2, u32 c);
int  fb_text_w(const char *s, int size);                                         /* taille en pixels (hauteur de ligne) */
void fb_text(fb_t *f, int x, int y, const char *s, int size, u32 c);            /* y = haut de la ligne, texte UTF-8 */
#endif
