package org.recvx.vr;

import org.libsdl.app.SDLActivity;

/* The game's activity: SDL's, with the game library loaded by librecvxloader.so at the PS2 address window
   (android/loader.c) and its own main(). Its save folder (recvx.ini, the log, saves; the disc image goes there too)
   is the app's folder on the shared storage: Android/data/org.recvx.vr/files. */
public class RecvxActivity extends SDLActivity {
    /* the disc image picked in the system's file selector (SDL's dialog): keep the right to read it, so recvx.ini's
       content:// path still opens after the app is closed */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, android.content.Intent data) {
        if (data != null && data.getData() != null) {
            try {
                getContentResolver().takePersistableUriPermission(data.getData(), android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } catch (Exception e) {
                android.util.Log.w("recvx", "cannot keep the permission for " + data.getData() + ": " + e);
            }
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "recvxloader" };
    }

    @Override
    protected String getMainSharedObject() {
        return getContext().getApplicationInfo().nativeLibraryDir + "/librecvx.so";
    }

    @Override
    protected String getMainFunction() {
        return "main";
    }

    @Override
    protected String[] getArguments() {
        java.io.File dir = getExternalFilesDir(null);
        String save = dir != null ? dir.getAbsolutePath() : getFilesDir().getAbsolutePath();
        return new String[] { "--save-dir", save, "--log=" + save + "/recvx.log", "--no-launcher", "--vr" };
    }
}
