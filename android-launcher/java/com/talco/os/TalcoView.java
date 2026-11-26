package com.talco.os;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.media.AudioManager;
import android.net.Uri;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Environment;
import android.provider.ContactsContract;
import android.provider.MediaStore;
import android.provider.Settings;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.webkit.MimeTypeMap;
import android.widget.EditText;
import android.widget.Toast;

import java.io.File;

/** Affiche TALCO (rendu C), transmet le tactile, et fournit les donnees (fichiers, etat de l'appareil) et actions (ouvrir une appli...). */
public class TalcoView extends View {
    static { System.loadLibrary("talco"); }

    private static final int IDX_FILES = 6, IDX_SETTINGS = 13;

    native void nativeInit();
    native void nativeRender(int[] px, int w, int h, int dpi, int bat);
    native int nativeTap(int x, int y);
    native boolean nativeBack();
    native int nativeApp();
    native int nativeEvent();
    native int nativeEventArg();
    native void nativeScroll(int dy);
    native boolean nativeLongPress(int x, int y);
    native void nativeSetVolumes(String data, int sel);
    native void nativeSetDir(String path, String data);
    native void nativeSetInfo(String data);
    native void nativeSetVolume(int pct);
    native void nativeSetClipboard(int has);

    private final Activity act;
    private final FileManager fm;
    private Bitmap bmp;
    private int[] px;
    private int w, h;
    private float downX, downY, lastY;
    private boolean moved, longFired;
    private final int slop;

    private final Runnable tick = new Runnable() {
        @Override public void run() { invalidate(); postDelayed(this, 30000); }
    };
    private final Runnable longPress = new Runnable() {
        @Override public void run() {
            if (!moved && nativeLongPress((int) downX, (int) downY)) {
                longFired = true;
                handleEvents();
                invalidate();
            }
        }
    };

    public TalcoView(Context c) {
        super(c);
        act = (Activity) c;
        fm = new FileManager(c);
        slop = ViewConfiguration.get(c).getScaledTouchSlop();
        setFocusable(true);
        nativeInit();
    }

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
        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                downX = e.getX(); downY = e.getY(); lastY = downY; moved = false; longFired = false;
                postDelayed(longPress, 450);
                return true;
            case MotionEvent.ACTION_MOVE:
                if (!moved && Math.abs(e.getY() - downY) > slop) { moved = true; removeCallbacks(longPress); }
                if (moved) { nativeScroll((int) (lastY - e.getY())); lastY = e.getY(); invalidate(); }
                return true;
            case MotionEvent.ACTION_UP:
                removeCallbacks(longPress);
                if (!moved && !longFired) {
                    int app = nativeTap((int) e.getX(), (int) e.getY());
                    if (app >= 0) launch(app);
                    handleEvents();
                    invalidate();
                }
                return true;
            case MotionEvent.ACTION_CANCEL:
                removeCallbacks(longPress);
                return true;
            default:
                return true;
        }
    }

    /** Touche Retour : remonte d'un dossier dans Fichiers, sinon ferme la fenetre TALCO ouverte. */
    public boolean back() {
        if (nativeApp() == IDX_FILES && fm.dir != null && fm.canUp()) { fm.up(); pushFiles(); return true; }
        boolean r = nativeBack();
        if (r) invalidate();
        return r;
    }

    /** Au retour dans l'appli (par ex. apres l'autorisation d'acces aux fichiers) : recharge les donnees affichees. */
    public void refresh() {
        int a = nativeApp();
        if (a == IDX_FILES) enterFiles();
        else if (a == IDX_SETTINGS) pushInfo();
        invalidate();
    }

    // ------------------------------------------------------------------ evenements venant du dessin (C)
    private void handleEvents() {
        int code;
        while ((code = nativeEvent()) != 0) onEvent(code, nativeEventArg());
    }

    private void onEvent(int code, int arg) {
        switch (code) {
            case 1: enterFiles(); break;                                  // FM_ENTER
            case 2: { File f = fm.open(arg); if (f != null) openFile(f); pushFiles(); break; }   // FM_OPEN
            case 3: fm.up(); pushFiles(); break;                          // FM_UP
            case 4: fm.selectVolume(arg); pushFiles(); break;             // FM_VOLUME
            case 5: askNewFolder(); break;                                // FM_NEWDIR
            case 6: fileMenu(arg); break;                                 // FM_MENU (appui long)
            case 7: doPaste(); break;                                     // FM_PASTE
            case 8: pushInfo(); break;                                    // SET_ENTER
            case 9: setVolume(arg); break;                                // SET_VOLUME
            case 10: openSystemSettings(arg); break;                      // SET_OPEN
            default: break;
        }
    }

    private void toast(String s) { Toast.makeText(getContext(), s, Toast.LENGTH_SHORT).show(); }

    // ------------------------------------------------------------------ Fichiers
    private boolean hasAccess() {
        if (Build.VERSION.SDK_INT >= 30) return Environment.isExternalStorageManager();
        if (Build.VERSION.SDK_INT >= 23) return act.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        return true;
    }

    private void requestAccess() {
        try {
            if (Build.VERSION.SDK_INT >= 30) {
                Intent i = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + act.getPackageName()));
                act.startActivity(i);
            } else if (Build.VERSION.SDK_INT >= 23) {
                act.requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE }, 1);
            }
        } catch (Exception e) {
            try { act.startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)); } catch (Exception ignored) { }
        }
    }

    private void enterFiles() {
        if (!hasAccess()) {
            nativeSetVolumes("Interne\t0\t0\n", 0);
            nativeSetDir("/", "!Autorise l’accès aux fichiers dans la fenêtre Android, puis reviens ici.");
            requestAccess();
            invalidate();
            return;
        }
        fm.enter();
        pushFiles();
    }

    private void pushFiles() {
        if (fm.dir == null) { enterFiles(); return; }
        nativeSetVolumes(fm.volumesData(), fm.volIdx);
        nativeSetClipboard(fm.clip != null ? 1 : 0);
        nativeSetDir(fm.pathLabel(), fm.dirData());
        invalidate();
    }

    private void openFile(File f) {
        String n = f.getName();
        int d = n.lastIndexOf('.');
        String ext = d >= 0 ? n.substring(d + 1).toLowerCase() : "";
        String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(ext);
        if (mime == null) mime = "*/*";
        Intent i = new Intent(Intent.ACTION_VIEW);
        i.setDataAndType(Uri.fromFile(f), mime);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try { act.startActivity(i); } catch (Exception ex) { toast("Aucune application pour ouvrir ce fichier"); }
    }

    private void fileMenu(final int idx) {
        final File f = fm.item(idx);
        if (f == null) return;
        new AlertDialog.Builder(act).setTitle(f.getName())
            .setItems(new String[] { "Renommer", "Copier", "Déplacer", "Supprimer" }, new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface dlg, int which) {
                    if (which == 0) askRename(f);
                    else if (which == 1) { fm.setClip(f, false); pushFiles(); toast("Copié : ouvre le dossier voulu puis « Coller »"); }
                    else if (which == 2) { fm.setClip(f, true); pushFiles(); toast("À déplacer : ouvre le dossier voulu puis « Coller »"); }
                    else askDelete(f);
                }
            }).show();
    }

    private void askNewFolder() {
        final EditText in = new EditText(act);
        in.setSingleLine(true);
        new AlertDialog.Builder(act).setTitle("Nouveau dossier").setView(in)
            .setPositiveButton("Créer", new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface d, int w) {
                    String n = in.getText().toString().trim();
                    if (!FileManager.valid(n)) toast("Nom invalide");
                    else if (!fm.mkdir(n)) toast("Impossible de créer le dossier");
                    fm.enterSame();
                    pushFiles();
                }
            }).setNegativeButton("Annuler", null).show();
    }

    private void askRename(final File f) {
        final EditText in = new EditText(act);
        in.setSingleLine(true);
        in.setText(f.getName());
        new AlertDialog.Builder(act).setTitle("Renommer").setView(in)
            .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface d, int w) {
                    String n = in.getText().toString().trim();
                    if (!FileManager.valid(n) || !fm.rename(f, n)) toast("Impossible de renommer");
                    fm.enterSame();
                    pushFiles();
                }
            }).setNegativeButton("Annuler", null).show();
    }

    private void askDelete(final File f) {
        new AlertDialog.Builder(act).setTitle("Supprimer ?").setMessage("« " + f.getName() + " » sera supprimé définitivement.")
            .setPositiveButton("Supprimer", new DialogInterface.OnClickListener() {
                @Override public void onClick(DialogInterface d, int w) {
                    new Thread(new Runnable() {
                        @Override public void run() {
                            final boolean ok = fm.delete(f);
                            post(new Runnable() { @Override public void run() { if (!ok) toast("Suppression incomplète"); pushFiles(); } });
                        }
                    }).start();
                }
            }).setNegativeButton("Annuler", null).show();
    }

    private void doPaste() {
        toast("Collage en cours…");
        new Thread(new Runnable() {
            @Override public void run() {
                final String err = fm.paste();
                post(new Runnable() { @Override public void run() { toast(err == null ? "Terminé" : err); pushFiles(); } });
            }
        }).start();
    }

    // ------------------------------------------------------------------ Reglages
    private void pushInfo() {
        nativeSetInfo(DeviceInfo.collect(act));
        try {
            AudioManager am = (AudioManager) act.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            if (max > 0) nativeSetVolume(am.getStreamVolume(AudioManager.STREAM_MUSIC) * 100 / max);
        } catch (Exception ignored) { }
        invalidate();
    }

    private void setVolume(int pct) {
        try {
            AudioManager am = (AudioManager) act.getSystemService(Context.AUDIO_SERVICE);
            int max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            am.setStreamVolume(AudioManager.STREAM_MUSIC, Math.round(pct * max / 100f), 0);
        } catch (Exception e) { toast("Volume non modifiable"); }
    }

    private void openSystemSettings(int k) {
        String[] actions = {
            Settings.ACTION_WIFI_SETTINGS, Settings.ACTION_BLUETOOTH_SETTINGS, Settings.ACTION_DISPLAY_SETTINGS,
            Settings.ACTION_SOUND_SETTINGS, Settings.ACTION_INTERNAL_STORAGE_SETTINGS, Intent.ACTION_POWER_USAGE_SUMMARY,
            Settings.ACTION_APPLICATION_SETTINGS, Settings.ACTION_DATE_SETTINGS, Settings.ACTION_DEVICE_INFO_SETTINGS, Settings.ACTION_SETTINGS
        };
        if (k < 0 || k >= actions.length) return;
        try {
            Intent i = new Intent(actions[k]);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            act.startActivity(i);
        } catch (Exception e) { toast("Réglage introuvable sur cet appareil"); }
    }

    // ------------------------------------------------------------------ applications Android
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
            case 7: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_CALENDAR); break;
            case 8: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_EMAIL); break;
            case 9: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MUSIC); break;
            case 12: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MAPS); break;
            case 14: i = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN, Intent.CATEGORY_APP_MARKET); break;
            case 15: i = new Intent(Intent.ACTION_ASSIST); break;
            default: return;
        }
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try { getContext().startActivity(i); }
        catch (Exception ex) { toast("Application introuvable"); }
    }
}
