package org.recvx.vr;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.io.File;
import java.io.FileInputStream;
import java.util.Locale;

/* Shown when the game library could not be loaded (RecvxActivity.loadLibraries): a plain 2D panel in its own process (the game's dies),
   with the reason android/loader.c wrote, so the player sees why instead of an app that only restarts. */
public class RecvxErrorActivity extends Activity {
    private String reason() {
        try {
            File f = new File(getFilesDir(), "start_error.txt");
            byte[] b = new byte[Math.min((int) f.length(), 1024)];
            FileInputStream in = new FileInputStream(f);
            int n = in.read(b);
            in.close();
            if (n > 0) return new String(b, 0, n, "UTF-8").trim();
        } catch (Exception e) {
            android.util.Log.w("recvx", "no start error file: " + e);
        }
        String extra = getIntent().getStringExtra("reason");
        return extra != null ? extra : "?";
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        String lang = Locale.getDefault().getLanguage();
        String title = lang.equals("pt") ? "O jogo não conseguiu abrir neste aparelho."
                     : lang.equals("es") ? "El juego no pudo abrirse en este dispositivo."
                     : "The game could not start on this device.";
        String ask = lang.equals("pt") ? "Tire uma foto desta mensagem e envie na página do projeto no GitHub."
                   : lang.equals("es") ? "Haz una captura de este mensaje y envíala a la página del proyecto en GitHub."
                   : "Take a screenshot of this message and send it to the project's GitHub page.";
        String close = lang.equals("pt") ? "Fechar" : lang.equals("es") ? "Cerrar" : "Close";

        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setGravity(Gravity.CENTER);
        box.setPadding(60, 60, 60, 60);
        box.setBackgroundColor(Color.rgb(24, 24, 28));
        TextView a = new TextView(this);
        a.setText(title);
        a.setTextSize(24);
        a.setTextColor(Color.WHITE);
        TextView r = new TextView(this);
        r.setText(reason());
        r.setTextSize(16);
        r.setTextColor(Color.rgb(224, 120, 120));
        r.setPadding(0, 30, 0, 30);
        TextView s = new TextView(this);
        s.setText(ask);
        s.setTextSize(16);
        s.setTextColor(Color.LTGRAY);
        s.setPadding(0, 0, 0, 30);
        Button b = new Button(this);
        b.setText(close);
        b.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { finish(); }
        });
        box.addView(a);
        box.addView(r);
        box.addView(s);
        box.addView(b);
        setContentView(box);
    }
}
