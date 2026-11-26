package com.talco.os;

import android.content.Context;
import android.os.Environment;
import android.os.StatFs;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

/** Etat et operations du gestionnaire de fichiers (volumes, dossier courant, copier/deplacer/supprimer). */
class FileManager {
    static class Vol {
        String label;
        File root;
    }

    private final Context ctx;
    final List<Vol> vols = new ArrayList<Vol>();
    int volIdx = 0;
    File dir;
    File[] items = new File[0];
    String error;
    File clip;
    boolean clipMove;

    FileManager(Context c) { ctx = c; }

    /** Stockage interne + cartes amovibles (deduites des dossiers specifiques a l'application). */
    static List<Vol> volumes(Context c) {
        List<Vol> out = new ArrayList<Vol>();
        Vol in = new Vol();
        in.label = "Interne";
        in.root = Environment.getExternalStorageDirectory();
        out.add(in);
        File[] dirs = c.getExternalFilesDirs(null);
        if (dirs != null) {
            for (int i = 1; i < dirs.length; i++) {
                if (dirs[i] == null) continue;
                String p = dirs[i].getAbsolutePath();
                int k = p.indexOf("/Android/data");
                if (k <= 0) continue;
                Vol v = new Vol();
                v.root = new File(p.substring(0, k));
                v.label = Environment.isExternalStorageRemovable(dirs[i]) ? "Carte SD" : "Stockage " + (i + 1);
                out.add(v);
            }
        }
        return out;
    }

    void enter() {
        vols.clear();
        vols.addAll(volumes(ctx));
        if (volIdx >= vols.size()) volIdx = 0;
        dir = vols.get(volIdx).root;
        list();
    }

    /** Recharge le dossier courant. */
    void enterSame() { if (dir != null) list(); }

    private void list() {
        error = null;
        File[] f = dir.listFiles();
        if (f == null) {
            f = new File[0];
            error = "Impossible de lire ce dossier (accès refusé ?)";
        }
        Arrays.sort(f, new Comparator<File>() {
            @Override public int compare(File a, File b) {
                if (a.isDirectory() != b.isDirectory()) return a.isDirectory() ? -1 : 1;
                return a.getName().compareToIgnoreCase(b.getName());
            }
        });
        items = f;
    }

    void selectVolume(int i) {
        if (i < 0 || i >= vols.size()) return;
        volIdx = i;
        dir = vols.get(i).root;
        list();
    }

    boolean canUp() { return dir != null && !dir.equals(vols.get(volIdx).root) && dir.getParentFile() != null; }

    void up() {
        if (canUp()) { dir = dir.getParentFile(); list(); }
    }

    File item(int i) { return (i >= 0 && i < items.length) ? items[i] : null; }

    /** Entre dans un dossier (retourne null) ou retourne le fichier a ouvrir. */
    File open(int i) {
        File f = item(i);
        if (f == null) return null;
        if (f.isDirectory()) { dir = f; list(); return null; }
        return f;
    }

    String pathLabel() {
        String p = dir.getAbsolutePath(), r = vols.get(volIdx).root.getAbsolutePath();
        String rel = p.startsWith(r) ? p.substring(r.length()) : p;
        return rel.length() == 0 ? "/" : rel;
    }

    private static String clean(String s) { return s.replace('\t', ' ').replace('\n', ' ').replace('\r', ' '); }

    String dirData() {
        if (error != null) return "!" + error;
        StringBuilder b = new StringBuilder();
        for (File f : items) {
            b.append(clean(f.getName())).append('\t').append(f.isDirectory() ? 0 : f.length()).append('\t')
             .append(f.lastModified() / 1000).append('\t').append(f.isDirectory() ? 1 : 0).append('\n');
        }
        return b.toString();
    }

    String volumesData() {
        StringBuilder b = new StringBuilder();
        for (Vol v : vols) {
            long total = 0, free = 0;
            try {
                StatFs s = new StatFs(v.root.getAbsolutePath());
                total = s.getTotalBytes();
                free = s.getAvailableBytes();
            } catch (Exception ignored) { }
            b.append(v.label).append('\t').append(total).append('\t').append(free).append('\n');
        }
        return b.toString();
    }

    static boolean valid(String n) {
        return n != null && n.length() > 0 && n.indexOf('/') < 0 && !n.equals(".") && !n.equals("..");
    }

    boolean mkdir(String name) { return valid(name) && new File(dir, name).mkdir(); }

    boolean rename(File f, String name) { return valid(name) && f.renameTo(new File(f.getParentFile(), name)); }

    static boolean deleteRec(File f) {
        if (f.isDirectory()) {
            File[] c = f.listFiles();
            if (c != null) for (File x : c) deleteRec(x);
        }
        return f.delete();
    }

    boolean delete(File f) {
        boolean ok = deleteRec(f);
        list();
        return ok;
    }

    void setClip(File f, boolean move) { clip = f; clipMove = move; }

    private static void copyRec(File src, File dst) throws IOException {
        if (src.isDirectory()) {
            if (!dst.exists() && !dst.mkdirs()) throw new IOException("mkdir");
            File[] c = src.listFiles();
            if (c != null) for (File x : c) copyRec(x, new File(dst, x.getName()));
        } else {
            InputStream in = new FileInputStream(src);
            try {
                OutputStream out = new FileOutputStream(dst);
                try {
                    byte[] buf = new byte[65536];
                    int n;
                    while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                } finally { out.close(); }
            } finally { in.close(); }
        }
    }

    /** Colle le fichier copie/coupe dans le dossier courant. Retourne un message d'erreur, ou null si tout va bien. A appeler hors du fil principal. */
    String paste() {
        File src = clip;
        if (src == null) return "Rien à coller";
        File dst = new File(dir, src.getName());
        try {
            if (dst.getCanonicalPath().startsWith(src.getCanonicalPath() + File.separator) || dst.getCanonicalPath().equals(src.getCanonicalPath()))
                return "Impossible de coller un dossier dans lui-même";
            if (dst.exists()) return "Un élément du même nom existe déjà";
            if (clipMove) {
                if (!src.renameTo(dst)) { copyRec(src, dst); deleteRec(src); }
                clip = null;
            } else {
                copyRec(src, dst);
            }
        } catch (IOException e) {
            return "Échec : " + e.getMessage();
        }
        list();
        return null;
    }
}
