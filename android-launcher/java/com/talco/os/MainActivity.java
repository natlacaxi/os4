package com.talco.os;

import android.app.Activity;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.View;

public class MainActivity extends Activity {
    private TalcoView view;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        // permet d'ouvrir un fichier avec une autre application (chemin file://) depuis le gestionnaire de fichiers
        StrictMode.setVmPolicy(new StrictMode.VmPolicy.Builder().build());
        view = new TalcoView(this);
        setContentView(view);
    }

    @Override protected void onResume() {
        super.onResume();
        view.refresh();
    }

    @Override public void onWindowFocusChanged(boolean focus) {
        super.onWindowFocusChanged(focus);
        if (focus) view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    /** Retour : remonte d'un dossier ou ferme l'app TALCO ouverte ; sur l'ecran d'accueil on reste (comportement d'un launcher). */
    @Override public void onBackPressed() { view.back(); }
}
