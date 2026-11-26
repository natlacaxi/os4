package com.talco.os;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.wifi.WifiManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.StatFs;
import android.provider.Settings;
import android.telephony.TelephonyManager;

import java.util.List;

/** Collecte l'etat de l'appareil sous forme de lignes « libelle<TAB>valeur<TAB>pourcentage ». */
class DeviceInfo {
    private static void add(StringBuilder b, String label, String value, int pct) {
        b.append(label).append('\t').append(value).append('\t').append(pct < 0 ? "" : String.valueOf(pct)).append('\n');
    }

    static String size(long bytes) {
        double v = bytes;
        String u = "o";
        if (bytes >= 1000000000L) { v = bytes / 1e9; u = "Go"; }
        else if (bytes >= 1000000L) { v = bytes / 1e6; u = "Mo"; }
        else if (bytes >= 1000L) { v = bytes / 1e3; u = "Ko"; }
        long t = Math.round(v * 10);
        String s = (t % 10 == 0) ? String.valueOf(t / 10) : (t / 10) + "," + (t % 10);
        return s + " " + u;
    }

    private static int pct(long used, long total) { return total <= 0 ? -1 : (int) (used * 100 / total); }

    static String collect(Context c) {
        StringBuilder b = new StringBuilder();
        add(b, "Appareil", Build.MANUFACTURER + " " + Build.MODEL, -1);
        add(b, "Android", Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")", -1);
        try {
            List<FileManager.Vol> vols = FileManager.volumes(c);
            for (FileManager.Vol v : vols) {
                StatFs s = new StatFs(v.root.getAbsolutePath());
                long tot = s.getTotalBytes(), fr = s.getAvailableBytes();
                String name = v.label.equals("Interne") ? "Stockage interne" : v.label;
                add(b, name, size(fr) + " libres sur " + size(tot), pct(tot - fr, tot));
            }
        } catch (Exception ignored) { }
        try {
            ActivityManager am = (ActivityManager) c.getSystemService(Context.ACTIVITY_SERVICE);
            ActivityManager.MemoryInfo mi = new ActivityManager.MemoryInfo();
            am.getMemoryInfo(mi);
            add(b, "Mémoire (RAM)", size(mi.availMem) + " libres sur " + size(mi.totalMem), pct(mi.totalMem - mi.availMem, mi.totalMem));
        } catch (Exception ignored) { }
        try {
            Intent bi = c.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
            if (bi != null) {
                int lvl = bi.getIntExtra(BatteryManager.EXTRA_LEVEL, -1), sc = bi.getIntExtra(BatteryManager.EXTRA_SCALE, 100);
                int st = bi.getIntExtra(BatteryManager.EXTRA_STATUS, -1), tp = bi.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, 0);
                int p = sc > 0 ? lvl * 100 / sc : -1;
                boolean ch = st == BatteryManager.BATTERY_STATUS_CHARGING || st == BatteryManager.BATTERY_STATUS_FULL;
                add(b, "Batterie", p + " %" + (ch ? " · en charge" : "") + " · " + (tp / 10) + " °C", p);
            }
        } catch (Exception ignored) { }
        try {
            WifiManager w = (WifiManager) c.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            add(b, "Wi-Fi", w.isWifiEnabled() ? "Activé" : "Désactivé", -1);
        } catch (Exception ignored) { }
        try {
            if (!c.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
                add(b, "Carte SIM", "Pas de modem sur cet appareil", -1);
            } else {
                TelephonyManager tm = (TelephonyManager) c.getSystemService(Context.TELEPHONY_SERVICE);
                int st = tm.getSimState();
                String v;
                if (st == TelephonyManager.SIM_STATE_READY) {
                    String op = tm.getSimOperatorName();
                    v = "Prête" + (op != null && op.length() > 0 ? " · " + op : "");
                } else if (st == TelephonyManager.SIM_STATE_ABSENT) v = "Absente";
                else if (st == TelephonyManager.SIM_STATE_PIN_REQUIRED) v = "Code PIN requis";
                else v = "Indisponible";
                add(b, "Carte SIM", v, -1);
            }
        } catch (Exception ignored) { }
        try {
            int br = Settings.System.getInt(c.getContentResolver(), Settings.System.SCREEN_BRIGHTNESS);
            add(b, "Luminosité", (br * 100 / 255) + " %", -1);
        } catch (Exception ignored) { }
        return b.toString();
    }
}
