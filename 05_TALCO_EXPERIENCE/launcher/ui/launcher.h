#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "gfx.h"
typedef enum { FF_PHONE, FF_TABLET } form_t;
/* Regle Android : plus petite largeur >= 600 dp => tablette. dpi = densite (160 = 1 px/dp). */
form_t ui_form_factor(int w_px, int h_px, int dpi);
void   launcher_set_date(int wday, int mday, int mon);   /* wday 0=dimanche, mon 0..11 */
void   launcher_draw(fb_t *f, int dpi, int hh, int mm, int battery_pct);
/* tactile : zones des icones dessinees par launcher_draw */
int         launcher_hit(int x, int y);            /* index d'app ou -1 */
const char *launcher_app_name(int i);
int         launcher_hit_count(void);
int         launcher_hit_center(int k, int *x, int *y, int *app);
void        launcher_app_window(fb_t *f, int dpi, int idx);
int         launcher_app_tap(int x, int y, int idx);      /* 1 = traite, 0 = fermer la fenetre */
void        calc_press(char c);
const char *calc_display(void);
int         launcher_calc_key_center(char c, int *x, int *y);
void        notes_press(char c);                          /* '<' = effacer */
const char *notes_text(void);
int         launcher_notes_key_center(char c, int *x, int *y);
#endif
