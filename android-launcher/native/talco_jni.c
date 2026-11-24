/* Pont JNI : fait tourner le launcher TALCO (C portable) dans une application Android. */
#include <jni.h>
#include <stdlib.h>
#include <time.h>
#include "launcher.h"

static fb_t fb;
static int app = -1;

JNIEXPORT void JNICALL Java_com_talco_os_TalcoView_nativeRender(JNIEnv *e, jobject o, jintArray arr, jint w, jint h, jint dpi, jint bat)
{
    (void)o;
    if (!fb.px || fb.w != w || fb.h != h) { free(fb.px); fb.px = calloc((size_t)w * h, 4); fb.w = w; fb.h = h; app = -1; }
    time_t t = time(0); struct tm tmv; localtime_r(&t, &tmv);
    launcher_set_date(tmv.tm_wday, tmv.tm_mday, tmv.tm_mon);
    launcher_render(&fb, dpi, tmv.tm_hour, tmv.tm_min, bat, app);
    (*e)->SetIntArrayRegion(e, arr, 0, w * h, (const jint *)fb.px);
}
/* retourne l'index d'une application a ouvrir par Android, ou -1 */
JNIEXPORT jint JNICALL Java_com_talco_os_TalcoView_nativeTap(JNIEnv *e, jobject o, jint x, jint y)
{
    (void)e; (void)o;
    if (app >= 0) { if (!launcher_app_tap(x, y, app)) app = -1; return -1; }
    int i = launcher_hit(x, y);
    if (i < 0) return -1;
    if (launcher_app_internal(i)) { app = i; return -1; }
    return i;
}
JNIEXPORT jboolean JNICALL Java_com_talco_os_TalcoView_nativeBack(JNIEnv *e, jobject o)
{
    (void)e; (void)o;
    if (app >= 0) { app = -1; return JNI_TRUE; }
    return JNI_FALSE;
}
