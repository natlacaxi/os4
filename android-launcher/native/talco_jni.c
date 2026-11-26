/* Pont JNI : fait tourner le launcher TALCO (C portable) dans une application Android. */
#include <jni.h>
#include <stdlib.h>
#include <time.h>
#include "launcher.h"

static fb_t fb;
static int app = -1, last_arg;

#define JF(name) Java_com_talco_os_TalcoView_##name

JNIEXPORT void JNICALL JF(nativeInit)(JNIEnv *e, jobject o) { (void)e; (void)o; launcher_set_host_apps(1); }

JNIEXPORT void JNICALL JF(nativeRender)(JNIEnv *e, jobject o, jintArray arr, jint w, jint h, jint dpi, jint bat)
{
    (void)o;
    if (!fb.px || fb.w != w || fb.h != h) { free(fb.px); fb.px = calloc((size_t)w * h, 4); fb.w = w; fb.h = h; app = -1; }
    time_t t = time(0); struct tm tmv; localtime_r(&t, &tmv);
    launcher_set_date(tmv.tm_wday, tmv.tm_mday, tmv.tm_mon);
    launcher_render(&fb, dpi, tmv.tm_hour, tmv.tm_min, bat, app);
    (*e)->SetIntArrayRegion(e, arr, 0, w * h, (const jint *)fb.px);
}
/* retourne l'index d'une application a ouvrir par Android, ou -1 */
JNIEXPORT jint JNICALL JF(nativeTap)(JNIEnv *e, jobject o, jint x, jint y)
{
    (void)e; (void)o;
    if (app >= 0) { if (!launcher_app_tap(x, y, app)) app = -1; return -1; }
    int i = launcher_hit(x, y);
    if (i < 0) return -1;
    if (launcher_app_internal(i)) { app = i; launcher_app_opened(i); return -1; }
    return i;
}
JNIEXPORT jboolean JNICALL JF(nativeBack)(JNIEnv *e, jobject o)
{
    (void)e; (void)o;
    if (app >= 0) { app = -1; return JNI_TRUE; }
    return JNI_FALSE;
}
JNIEXPORT jint JNICALL JF(nativeApp)(JNIEnv *e, jobject o) { (void)e; (void)o; return app; }
JNIEXPORT jint JNICALL JF(nativeEvent)(JNIEnv *e, jobject o) { (void)e; (void)o; return launcher_event(&last_arg); }
JNIEXPORT jint JNICALL JF(nativeEventArg)(JNIEnv *e, jobject o) { (void)e; (void)o; return last_arg; }
JNIEXPORT void JNICALL JF(nativeScroll)(JNIEnv *e, jobject o, jint dy) { (void)e; (void)o; if (app >= 0) launcher_scroll(app, dy); }
JNIEXPORT jboolean JNICALL JF(nativeLongPress)(JNIEnv *e, jobject o, jint x, jint y)
{
    (void)e; (void)o;
    return (app >= 0 && launcher_long_press(x, y, app)) ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT void JNICALL JF(nativeSetVolumes)(JNIEnv *e, jobject o, jstring data, jint sel)
{
    (void)o; const char *c = (*e)->GetStringUTFChars(e, data, 0);
    if (c) { launcher_fm_set_volumes(c, sel); (*e)->ReleaseStringUTFChars(e, data, c); }
}
JNIEXPORT void JNICALL JF(nativeSetDir)(JNIEnv *e, jobject o, jstring path, jstring data)
{
    (void)o; const char *p = (*e)->GetStringUTFChars(e, path, 0), *c = (*e)->GetStringUTFChars(e, data, 0);
    if (p && c) launcher_fm_set_dir(p, c);
    if (p) (*e)->ReleaseStringUTFChars(e, path, p);
    if (c) (*e)->ReleaseStringUTFChars(e, data, c);
}
JNIEXPORT void JNICALL JF(nativeSetInfo)(JNIEnv *e, jobject o, jstring data)
{
    (void)o; const char *c = (*e)->GetStringUTFChars(e, data, 0);
    if (c) { launcher_info_set(c); (*e)->ReleaseStringUTFChars(e, data, c); }
}
JNIEXPORT void JNICALL JF(nativeSetVolume)(JNIEnv *e, jobject o, jint pct) { (void)e; (void)o; launcher_set_volume_pct(pct); }
JNIEXPORT void JNICALL JF(nativeSetClipboard)(JNIEnv *e, jobject o, jint has) { (void)e; (void)o; launcher_set_clipboard(has); }
