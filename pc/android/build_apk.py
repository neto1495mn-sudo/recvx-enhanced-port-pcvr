"""Android (Quest): packs build-android's librecvx.so and librecvxloader.so with SDL's Java activity into an APK.

    python pc/android/build_apk.py [build-android]

Needs the Android SDK (ANDROID_HOME or %LOCALAPPDATA%/Android/Sdk: build-tools, platforms/android-34, the NDK for
llvm-strip) and a JDK (javac). Signs with the debug key (~/.android/debug.keystore). Output: <build>/recvx-quest.apk.
"""
import glob, os, shutil, subprocess, sys, zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'build-android'))
SDK = os.environ.get('ANDROID_HOME') or os.path.join(os.environ['LOCALAPPDATA'], 'Android', 'Sdk')
BT = sorted(glob.glob(os.path.join(SDK, 'build-tools', '*')))[-1]
JAR = os.path.join(SDK, 'platforms', 'android-34', 'android.jar')
NDK = sorted(glob.glob(os.path.join(SDK, 'ndk', '*')))[-1]
STRIP = glob.glob(os.path.join(NDK, 'toolchains', 'llvm', 'prebuilt', '*', 'bin', 'llvm-strip*'))[0]
SDL_JAVA = os.path.join(BUILD, '_deps', 'sdl3-src', 'android-project', 'app', 'src', 'main', 'java')
HERE = os.path.dirname(os.path.abspath(__file__))
WIN = os.name == 'nt'

def run(*cmd):
    print('>', ' '.join(os.path.basename(c) if i == 0 else c for i, c in enumerate(cmd)))
    subprocess.run(list(cmd), check=True, shell=False)

def tool(name):
    for ext in (('.bat', '.exe', '') if WIN else ('',)):
        p = os.path.join(BT, name + ext)
        if os.path.exists(p):
            return p
    raise SystemExit('missing build tool ' + name)

out = os.path.join(BUILD, 'apk')
shutil.rmtree(out, ignore_errors=True)
os.makedirs(os.path.join(out, 'classes'))
os.makedirs(os.path.join(out, 'lib', 'armeabi-v7a'))

# Java: SDL's activity and ours, to dex
srcs = glob.glob(os.path.join(SDL_JAVA, '**', '*.java'), recursive=True) + glob.glob(os.path.join(HERE, 'java', '**', '*.java'), recursive=True)
run('javac', '-nowarn', '-source', '11', '-target', '11', '-classpath', JAR, '-d', os.path.join(out, 'classes'), *srcs)
classes = glob.glob(os.path.join(out, 'classes', '**', '*.class'), recursive=True)
run(tool('d8'), '--release', '--min-api', '29', '--lib', JAR, '--output', out, *classes)

# native: stripped
for so in ('librecvx.so', 'librecvxloader.so'):
    run(STRIP, '--strip-debug', '-o', os.path.join(out, 'lib', 'armeabi-v7a', so), os.path.join(BUILD, 'pc', so))

# manifest -> base, then the dex and the libraries (stored, page-aligned by zipalign -p)
base = os.path.join(out, 'base.apk')
run(tool('aapt2'), 'link', '-o', base, '--manifest', os.path.join(HERE, 'AndroidManifest.xml'), '-I', JAR)
with zipfile.ZipFile(base, 'a') as z:
    z.write(os.path.join(out, 'classes.dex'), 'classes.dex', compress_type=zipfile.ZIP_DEFLATED)
    for so in ('librecvx.so', 'librecvxloader.so'):
        z.write(os.path.join(out, 'lib', 'armeabi-v7a', so), 'lib/armeabi-v7a/' + so, compress_type=zipfile.ZIP_DEFLATED)
aligned = os.path.join(out, 'aligned.apk')
run(tool('zipalign'), '-f', '-p', '4', base, aligned)
apk = os.path.join(BUILD, 'recvx-quest.apk')
ks = os.path.join(os.path.expanduser('~'), '.android', 'debug.keystore')
run(tool('apksigner'), 'sign', '--ks', ks, '--ks-pass', 'pass:android', '--out', apk, aligned)
print('apk:', apk, os.path.getsize(apk) // 1024, 'KB')
