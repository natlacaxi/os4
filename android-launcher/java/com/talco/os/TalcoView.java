package com.talco.os;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.BatteryManager;
import android.view.MotionEvent;
import android.view.View;

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
    native void nativeTap(int x, int y);
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
        if (e.getAction() == MotionEvent.ACTION_DOWN) { nativeTap((int) e.getX(), (int) e.getY()); invalidate(); }
        return true;
    }

    public boolean back() { boolean r = nativeBack(); if (r) invalidate(); return r; }
}
