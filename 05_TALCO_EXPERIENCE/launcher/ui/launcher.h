#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "gfx.h"
typedef enum { FF_PHONE, FF_TABLET } form_t;
/* Regle Android : plus petite largeur >= 600 dp => tablette. dpi = densite (160 = 1 px/dp). */
form_t ui_form_factor(int w_px, int h_px, int dpi);
void   launcher_draw(fb_t *f, int dpi, int hh, int mm, int battery_pct);
#endif
