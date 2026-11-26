#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "gfx.h"
typedef enum { FF_PHONE, FF_TABLET } form_t;
/* Regle Android : plus petite largeur >= 600 dp => tablette. dpi = densite (160 = 1 px/dp). */
form_t ui_form_factor(int w_px, int h_px, int dpi);
void   launcher_set_date(int wday, int mday, int mon);   /* wday 0=dimanche, mon 0..11 */
void   launcher_render(fb_t *f, int dpi, int hh, int mm, int battery_pct, int app);   /* accueil ou app ouverte (index, ou -1), avec cache */
void   launcher_draw(fb_t *f, int dpi, int hh, int mm, int battery_pct);
/* tactile : zones des icones dessinees par launcher_draw */
int         launcher_hit(int x, int y);            /* index d'app ou -1 */
const char *launcher_app_name(int i);
int         launcher_hit_count(void);
int         launcher_hit_center(int k, int *x, int *y, int *app);
void        launcher_app_window(fb_t *f, int dpi, int idx);
int         launcher_app_internal(int idx);               /* 1 = fenetre TALCO, 0 = a ouvrir par le systeme */
int         launcher_app_tap(int x, int y, int idx);      /* 1 = traite, 0 = fermer la fenetre */
void        calc_press(char c);
const char *calc_display(void);
int         launcher_calc_key_center(char c, int *x, int *y);
void        notes_press(char c);                          /* '<' = effacer */
const char *notes_text(void);
int         launcher_notes_key_center(char c, int *x, int *y);

/* ---- Fichiers et Reglages (donnees fournies par le systeme hote, p. ex. Android) ---- */
enum { EV_NONE = 0, EV_FM_ENTER, EV_FM_OPEN, EV_FM_UP, EV_FM_VOLUME, EV_FM_NEWDIR, EV_FM_MENU, EV_FM_PASTE, EV_SET_ENTER, EV_SET_VOLUME, EV_SET_OPEN };
void launcher_set_host_apps(int on);                       /* 1 = Fichiers et Reglages ont leur propre fenetre alimentee par l'hote */
void launcher_app_opened(int idx);                         /* a appeler quand une fenetre vient de s'ouvrir (declenche EV_*_ENTER) */
int  launcher_event(int *arg);                             /* evenement en attente pour l'hote (0 = aucun) */
void launcher_fm_set_volumes(const char *data, int sel);   /* lignes « libelle<TAB>total<TAB>libre » */
void launcher_fm_set_dir(const char *path, const char *data); /* lignes « nom<TAB>taille<TAB>date<TAB>dossier(0/1) » ; premiere ligne « !message » = erreur */
int  launcher_fm_count(void);
int  launcher_fm_order(int i);                             /* index d'origine de la i-eme ligne affichee (apres tri) */
void launcher_fm_sort(int key, int desc);                  /* 0 nom, 1 taille, 2 date, 3 type */
void launcher_set_clipboard(int has);
void launcher_info_set(const char *data);                  /* lignes « libelle<TAB>valeur<TAB>pourcentage(-1 = pas de barre) » */
void launcher_set_volume_pct(int pct);
void launcher_scroll(int idx, int dy);
int  launcher_long_press(int x, int y, int idx);
/* accesseurs pour les tests */
const char *launcher_fm_name(int i);
long long   launcher_fm_size(int i);
long long   launcher_fm_mtime(int i);
int         launcher_fm_row_center(int k, int *x, int *y);
int         launcher_fm_chip_center(int i, int *x, int *y);
int         launcher_sc_button_center(int i, int *x, int *y);
int         launcher_sc_slider_rect(int *x, int *y, int *w);
#endif
