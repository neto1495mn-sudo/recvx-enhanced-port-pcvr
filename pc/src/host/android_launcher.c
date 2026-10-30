/*
 * Quest: what the headset launcher asks of Android (overlay.cpp overlay_launcher_xr) - the "all files" permission, the update
 * from GitHub - through the static methods of org.recvx.vr.RecvxActivity (android/java). SDL's JNI environment, the activity's
 * own class (FindClass from a native thread would not see the app's classes).
 */
#ifdef __ANDROID__
#include <SDL3/SDL.h>
#include <jni.h>
#include <string.h>

static jclass act_class(JNIEnv* env, jobject* act)
{
    *act = (jobject)SDL_GetAndroidActivity();
    return *act ? (*env)->GetObjectClass(env, *act) : NULL;
}

static int call_int(const char* name, const char* sig, int def, const char* arg)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject act;
    jclass c;
    jmethodID m;
    int r = def;
    if (!env || !(c = act_class(env, &act))) return def;
    m = (*env)->GetStaticMethodID(env, c, name, sig);
    if (m) {
        if (arg) {
            jstring js = (*env)->NewStringUTF(env, arg);
            (*env)->CallStaticVoidMethod(env, c, m, js);
            (*env)->DeleteLocalRef(env, js);
            r = 0;
        } else if (sig[2] == 'Z') r = (*env)->CallStaticBooleanMethod(env, c, m);
        else if (sig[2] == 'I') r = (*env)->CallStaticIntMethod(env, c, m);
        else { (*env)->CallStaticVoidMethod(env, c, m); r = 0; }
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, c);
    (*env)->DeleteLocalRef(env, act);
    return r;
}

int and_all_files(void) { return call_int("hasAllFiles", "()Z", 0, NULL); }
void and_all_files_request(void) { call_int("requestAllFiles", "()V", 0, NULL); }
int and_can_install(void) { return call_int("canInstall", "()Z", 0, NULL); }
void and_request_install(void) { call_int("requestInstall", "()V", 0, NULL); }
void and_update_check(const char* cur) { call_int("updateCheck", "(Ljava/lang/String;)V", 0, cur); }
int and_update_state(void) { return call_int("updateState", "()I", 0, NULL); }
int and_update_pct(void) { return call_int("updatePct", "()I", 0, NULL); }
void and_update_download(void) { call_int("updateDownload", "()V", 0, NULL); }

void and_update_tag(char* out, int n)
{
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject act;
    jclass c;
    jmethodID m;
    out[0] = 0;
    if (!env || !(c = act_class(env, &act))) return;
    m = (*env)->GetStaticMethodID(env, c, "updateTag", "()Ljava/lang/String;");
    if (m) {
        jstring js = (jstring)(*env)->CallStaticObjectMethod(env, c, m);
        if (js) {
            const char* s = (*env)->GetStringUTFChars(env, js, NULL);
            if (s) { SDL_strlcpy(out, s, n); (*env)->ReleaseStringUTFChars(env, js, s); }
            (*env)->DeleteLocalRef(env, js);
        }
    }
    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    (*env)->DeleteLocalRef(env, c);
    (*env)->DeleteLocalRef(env, act);
}
#endif
