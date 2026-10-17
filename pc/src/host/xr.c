/*
 * VR output through OpenXR (see xr.h).
 *
 * Render thread: one session; each present is one headset frame:
 *   xrPollEvent -> xrWaitFrame (paces to the headset) -> xrBeginFrame -> xrLocateViews
 *   -> stereo: the renderer draws each eye into its swapchain image (xr_eye_target), submitted
 *      as a projection layer; otherwise the window's back buffer goes onto a quad layer.
 * Copies are framebuffer blits with GL_FRAMEBUFFER_SRGB off, so the bytes the game drew reach
 * the compositor unchanged (an sRGB swapchain then shows them as the window does).
 *
 * The head pose of every frame is published (spin lock) for the game thread's VR camera.
 */
/* no ps2_platform.h here: the PS2 SDK names (CreateThread, GetThreadId, ...) clash with windows.h */
#include "xr.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <string.h>

void host_log(const char* fmt, ...);
int  render_vsync_setting(void);   /* g_config.vsync (render.c) */

int g_vr_request;
int g_vr_screen_dist = 250;
int g_vr_screen_width = 400;
int g_vr_stereo = 1;
int g_vr_res = 100;       /* ini vr_res: eye resolution, percent of the headset's recommended size */
static int rec_eye_h, rec_eye_w;     /* the headset's recommended eye size (pixels) */
int xr_recommended_eye_height(void) { return rec_eye_h; }
int xr_recommended_eye_width(void) { return rec_eye_w; }
int g_vr_screen_angle = 60;   /* ini vr_screen_angle: the cinema screen's width as seen from the head (degrees) */

/* recentring (shared): recentred = Ry(-yaw0) * (local - origin) */
static SDL_SpinLock pose_lock;
static VrPose head_rec;           /* latest head pose, recentred */
static int head_valid;
static float rec_yaw, rec_ox, rec_oy, rec_oz;
static SDL_AtomicInt stereo_ready;
static VrInput input_now;         /* latest controller state */
static int input_valid;
static SDL_AtomicInt rec_gen;

unsigned xr_recenter_gen(void) { return (unsigned)SDL_GetAtomicInt(&rec_gen); }

int xr_input(VrInput* out)
{
    int ok;
    SDL_LockSpinlock(&pose_lock);
    ok = input_valid;
    if (ok) *out = input_now;
    SDL_UnlockSpinlock(&pose_lock);
    if (!ok) memset(out, 0, sizeof(*out));
    return ok;
}

int xr_stereo_ready(void) { return SDL_GetAtomicInt(&stereo_ready); }

int xr_head_pose(VrPose* out)
{
    int ok;
    SDL_LockSpinlock(&pose_lock);
    ok = head_valid;
    if (ok) *out = head_rec;
    SDL_UnlockSpinlock(&pose_lock);
    return ok;
}

void xr_recenter_matrix(float m[16])
{
    float c, s, ox, oy, oz;
    SDL_LockSpinlock(&pose_lock);
    c = cosf(-rec_yaw); s = sinf(-rec_yaw); ox = rec_ox; oy = rec_oy; oz = rec_oz;
    SDL_UnlockSpinlock(&pose_lock);
    /* Ry(-yaw0) * T(-origin), column-major */
    vr_mat_identity(m);
    m[0] = c;  m[8] = s;
    m[2] = -s; m[10] = c;
    m[12] = -(c * ox + s * oz);
    m[13] = -oy;
    m[14] = -(-s * ox + c * oz);
}

#if !defined(_WIN32) || !defined(RECVX_HAVE_OPENXR)

int    xr_init(void) { if (g_vr_request) host_log("vr: not available on this platform yet\n"); return 0; }
int    xr_active(void) { return 0; }
double xr_display_hz(void) { return 0.0; }
void   xr_recenter(void) {}
void   xr_shutdown(void) {}
int    xr_frame_begin(void) { return -1; }
const VrEye* xr_frame_eye(int e) { (void)e; return NULL; }
unsigned int xr_eye_target(int e, int w, int h) { (void)e; (void)w; (void)h; return 0; }
void   xr_eye_release(int e) { (void)e; }
void   xr_frame_end_stereo(void) {}
void   xr_frame_end_quad(int w, int h) { (void)w; (void)h; }
void   xr_frame_end_none(void) {}
int    xr_have_held(void) { return 0; }
void   xr_frame_end_held(void) {}

#else

#include <windows.h>
#include <unknwn.h>
#include "../render/gl_loader.h"
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

typedef struct {
    XrSwapchain h;
    int w, ht;
    uint32_t n;
    XrSwapchainImageOpenGLKHR img[8];
    int acquired;
} Chain;

static XrInstance  inst = XR_NULL_HANDLE;
static XrSystemId  sys = XR_NULL_SYSTEM_ID;
static XrSession   sess = XR_NULL_HANDLE;
static XrSpace     local_space = XR_NULL_HANDLE, view_space = XR_NULL_HANDLE;
static XrSessionState state = XR_SESSION_STATE_UNKNOWN;
static int running;             /* between xrBeginSession and xrEndSession */
static int lost;                /* the session ended for good: back to the desktop */
static double hz;               /* from the predicted display period */
static int64_t chain_fmt;
static GLuint blit_fbo;

static Chain quad_chain, eye_chain[2];
static XrFrameState fs;
static int in_frame;            /* between xrBeginFrame and xrEndFrame */
static XrView views[2];
static XrView held_views[2];   /* the poses of the last stereo frame (xr_frame_end_held) */
static int held_ok;
static VrEye eyes[2];
static int eyes_valid;

static XrPosef screen_pose;     /* the cinema quad's pose in LOCAL space */
static int need_center = 1;     /* recentre on the next frame */

/* controller actions (Touch profile) */
static XrActionSet aset = XR_NULL_HANDLE;
static XrAction a_hapr, a_hapl, a_gripr, a_gripl, a_aiml, a_aim, a_lstick, a_rstick, a_ltrig, a_rtrig, a_lgrip, a_rgrip, a_a, a_b, a_x, a_y, a_menu, a_lclick, a_rclick;
static XrSpace aim_space = XR_NULL_HANDLE, aiml_space = XR_NULL_HANDLE, grip_space = XR_NULL_HANDLE, gripl_space = XR_NULL_HANDLE;
static XrPath p_left, p_right;

static int xr_ok(XrResult r, const char* what)
{
    if (XR_SUCCEEDED(r)) return 1;
    {
        char name[XR_MAX_RESULT_STRING_SIZE] = "";
        if (inst) xrResultToString(inst, r, name);
        host_log("vr: %s failed: %d %s\n", what, (int)r, name);
    }
    return 0;
}

static void chain_destroy(Chain* c)
{
    if (c->h) xrDestroySwapchain(c->h);
    memset(c, 0, sizeof(*c));
}

static int chain_make(Chain* c, int w, int h)
{
    XrSwapchainCreateInfo ci = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    chain_destroy(c);
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = chain_fmt;
    ci.sampleCount = 1;
    ci.width = (uint32_t)w;
    ci.height = (uint32_t)h;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (!xr_ok(xrCreateSwapchain(sess, &ci, &c->h), "xrCreateSwapchain")) return 0;
    for (uint32_t i = 0; i < 8; i++) c->img[i] = (XrSwapchainImageOpenGLKHR){ XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR };
    if (!xr_ok(xrEnumerateSwapchainImages(c->h, 8, &c->n, (XrSwapchainImageBaseHeader*)c->img), "xrEnumerateSwapchainImages")) {
        chain_destroy(c);
        return 0;
    }
    c->w = w; c->ht = h;
    host_log("vr: swapchain %dx%d, %u images, format 0x%llx\n", w, h, c->n, (unsigned long long)chain_fmt);
    return 1;
}

/* acquire the next image of c (made w x h first if needed): its GL texture, 0 on failure */
static GLuint chain_acquire(Chain* c, int w, int h)
{
    uint32_t idx = 0;
    XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    if ((w != c->w || h != c->ht || !c->h) && !chain_make(c, w, h)) return 0;
    if (!xr_ok(xrAcquireSwapchainImage(c->h, &ai, &idx), "xrAcquireSwapchainImage")) return 0;
    wi.timeout = XR_INFINITE_DURATION;
    if (!xr_ok(xrWaitSwapchainImage(c->h, &wi), "xrWaitSwapchainImage")) {
        XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xrReleaseSwapchainImage(c->h, &ri);
        return 0;
    }
    c->acquired = 1;
    return c->img[idx].image;
}

static void chain_release(Chain* c)
{
    XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    if (!c->acquired) return;
    xrReleaseSwapchainImage(c->h, &ri);
    c->acquired = 0;
}

static int pick_format(void)
{
    int64_t fmts[64];
    uint32_t n = 0;
    if (!xr_ok(xrEnumerateSwapchainFormats(sess, 64, &n, fmts), "xrEnumerateSwapchainFormats")) return 0;
    chain_fmt = 0;
    for (uint32_t i = 0; i < n && !chain_fmt; i++) if (fmts[i] == GL_SRGB8_ALPHA8) chain_fmt = fmts[i];
    for (uint32_t i = 0; i < n && !chain_fmt; i++) if (fmts[i] == GL_RGBA8) chain_fmt = fmts[i];
    if (!chain_fmt && n) chain_fmt = fmts[0];
    return chain_fmt != 0;
}


/* ---- controllers ---- */

static XrAction mk_action(const char* name, const char* loc, XrActionType type)
{
    XrActionCreateInfo ci = { XR_TYPE_ACTION_CREATE_INFO };
    XrPath subs[2] = { p_left, p_right };
    XrAction a = XR_NULL_HANDLE;
    strcpy(ci.actionName, name);
    strcpy(ci.localizedActionName, loc);
    ci.actionType = type;
    ci.countSubactionPaths = 0;
    (void)subs;
    xr_ok(xrCreateAction(aset, &ci, &a), "xrCreateAction");
    return a;
}

static XrPath path_of(const char* s) { XrPath p = XR_NULL_PATH; xrStringToPath(inst, s, &p); return p; }

/* Touch controllers: left stick moves, right stick turns, triggers/grips/buttons as in the README */
static void make_actions(void)
{
    XrActionSetCreateInfo si = { XR_TYPE_ACTION_SET_CREATE_INFO };
    XrInteractionProfileSuggestedBinding sb = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    XrActionSuggestedBinding b[24];
    XrSessionActionSetsAttachInfo ai = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    XrActionSpaceCreateInfo sci = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
    int n = 0;
    p_left = path_of("/user/hand/left"); p_right = path_of("/user/hand/right");
    strcpy(si.actionSetName, "gameplay");
    strcpy(si.localizedActionSetName, "Gameplay");
    if (!xr_ok(xrCreateActionSet(inst, &si, &aset), "xrCreateActionSet")) return;
    a_aim = mk_action("aim", "Aim", XR_ACTION_TYPE_POSE_INPUT);
    a_aiml = mk_action("aim_left", "Left hand", XR_ACTION_TYPE_POSE_INPUT);
    a_gripr = mk_action("grip_right", "Right hand grip", XR_ACTION_TYPE_POSE_INPUT);
    a_gripl = mk_action("grip_left", "Left hand grip", XR_ACTION_TYPE_POSE_INPUT);
    a_hapr = mk_action("haptic_right", "Right vibration", XR_ACTION_TYPE_VIBRATION_OUTPUT);
    a_hapl = mk_action("haptic_left", "Left vibration", XR_ACTION_TYPE_VIBRATION_OUTPUT);
    a_lstick = mk_action("move", "Move", XR_ACTION_TYPE_VECTOR2F_INPUT);
    a_rstick = mk_action("turn", "Turn", XR_ACTION_TYPE_VECTOR2F_INPUT);
    a_ltrig = mk_action("run", "Run", XR_ACTION_TYPE_FLOAT_INPUT);
    a_rtrig = mk_action("fire", "Fire", XR_ACTION_TYPE_FLOAT_INPUT);
    a_lgrip = mk_action("knife", "Knife", XR_ACTION_TYPE_FLOAT_INPUT);
    a_rgrip = mk_action("ready", "Ready weapon", XR_ACTION_TYPE_FLOAT_INPUT);
    a_a = mk_action("action", "Action", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_b = mk_action("cancel", "Cancel", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_x = mk_action("map", "Map", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_y = mk_action("inventory", "Inventory", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_menu = mk_action("pause", "Pause", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_lclick = mk_action("lclick", "Left stick click", XR_ACTION_TYPE_BOOLEAN_INPUT);
    a_rclick = mk_action("rclick", "Right stick click", XR_ACTION_TYPE_BOOLEAN_INPUT);
#define BIND(act, str) do { b[n].action = act; b[n].binding = path_of(str); n++; } while (0)
    BIND(a_aim, "/user/hand/right/input/aim/pose");
    BIND(a_aiml, "/user/hand/left/input/aim/pose");
    BIND(a_gripr, "/user/hand/right/input/grip/pose");
    BIND(a_gripl, "/user/hand/left/input/grip/pose");
    BIND(a_hapr, "/user/hand/right/output/haptic");
    BIND(a_hapl, "/user/hand/left/output/haptic");
    BIND(a_lstick, "/user/hand/left/input/thumbstick");
    BIND(a_rstick, "/user/hand/right/input/thumbstick");
    BIND(a_ltrig, "/user/hand/left/input/trigger/value");
    BIND(a_rtrig, "/user/hand/right/input/trigger/value");
    BIND(a_lgrip, "/user/hand/left/input/squeeze/value");
    BIND(a_rgrip, "/user/hand/right/input/squeeze/value");
    BIND(a_a, "/user/hand/right/input/a/click");
    BIND(a_b, "/user/hand/right/input/b/click");
    BIND(a_x, "/user/hand/left/input/x/click");
    BIND(a_y, "/user/hand/left/input/y/click");
    BIND(a_menu, "/user/hand/left/input/menu/click");
    BIND(a_lclick, "/user/hand/left/input/thumbstick/click");
    BIND(a_rclick, "/user/hand/right/input/thumbstick/click");
#undef BIND
    sb.interactionProfile = path_of("/interaction_profiles/oculus/touch_controller");
    sb.suggestedBindings = b;
    sb.countSuggestedBindings = (uint32_t)n;
    xr_ok(xrSuggestInteractionProfileBindings(inst, &sb), "xrSuggestInteractionProfileBindings(touch)");
    ai.countActionSets = 1;
    ai.actionSets = &aset;
    xr_ok(xrAttachSessionActionSets(sess, &ai), "xrAttachSessionActionSets");
    sci.action = a_aim;
    sci.poseInActionSpace.orientation.w = 1.0f;
    xr_ok(xrCreateActionSpace(sess, &sci, &aim_space), "xrCreateActionSpace(aim)");
    sci.action = a_aiml;
    xr_ok(xrCreateActionSpace(sess, &sci, &aiml_space), "xrCreateActionSpace(aim_left)");
    sci.action = a_gripr;
    xr_ok(xrCreateActionSpace(sess, &sci, &grip_space), "xrCreateActionSpace(grip_right)");
    sci.action = a_gripl;
    xr_ok(xrCreateActionSpace(sess, &sci, &gripl_space), "xrCreateActionSpace(grip_left)");
    host_log("vr: controllers: Touch bindings suggested\n");
}

static float get_f(XrAction a) { XrActionStateGetInfo gi = { XR_TYPE_ACTION_STATE_GET_INFO }; XrActionStateFloat st = { XR_TYPE_ACTION_STATE_FLOAT }; gi.action = a; return XR_SUCCEEDED(xrGetActionStateFloat(sess, &gi, &st)) && st.isActive ? st.currentState : 0.0f; }
static int get_b(XrAction a) { XrActionStateGetInfo gi = { XR_TYPE_ACTION_STATE_GET_INFO }; XrActionStateBoolean st = { XR_TYPE_ACTION_STATE_BOOLEAN }; gi.action = a; return XR_SUCCEEDED(xrGetActionStateBoolean(sess, &gi, &st)) && st.isActive && st.currentState; }
static void get_v(XrAction a, float* x, float* y) { XrActionStateGetInfo gi = { XR_TYPE_ACTION_STATE_GET_INFO }; XrActionStateVector2f st = { XR_TYPE_ACTION_STATE_VECTOR2F }; gi.action = a; *x = *y = 0; if (XR_SUCCEEDED(xrGetActionStateVector2f(sess, &gi, &st)) && st.isActive) { *x = st.currentState.x; *y = st.currentState.y; } }

static void to_recentred(const XrPosef* in, VrPose* p)
{
    float m[16], c, s;
    const XrQuaternionf* q = &in->orientation;
    xr_recenter_matrix(m);
    p->px = m[0] * in->position.x + m[4] * in->position.y + m[8] * in->position.z + m[12];
    p->py = m[1] * in->position.x + m[5] * in->position.y + m[9] * in->position.z + m[13];
    p->pz = m[2] * in->position.x + m[6] * in->position.y + m[10] * in->position.z + m[14];
    c = cosf(-rec_yaw * 0.5f); s = sinf(-rec_yaw * 0.5f);   /* Ry(-yaw0) * q */
    p->qw = c * q->w - s * q->y;
    p->qx = c * q->x + s * q->z;
    p->qy = c * q->y + s * q->w;
    p->qz = c * q->z - s * q->x;
}

/* the controllers now (session focused) */
/* vibration requests (any thread), sent with the next action sync */
static SDL_SpinLock hap_lock;
static float hap_amp[2];
static int hap_ms[2];
void xr_haptic(int hand, float amplitude, int ms)
{
    if (hand < 0 || hand > 1) return;
    SDL_LockSpinlock(&hap_lock);
    if (amplitude > hap_amp[hand]) hap_amp[hand] = amplitude;
    if (ms > hap_ms[hand]) hap_ms[hand] = ms;
    SDL_UnlockSpinlock(&hap_lock);
}
static void send_haptics(void)
{
    float amp[2];
    int ms[2];
    SDL_LockSpinlock(&hap_lock);
    amp[0] = hap_amp[0]; amp[1] = hap_amp[1]; ms[0] = hap_ms[0]; ms[1] = hap_ms[1];
    hap_amp[0] = hap_amp[1] = 0; hap_ms[0] = hap_ms[1] = 0;
    SDL_UnlockSpinlock(&hap_lock);
    for (int h = 0; h < 2; h++) {
        XrHapticActionInfo hi = { XR_TYPE_HAPTIC_ACTION_INFO };
        XrHapticVibration v = { XR_TYPE_HAPTIC_VIBRATION };
        if (amp[h] <= 0 || !(h ? a_hapl : a_hapr)) continue;
        hi.action = h ? a_hapl : a_hapr;
        v.amplitude = amp[h] > 1.0f ? 1.0f : amp[h];
        v.duration = (XrDuration)ms[h] * 1000000;
        v.frequency = XR_FREQUENCY_UNSPECIFIED;
        xrApplyHapticFeedback(sess, &hi, (const XrHapticBaseHeader*)&v);
    }
}

static void read_actions(XrTime t)
{
    XrActiveActionSet as = { aset, XR_NULL_PATH };
    XrActionsSyncInfo si = { XR_TYPE_ACTIONS_SYNC_INFO };
    VrInput in;
    if (!aset) return;
    si.countActiveActionSets = 1;
    si.activeActionSets = &as;
    if (state != XR_SESSION_STATE_FOCUSED || !XR_SUCCEEDED(xrSyncActions(sess, &si))) {
        SDL_LockSpinlock(&pose_lock); input_valid = 0; SDL_UnlockSpinlock(&pose_lock);
        return;
    }
    memset(&in, 0, sizeof(in));
    send_haptics();
    get_v(a_lstick, &in.lx, &in.ly);
    get_v(a_rstick, &in.rx, &in.ry);
    in.ltrig = get_f(a_ltrig); in.rtrig = get_f(a_rtrig);
    in.lgrip = get_f(a_lgrip); in.rgrip = get_f(a_rgrip);
    in.buttons = (get_b(a_a) ? VRB_A : 0) | (get_b(a_b) ? VRB_B : 0) | (get_b(a_x) ? VRB_X : 0) | (get_b(a_y) ? VRB_Y : 0) |
                 (get_b(a_menu) ? VRB_MENU : 0) | (get_b(a_lclick) ? VRB_LSTICK : 0) | (get_b(a_rclick) ? VRB_RSTICK : 0);
    for (int hnd = 0; hnd < 2; hnd++) {
        XrSpace sp = hnd ? aiml_space : aim_space;
        XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
        if (sp && XR_SUCCEEDED(xrLocateSpace(sp, local_space, t, &loc)) &&
            (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) && (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
            to_recentred(&loc.pose, hnd ? &in.aiml : &in.aim);
            if (hnd) in.aiml_valid = 1; else in.aim_valid = 1;
        }
    }
    for (int hnd = 0; hnd < 2; hnd++) {   /* the grip poses (the palm): where the hands are */
        XrSpace sp = hnd ? gripl_space : grip_space;
        XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
        if (sp && XR_SUCCEEDED(xrLocateSpace(sp, local_space, t, &loc)) && (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
            to_recentred(&loc.pose, hnd ? &in.gripl : &in.grip);
            if (hnd) in.gripl_valid = 1; else in.grip_valid = 1;
        }
    }
    {   /* diagnosis: log when a controller's pose comes and goes */
        static int last = -1;
        int now = in.aim_valid | (in.aiml_valid << 1) | (in.grip_valid << 2) | (in.gripl_valid << 3);
        if (now != last) { host_log("vr: controller poses: right %s, left %s; grips: right %s, left %s\n", in.aim_valid ? "tracked" : "lost", in.aiml_valid ? "tracked" : "lost", in.grip_valid ? "tracked" : "lost", in.gripl_valid ? "tracked" : "lost"); last = now; }
    }
    SDL_LockSpinlock(&pose_lock);
    input_now = in;
    input_valid = 1;
    SDL_UnlockSpinlock(&pose_lock);
}

int xr_init(void)
{
    static const char* exts[] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    XrInstanceCreateInfo ici = { XR_TYPE_INSTANCE_CREATE_INFO };
    XrSystemGetInfo sgi = { XR_TYPE_SYSTEM_GET_INFO };
    XrSystemProperties props = { XR_TYPE_SYSTEM_PROPERTIES };
    XrInstanceProperties iprops = { XR_TYPE_INSTANCE_PROPERTIES };
    XrGraphicsRequirementsOpenGLKHR req = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    XrGraphicsBindingOpenGLWin32KHR bind = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    XrSessionCreateInfo sci = { XR_TYPE_SESSION_CREATE_INFO };
    XrReferenceSpaceCreateInfo rci = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    PFN_xrGetOpenGLGraphicsRequirementsKHR get_req = NULL;

    if (!g_vr_request) return 0;
    host_log("vr: starting OpenXR\n");

    strcpy(ici.applicationInfo.applicationName, "recvx VR");
    ici.applicationInfo.applicationVersion = 2;
    strcpy(ici.applicationInfo.engineName, "recvx");
    ici.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);   /* 1.0: accepted by every runtime */
    ici.enabledExtensionCount = 1;
    ici.enabledExtensionNames = exts;
    if (!xr_ok(xrCreateInstance(&ici, &inst), "xrCreateInstance (is a VR runtime installed and set as the OpenXR runtime?)"))
        goto fail;
    if (XR_SUCCEEDED(xrGetInstanceProperties(inst, &iprops)))
        host_log("vr: runtime %s %u.%u.%u\n", iprops.runtimeName, XR_VERSION_MAJOR(iprops.runtimeVersion),
                 XR_VERSION_MINOR(iprops.runtimeVersion), XR_VERSION_PATCH(iprops.runtimeVersion));

    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!xr_ok(xrGetSystem(inst, &sgi, &sys), "xrGetSystem (is the headset connected?)")) goto fail;
    if (XR_SUCCEEDED(xrGetSystemProperties(inst, sys, &props))) host_log("vr: headset %s\n", props.systemName);
    {
        XrViewConfigurationView vcv[2] = { { XR_TYPE_VIEW_CONFIGURATION_VIEW }, { XR_TYPE_VIEW_CONFIGURATION_VIEW } };
        uint32_t nv = 0;
        if (XR_SUCCEEDED(xrEnumerateViewConfigurationViews(inst, sys, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &nv, vcv)) && nv == 2)
        {
            host_log("vr: recommended eye size %ux%u\n", vcv[0].recommendedImageRectWidth, vcv[0].recommendedImageRectHeight);
            rec_eye_h = (int)vcv[0].recommendedImageRectHeight;
            rec_eye_w = (int)vcv[0].recommendedImageRectWidth;
        }
    }

    /* required before xrCreateSession */
    if (!xr_ok(xrGetInstanceProcAddr(inst, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&get_req), "xrGetInstanceProcAddr"))
        goto fail;
    if (!xr_ok(get_req(inst, sys, &req), "xrGetOpenGLGraphicsRequirementsKHR")) goto fail;

    {   /* the context SDL made current on this thread */
        HMODULE gl = GetModuleHandleA("opengl32.dll");
        HDC (WINAPI *get_dc)(void) = gl ? (HDC (WINAPI *)(void))GetProcAddress(gl, "wglGetCurrentDC") : NULL;
        HGLRC (WINAPI *get_rc)(void) = gl ? (HGLRC (WINAPI *)(void))GetProcAddress(gl, "wglGetCurrentContext") : NULL;
        bind.hDC = get_dc ? get_dc() : NULL;
        bind.hGLRC = get_rc ? get_rc() : NULL;
        if (!bind.hDC || !bind.hGLRC) { host_log("vr: no current WGL context\n"); goto fail; }
    }
    sci.next = &bind;
    sci.systemId = sys;
    if (!xr_ok(xrCreateSession(inst, &sci, &sess), "xrCreateSession")) goto fail;

    rci.poseInReferenceSpace.orientation.w = 1.0f;
    rci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    if (!xr_ok(xrCreateReferenceSpace(sess, &rci, &local_space), "xrCreateReferenceSpace(LOCAL)")) goto fail;
    rci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    if (!xr_ok(xrCreateReferenceSpace(sess, &rci, &view_space), "xrCreateReferenceSpace(VIEW)")) goto fail;

    if (!pick_format()) goto fail;
    make_actions();   /* before xrBeginSession (poll_events) */
    glGenFramebuffers(1, &blit_fbo);

    screen_pose.orientation.w = 1.0f;
    screen_pose.position.z = -(float)g_vr_screen_dist / 100.0f;
    SDL_GL_SetSwapInterval(0);   /* the headset paces; the window only mirrors */
    host_log("vr: session created, waiting for the headset (stereo %s)\n", g_vr_stereo ? "on" : "off");
    return 1;

fail:
    xr_shutdown();
    host_log("vr: VR unavailable, running on the desktop\n");
    return 0;
}

int xr_active(void) { return sess != XR_NULL_HANDLE && !lost; }
double xr_display_hz(void) { return xr_active() ? hz : 0.0; }
void xr_recenter(void) { need_center = 1; }

static void poll_events(void)
{
    XrEventDataBuffer ev;
    for (;;) {
        ev.type = XR_TYPE_EVENT_DATA_BUFFER;
        ev.next = NULL;
        if (xrPollEvent(inst, &ev) != XR_SUCCESS) break;
        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const XrEventDataSessionStateChanged* sc = (const XrEventDataSessionStateChanged*)&ev;
            state = sc->state;
            host_log("vr: session state %d\n", (int)state);
            if (state == XR_SESSION_STATE_READY && !running) {
                XrSessionBeginInfo bi = { XR_TYPE_SESSION_BEGIN_INFO };
                bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                if (xr_ok(xrBeginSession(sess, &bi), "xrBeginSession")) { running = 1; need_center = 1; }
            } else if (state == XR_SESSION_STATE_STOPPING && running) {
                xrEndSession(sess);
                running = 0;
            } else if (state == XR_SESSION_STATE_EXITING || state == XR_SESSION_STATE_LOSS_PENDING) {
                lost = 1;
            }
        } else if (ev.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            lost = 1;
        } else if (ev.type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) {
            need_center = 1;   /* the user recentred (long press of the headset's system button) */
        }
    }
    if (lost) {
        host_log("vr: session lost, back to the desktop\n");
        xr_shutdown();
        SDL_GL_SetSwapInterval(render_vsync_setting() ? 1 : 0);
    }
}

static float yaw_of(const XrQuaternionf* q)   /* heading of -Z rotated by q, about +Y */
{
    float fx = -2.0f * (q->x * q->z + q->w * q->y);
    float fz = -(1.0f - 2.0f * (q->x * q->x + q->y * q->y));
    if (fx * fx + fz * fz < 1e-8f) return 0.0f;
    return atan2f(-fx, -fz);
}

/* the head now: recentre if asked (cinema screen in front, game forward where the head looks),
   then publish the recentred head pose */
static void update_head(XrTime t)
{
    XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
    if (!XR_SUCCEEDED(xrLocateSpace(view_space, local_space, t, &loc))) return;
    if (!(loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) || !(loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) return;
    if (need_center) {
        float yaw = yaw_of(&loc.pose.orientation), d = (float)g_vr_screen_dist / 100.0f;
        screen_pose.orientation.x = 0.0f;
        screen_pose.orientation.y = sinf(yaw * 0.5f);
        screen_pose.orientation.z = 0.0f;
        screen_pose.orientation.w = cosf(yaw * 0.5f);
        screen_pose.position.x = loc.pose.position.x - sinf(yaw) * d;
        screen_pose.position.y = loc.pose.position.y;
        screen_pose.position.z = loc.pose.position.z - cosf(yaw) * d;
        SDL_LockSpinlock(&pose_lock);
        rec_yaw = yaw;
        rec_ox = loc.pose.position.x; rec_oy = loc.pose.position.y; rec_oz = loc.pose.position.z;
        SDL_UnlockSpinlock(&pose_lock);
        need_center = 0;
        SDL_AddAtomicInt(&rec_gen, 1);
        host_log("vr: recentred (yaw %.1f deg)\n", yaw * 57.29578f);
    }
    {
        float m[16], c, s, px, py, pz;
        VrPose p;
        const XrQuaternionf* q = &loc.pose.orientation;
        xr_recenter_matrix(m);
        px = loc.pose.position.x; py = loc.pose.position.y; pz = loc.pose.position.z;
        p.px = m[0] * px + m[4] * py + m[8] * pz + m[12];
        p.py = m[1] * px + m[5] * py + m[9] * pz + m[13];
        p.pz = m[2] * px + m[6] * py + m[10] * pz + m[14];
        /* orientation: Ry(-yaw0) * q */
        c = cosf(-rec_yaw * 0.5f); s = sinf(-rec_yaw * 0.5f);
        p.qw = c * q->w - s * q->y;
        p.qx = c * q->x + s * q->z;
        p.qy = c * q->y + s * q->w;
        p.qz = c * q->z - s * q->x;
        SDL_LockSpinlock(&pose_lock);
        head_rec = p;
        head_valid = 1;
        SDL_UnlockSpinlock(&pose_lock);
    }
}

int xr_frame_begin(void)
{
    XrFrameWaitInfo wi = { XR_TYPE_FRAME_WAIT_INFO };
    XrFrameBeginInfo bi = { XR_TYPE_FRAME_BEGIN_INFO };
    eyes_valid = 0;
    if (!xr_active()) return -1;
    poll_events();
    if (!xr_active() || !running) { SDL_SetAtomicInt(&stereo_ready, 0); SDL_Delay(5); return -1; }   /* not visible yet: do not spin */
    fs = (XrFrameState){ XR_TYPE_FRAME_STATE };
    if (!xr_ok(xrWaitFrame(sess, &wi, &fs), "xrWaitFrame")) return -1;
    if (fs.predictedDisplayPeriod > 0) hz = 1e9 / (double)fs.predictedDisplayPeriod;
    if (!xr_ok(xrBeginFrame(sess, &bi), "xrBeginFrame")) return -1;
    in_frame = 1;
    SDL_SetAtomicInt(&stereo_ready, g_vr_stereo && (state == XR_SESSION_STATE_VISIBLE || state == XR_SESSION_STATE_FOCUSED));
    if (!fs.shouldRender) return 0;
    update_head(fs.predictedDisplayTime);
    read_actions(fs.predictedDisplayTime);
    {
        XrViewLocateInfo li = { XR_TYPE_VIEW_LOCATE_INFO };
        XrViewState vs = { XR_TYPE_VIEW_STATE };
        uint32_t n = 0;
        li.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        li.displayTime = fs.predictedDisplayTime;
        li.space = local_space;
        views[0] = (XrView){ XR_TYPE_VIEW }; views[1] = (XrView){ XR_TYPE_VIEW };
        if (XR_SUCCEEDED(xrLocateViews(sess, &li, &vs, 2, &n, views)) && n == 2 &&
            (vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) && (vs.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT)) {
            for (int e = 0; e < 2; e++) {
                const XrView* v = &views[e];
                eyes[e].pose = (VrPose){ v->pose.position.x, v->pose.position.y, v->pose.position.z,
                                         v->pose.orientation.x, v->pose.orientation.y, v->pose.orientation.z, v->pose.orientation.w };
                eyes[e].l = tanf(v->fov.angleLeft); eyes[e].r = tanf(v->fov.angleRight);
                eyes[e].u = tanf(v->fov.angleUp);   eyes[e].d = tanf(v->fov.angleDown);
            }
            eyes_valid = 1;
        }
    }
    return 1;
}

const VrEye* xr_frame_eye(int e) { return eyes_valid && e >= 0 && e < 2 ? &eyes[e] : NULL; }

unsigned int xr_eye_target(int e, int w, int h)
{
    if (!in_frame || e < 0 || e > 1 || w <= 0 || h <= 0) return 0;
    return chain_acquire(&eye_chain[e], w, h);
}

void xr_eye_release(int e) { if (e >= 0 && e < 2) chain_release(&eye_chain[e]); }

static void end_frame(const XrCompositionLayerBaseHeader* const* layers, uint32_t n)
{
    XrFrameEndInfo ei = { XR_TYPE_FRAME_END_INFO };
    if (!in_frame) return;
    ei.displayTime = fs.predictedDisplayTime;
    ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    ei.layerCount = n;
    ei.layers = n ? layers : NULL;
    xr_ok(xrEndFrame(sess, &ei), "xrEndFrame");
    in_frame = 0;
}

void xr_frame_end_none(void)
{
    chain_release(&eye_chain[0]); chain_release(&eye_chain[1]);
    end_frame(NULL, 0);
}

void xr_frame_end_stereo(void)
{
    XrCompositionLayerProjection proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    XrCompositionLayerProjectionView pv[2] = { { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW } };
    const XrCompositionLayerBaseHeader* layers[1];
    if (!eyes_valid || !eye_chain[0].h || !eye_chain[1].h) { xr_frame_end_none(); return; }
    for (int e = 0; e < 2; e++) {
        pv[e].pose = views[e].pose;
        pv[e].fov = views[e].fov;
        pv[e].subImage.swapchain = eye_chain[e].h;
        pv[e].subImage.imageRect.offset.x = 0;
        pv[e].subImage.imageRect.offset.y = 0;
        pv[e].subImage.imageRect.extent.width = eye_chain[e].w;
        pv[e].subImage.imageRect.extent.height = eye_chain[e].ht;
        pv[e].subImage.imageArrayIndex = 0;
    }
    proj.space = local_space;
    proj.viewCount = 2;
    proj.views = pv;
    layers[0] = (const XrCompositionLayerBaseHeader*)&proj;
    held_views[0] = views[0]; held_views[1] = views[1]; held_ok = 1;
    end_frame(layers, 1);
}

/* the eye images (redrawn from the last stereo frame by the caller) at the poses that frame was
   drawn for: the headset keeps them fixed in the room while the head moves (room changes) */
int xr_have_held(void) { return held_ok; }
void xr_frame_end_held(void)
{
    XrCompositionLayerProjection proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    XrCompositionLayerProjectionView pv[2] = { { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW } };
    const XrCompositionLayerBaseHeader* layers[1];
    if (!held_ok || !eye_chain[0].h || !eye_chain[1].h) { xr_frame_end_none(); return; }
    for (int e = 0; e < 2; e++) {
        pv[e].pose = held_views[e].pose;
        pv[e].fov = held_views[e].fov;
        pv[e].subImage.swapchain = eye_chain[e].h;
        pv[e].subImage.imageRect.extent.width = eye_chain[e].w;
        pv[e].subImage.imageRect.extent.height = eye_chain[e].ht;
    }
    proj.space = local_space;
    proj.viewCount = 2;
    proj.views = pv;
    layers[0] = (const XrCompositionLayerBaseHeader*)&proj;
    end_frame(layers, 1);
}

void xr_frame_end_quad(int w, int h) { xr_frame_end_quad_rect(0, 0, w, h); }

/* the cinema screen: the game's picture (x, y, w, h of the window's back buffer, the black
   bars left out) at its own aspect, as wide as vr_screen_angle degrees seen from vr_dist */
void xr_frame_end_quad_rect(int x, int y, int w, int h)
{
    XrCompositionLayerQuad quad = { XR_TYPE_COMPOSITION_LAYER_QUAD };
    const XrCompositionLayerBaseHeader* layers[1];
    GLuint tex;
    if (!in_frame) return;
    if (!fs.shouldRender || w <= 0 || h <= 0 || !(tex = chain_acquire(&quad_chain, w, h))) { xr_frame_end_none(); return; }
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);   /* the window's back buffer, as just drawn */
    glBlitFramebuffer(x, y, x + w, y + h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    chain_release(&quad_chain);
    quad.layerFlags = 0;   /* opaque: the game's alpha channel is GS alpha, not coverage */
    quad.space = local_space;
    quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    quad.subImage.swapchain = quad_chain.h;
    quad.subImage.imageRect.extent.width = quad_chain.w;
    quad.subImage.imageRect.extent.height = quad_chain.ht;
    {   /* the screen floats where the head looks: placed in front of the gaze when it appears,
           then following the head's heading and pitch slowly (~1.2 s), at vr_dist */
        static float sy, sp, shp[3];
        static uint64_t last_ms;
        XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
        uint64_t now = SDL_GetTicks();
        if (view_space && XR_SUCCEEDED(xrLocateSpace(view_space, local_space, fs.predictedDisplayTime, &loc)) &&
            (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) && (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
            const XrQuaternionf* q = &loc.pose.orientation;
            float fy = -2.0f * (q->y * q->z - q->w * q->x);
            float yaw = yaw_of(q), pitch = asinf(fy < -1.0f ? -1.0f : fy > 1.0f ? 1.0f : fy), d = (float)g_vr_screen_dist / 100.0f, k;
            float hp[3] = { loc.pose.position.x, loc.pose.position.y, loc.pose.position.z };
            if (pitch > 0.1f) pitch = 0.1f;   /* vr100: at the eyes' height - it follows sideways, up and down only ~6 degrees */
            if (pitch < -0.1f) pitch = -0.1f;
            if (!last_ms || now - last_ms > 500) { sy = yaw; sp = 0.0f; memcpy(shp, hp, sizeof(shp)); }   /* appears: in front, level with the eyes (even looking down) */
            else {
                float dt = (now - last_ms) / 1000.0f, dy = yaw - sy;
                while (dy > 3.14159265f) dy -= 6.2831853f;
                while (dy < -3.14159265f) dy += 6.2831853f;
                k = 1.0f - expf(-dt / 1.2f);
                sy += dy * k; sp += (pitch - sp) * k;
                for (int i = 0; i < 3; i++) shp[i] += (hp[i] - shp[i]) * k;
            }
            last_ms = now;
            {
                float cy = cosf(sy * 0.5f), syh = sinf(sy * 0.5f), cp = cosf(sp * 0.5f), sph = sinf(sp * 0.5f);
                screen_pose.orientation.x = cy * sph;   /* yaw (about y) then pitch (about x) */
                screen_pose.orientation.y = syh * cp;
                screen_pose.orientation.z = -syh * sph;
                screen_pose.orientation.w = cy * cp;
                screen_pose.position.x = shp[0] - sinf(sy) * cosf(sp) * d;
                screen_pose.position.y = shp[1] + sinf(sp) * d;
                screen_pose.position.z = shp[2] - cosf(sy) * cosf(sp) * d;
            }
        }
    }
    quad.pose = screen_pose;
    {
        float a = (float)(g_vr_screen_angle < 20 ? 20 : g_vr_screen_angle > 120 ? 120 : g_vr_screen_angle) * (3.14159265f / 360.0f);
        quad.size.width = 2.0f * ((float)g_vr_screen_dist / 100.0f) * tanf(a);
    }
    quad.size.height = quad.size.width * (float)quad_chain.ht / (float)quad_chain.w;
    layers[0] = (const XrCompositionLayerBaseHeader*)&quad;
    chain_release(&eye_chain[0]); chain_release(&eye_chain[1]);
    end_frame(layers, 1);
}

void xr_shutdown(void)
{
    SDL_SetAtomicInt(&stereo_ready, 0);
    SDL_LockSpinlock(&pose_lock); input_valid = 0; SDL_UnlockSpinlock(&pose_lock);
    if (aim_space) xrDestroySpace(aim_space);
    if (aiml_space) xrDestroySpace(aiml_space);
    if (grip_space) xrDestroySpace(grip_space);
    if (gripl_space) xrDestroySpace(gripl_space);
    aim_space = aiml_space = grip_space = gripl_space = XR_NULL_HANDLE;
    if (aset) xrDestroyActionSet(aset);
    aset = XR_NULL_HANDLE;
    chain_destroy(&quad_chain);
    chain_destroy(&eye_chain[0]);
    chain_destroy(&eye_chain[1]);
    if (blit_fbo) { glDeleteFramebuffers(1, &blit_fbo); blit_fbo = 0; }
    if (view_space) xrDestroySpace(view_space);
    if (local_space) xrDestroySpace(local_space);
    view_space = local_space = XR_NULL_HANDLE;
    if (sess) { if (running) xrEndSession(sess); xrDestroySession(sess); }
    sess = XR_NULL_HANDLE;
    running = 0;
    in_frame = 0;
    if (inst) xrDestroyInstance(inst);
    inst = XR_NULL_HANDLE;
    sys = XR_NULL_SYSTEM_ID;
    hz = 0.0;
}

#endif
