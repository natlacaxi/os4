package com.talco.os;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.BatteryManager;
import android.provider.ContactsContract;
import android.provider.MediaStore;
import android.provider.Settings;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

/** Affiche le launcher TALCO (rendu C) et transmet les touches. */
public class TalcoView extends View {
    static { System.loadLibrary("talco"); }
    private Bitmap bmp;
    private int[] px;
    private int w, h;
    private final Runnable tick = new Runnable() {
        @Override public void run() { invalidate(); postDelayed(this, 30000); }
    };

    native void nativeRender(int[] px, int w, int h, int dpi, int bat);
    native int nativeTap(int x, int y);
    native boolean nativeBack();

    public TalcoView(Context c) { super(c); setFocusable(true); }

    @Override protected void onAttachedToWindow() { super.onAttachedToWindow(); postDelayed(tick, 30000); }
    @Override protected void onDetachedFromWindow() { removeCallbacks(tick); super.onDetachedFromWindow(); }

    @Override protected void onSizeChanged(int nw, int nh, int ow, int oh) {
        w = nw; h = nh;
        if (w > 0 && h > 0) { bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888); px = new int[w * h]; }
    }

    @Override protected void onDraw(Canvas c) {
        if (bmp == null) return;
        int dpi = getResources().getDisplayMetrics().densityDpi;
        int bat = 78;
        try {
            BatteryManager bm = (BatteryManager) getContext().getSystemService(Context.BATTERY_SERVICE);
            int v = bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY);
            if (v > 0 && v <= 100) bat = v;
        } catch (Exception ignored) { }
        nativeRender(px, w, h, dpi, bat);
        bmp.setPixels(px, 0, w, 0, 0, w, h);
        c.drawBitmap(bmp, 0, 0, null);
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if (e.getAction() == MotionEvent.ACTION_DOWN) {
            int app = nativeTap((int) e.getX(), (int) e.getY());
            if (app >= 0) launch(app);
            invalidate();
        }
        return true;
    }

    /** Ouvre la vraie application Android correspondant a l'icone (telephone, messages, navigateur...). */
    private void launch(int idx) {
        Intent i;
        switch (idx) {
            case 0: i = new Intent(Intent.ACTION_DIAL); break;
            case 1: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MESSAGING); break;
            case 2: i = new Intent(Intent.ACTION_VIEW, ContactsContract.Contacts.CONTENT_URI); break;
            case 3: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_BROWSER); break;
            case 4: i = new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA); break;
            case 5: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_GALLERY); break;
            case 6: i = new Intent(Intent.ACTION_GET_CONTENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE); break;
            case 7: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_CALENDAR); break;
            case 8: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_EMAIL); break;
            case 9: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MUSIC); break;
            case 12: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MAPS); break;
            case 13: i = new Intent(Settings.ACTION_SETTINGS); break;
            case 14: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MARKET); break;
            case 15: i = new Intent(Intent.ACTION_ASSIST); break;
            default: return;
        }
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try { getContext().startActivity(i); }
        catch (Exception ex) { Toast.makeText(getContext(), "Application introuvable", Toast.LENGTH_SHORT).show(); }
    }

    public boolean back() { boolean r = nativeBack(); if (r) invalidate(); return r; }
}
