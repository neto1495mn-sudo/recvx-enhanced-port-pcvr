package org.recvx.vr;

import org.libsdl.app.SDLActivity;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInstaller;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/* The game's activity: SDL's, with the game library loaded by librecvxloader.so at the PS2 address window
   (android/loader.c) and its own main(). Its save folder (recvx.ini, the log, saves; the disc image goes there too)
   is the app's folder on the shared storage: Android/data/org.recvx.vr/files.
   The static methods below are what the headset launcher (host/android_launcher.c, through JNI) uses: the "all files"
   permission for its own file browser, and the update from GitHub (check, download, install). */
public class RecvxActivity extends SDLActivity {
    private static final String RELEASES = "https://api.github.com/repos/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases?per_page=20";

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
        return new String[] { "--save-dir", save, "--log=" + save + "/recvx.log", "--vr" };
    }

    /* ---- the "all files" permission (the launcher's own file browser reads Download and the like) ---- */
    public static boolean hasAllFiles() {
        return Build.VERSION.SDK_INT < 30 || Environment.isExternalStorageManager();
    }

    public static void requestAllFiles() {
        Context c = getContext();
        try {
            Intent i = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + c.getPackageName()));
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            c.startActivity(i);
        } catch (Exception e) {
            try {
                Intent i = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                c.startActivity(i);
            } catch (Exception e2) {
                android.util.Log.w("recvx", "cannot open the all-files settings: " + e2);
            }
        }
    }

    /* ---- the update: 0 idle, 1 checking, 2 up to date (or no answer), 3 newer one found, 4 downloading, 5 downloaded, 6 failed ---- */
    private static volatile int updState;
    private static volatile int updPct;
    private static volatile String updTag = "";
    private static volatile String updUrl = "";

    public static int updateState() { return updState; }
    public static int updatePct() { return updPct; }
    public static String updateTag() { return updTag; }

    private static int[] version(String s) {
        Matcher m = Pattern.compile("(\\d+)\\.(\\d+)(?:\\.(\\d+))?").matcher(s);
        if (!m.find()) return null;
        return new int[] { Integer.parseInt(m.group(1)), Integer.parseInt(m.group(2)), m.group(3) != null ? Integer.parseInt(m.group(3)) : 0 };
    }

    private static boolean newer(int[] a, int[] b) {
        for (int i = 0; i < 3; i++) if (a[i] != b[i]) return a[i] > b[i];
        return false;
    }

    /* looks at GitHub's releases for one with an .apk, newer than cur ("1.4.3"); one HTTPS request, nothing about the player sent */
    public static void updateCheck(final String cur) {
        if (updState == 1 || updState == 4) return;
        updState = 1;
        new Thread(new Runnable() {
            public void run() {
                try {
                    HttpURLConnection h = (HttpURLConnection) new URL(RELEASES).openConnection();
                    h.setConnectTimeout(5000);
                    h.setReadTimeout(8000);
                    h.setRequestProperty("Accept", "application/vnd.github+json");
                    h.setRequestProperty("User-Agent", "recvx-quest-update-check");
                    if (h.getResponseCode() != 200) { updState = 2; return; }
                    InputStream in = h.getInputStream();
                    java.io.ByteArrayOutputStream bo = new java.io.ByteArrayOutputStream();
                    byte[] b = new byte[8192];
                    int n;
                    while ((n = in.read(b)) > 0 && bo.size() < (4 << 20)) bo.write(b, 0, n);
                    in.close();
                    org.json.JSONArray arr = new org.json.JSONArray(bo.toString("UTF-8"));
                    int[] best = version(cur), got = null;
                    String bu = "";
                    for (int i = 0; i < arr.length(); i++) {
                        org.json.JSONObject r = arr.getJSONObject(i);
                        if (r.optBoolean("draft") || r.optBoolean("prerelease")) continue;
                        org.json.JSONArray as = r.optJSONArray("assets");
                        if (as == null) continue;
                        String url = null;
                        for (int k = 0; k < as.length(); k++) {
                            org.json.JSONObject a = as.getJSONObject(k);
                            if (a.optString("name").toLowerCase().endsWith(".apk")) { url = a.optString("browser_download_url"); break; }
                        }
                        int[] v = url != null ? version(r.optString("tag_name")) : null;
                        if (v != null && best != null && newer(v, got != null ? got : best)) { got = v; bu = url; }
                    }
                    if (got != null) { updTag = got[0] + "." + got[1] + "." + got[2]; updUrl = bu; updState = 3; } else updState = 2;
                } catch (Exception e) {
                    android.util.Log.w("recvx", "update check: " + e);
                    updState = 2;
                }
            }
        }).start();
    }

    public static boolean canInstall() {
        return Build.VERSION.SDK_INT < 26 || getContext().getPackageManager().canRequestPackageInstalls();
    }

    public static void requestInstall() {
        Context c = getContext();
        try {
            Intent i = new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES, Uri.parse("package:" + c.getPackageName()));
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            c.startActivity(i);
        } catch (Exception e) {
            android.util.Log.w("recvx", "cannot open the install-permission settings: " + e);
        }
    }

    /* downloads the newer APK to the app's folder, then installs it (the system asks the player to confirm) */
    public static void updateDownload() {
        if (updState != 3 && updState != 6) return;
        updState = 4;
        updPct = 0;
        new Thread(new Runnable() {
            public void run() {
                try {
                    Context c = getContext();
                    File dir = c.getExternalFilesDir(null);
                    File f = new File(dir != null ? dir : c.getFilesDir(), "update.apk");
                    HttpURLConnection h = (HttpURLConnection) new URL(updUrl).openConnection();
                    h.setConnectTimeout(8000);
                    h.setReadTimeout(15000);
                    h.setInstanceFollowRedirects(true);
                    h.setRequestProperty("User-Agent", "recvx-quest-update");
                    if (h.getResponseCode() != 200) { updState = 6; return; }
                    long total = h.getContentLengthLong(), got = 0;
                    InputStream in = h.getInputStream();
                    OutputStream out = new FileOutputStream(f);
                    byte[] b = new byte[65536];
                    int n;
                    while ((n = in.read(b)) > 0) {
                        out.write(b, 0, n);
                        got += n;
                        if (total > 0) updPct = (int) (got * 100 / total);
                    }
                    out.close();
                    in.close();
                    updPct = 100;
                    updState = 5;
                    install(f);
                } catch (Exception e) {
                    android.util.Log.w("recvx", "update download: " + e);
                    updState = 6;
                }
            }
        }).start();
    }

    private static void install(File f) {
        try {
            final Context c = getContext();
            PackageInstaller pi = c.getPackageManager().getPackageInstaller();
            PackageInstaller.SessionParams sp = new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
            int id = pi.createSession(sp);
            PackageInstaller.Session s = pi.openSession(id);
            OutputStream o = s.openWrite("update", 0, f.length());
            InputStream in = new FileInputStream(f);
            byte[] b = new byte[65536];
            int n;
            while ((n = in.read(b)) > 0) o.write(b, 0, n);
            s.fsync(o);
            in.close();
            o.close();
            String action = "org.recvx.vr.INSTALL_DONE";
            c.registerReceiver(new BroadcastReceiver() {
                public void onReceive(Context ctx, Intent i) {
                    int st = i.getIntExtra(PackageInstaller.EXTRA_STATUS, -1);
                    if (st == PackageInstaller.STATUS_PENDING_USER_ACTION) {
                        Intent a = (Intent) i.getParcelableExtra(Intent.EXTRA_INTENT);
                        if (a != null) { a.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK); ctx.startActivity(a); }
                    } else if (st != PackageInstaller.STATUS_SUCCESS) {
                        android.util.Log.w("recvx", "install: status " + st + " " + i.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE));
                        updState = 6;
                    }
                }
            }, new IntentFilter(action));
            PendingIntent pend = PendingIntent.getBroadcast(c, id, new Intent(action).setPackage(c.getPackageName()), PendingIntent.FLAG_MUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
            s.commit(pend.getIntentSender());
            s.close();
        } catch (Exception e) {
            android.util.Log.w("recvx", "install: " + e);
            updState = 6;
        }
    }
}
