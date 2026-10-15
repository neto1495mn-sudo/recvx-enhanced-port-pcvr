/*
 * VR output through OpenXR, stage 1: cinema mode (see xr.h).
 *
 * Render thread only. One session, one quad layer in LOCAL space. Each present:
 *   xrPollEvent (session state) -> xrWaitFrame (paces to the headset) -> xrBeginFrame
 *   -> copy the window's back buffer into the swapchain image -> xrEndFrame(quad).
 * The copy is a framebuffer blit with GL_FRAMEBUFFER_SRGB off, so the bytes the game drew
 * reach the compositor unchanged (an sRGB swapchain then shows them as the window does).
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

#if !defined(_WIN32) || !defined(RECVX_HAVE_OPENXR)

int    xr_init(void) { if (g_vr_request) host_log("vr: not available on this platform yet\n"); return 0; }
int    xr_active(void) { return 0; }
double xr_display_hz(void) { return 0.0; }
void   xr_submit_backbuffer(int w, int h) { (void)w; (void)h; }
void   xr_recenter(void) {}
void   xr_shutdown(void) {}

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

static XrInstance  inst = XR_NULL_HANDLE;
static XrSystemId  sys = XR_NULL_SYSTEM_ID;
static XrSession   sess = XR_NULL_HANDLE;
static XrSpace     local_space = XR_NULL_HANDLE, view_space = XR_NULL_HANDLE;
static XrSessionState state = XR_SESSION_STATE_UNKNOWN;
static int running;             /* between xrBeginSession and xrEndSession */
static int lost;                /* the session ended for good: back to the desktop */
static double hz;               /* from the predicted display period */

static XrSwapchain chain = XR_NULL_HANDLE;
static int64_t chain_fmt;
static int chain_w, chain_h;
static uint32_t nimages;
static XrSwapchainImageOpenGLKHR images[8];
static GLuint blit_fbo;

static XrPosef screen_pose;     /* the quad's pose in LOCAL space */
static int need_center = 1;     /* place the screen in front of the head on the next frame */

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

static void destroy_chain(void)
{
    if (chain) xrDestroySwapchain(chain);
    chain = XR_NULL_HANDLE;
    chain_w = chain_h = 0;
    nimages = 0;
}

static int make_chain(int w, int h)
{
    XrSwapchainCreateInfo ci = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    destroy_chain();
    ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    ci.format = chain_fmt;
    ci.sampleCount = 1;
    ci.width = (uint32_t)w;
    ci.height = (uint32_t)h;
    ci.faceCount = 1;
    ci.arraySize = 1;
    ci.mipCount = 1;
    if (!xr_ok(xrCreateSwapchain(sess, &ci, &chain), "xrCreateSwapchain")) return 0;
    for (uint32_t i = 0; i < 8; i++) images[i] = (XrSwapchainImageOpenGLKHR){ XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR };
    if (!xr_ok(xrEnumerateSwapchainImages(chain, 8, &nimages, (XrSwapchainImageBaseHeader*)images), "xrEnumerateSwapchainImages")) {
        destroy_chain();
        return 0;
    }
    chain_w = w; chain_h = h;
    host_log("vr: swapchain %dx%d, %u images, format 0x%llx\n", w, h, nimages, (unsigned long long)chain_fmt);
    return 1;
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
    ici.applicationInfo.applicationVersion = 1;
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
    glGenFramebuffers(1, &blit_fbo);

    screen_pose.orientation.w = 1.0f;
    screen_pose.position.z = -(float)g_vr_screen_dist / 100.0f;
    SDL_GL_SetSwapInterval(0);   /* the headset paces; the window only mirrors */
    host_log("vr: session created, waiting for the headset\n");
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
            need_center = 1;   /* the user recentered (long press of the Oculus/Meta button) */
        }
    }
    if (lost) {
        host_log("vr: session lost, back to the desktop\n");
        xr_shutdown();
        SDL_GL_SetSwapInterval(render_vsync_setting() ? 1 : 0);
    }
}

/* the screen faces the head: in front of it at the configured distance, upright, yaw only */
static void center_screen(XrTime t)
{
    XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
    if (!XR_SUCCEEDED(xrLocateSpace(view_space, local_space, t, &loc))) return;
    if (!(loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) || !(loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) return;
    {
        const XrQuaternionf q = loc.pose.orientation;
        /* forward is -Z rotated by q; keep its horizontal part */
        float fx = -2.0f * (q.x * q.z + q.w * q.y);
        float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
        float len = sqrtf(fx * fx + fz * fz), yaw, d = (float)g_vr_screen_dist / 100.0f;
        if (len < 1e-4f) { fx = 0.0f; fz = -1.0f; len = 1.0f; }
        fx /= len; fz /= len;
        yaw = atan2f(-fx, -fz);   /* rotation about +Y that turns -Z into (fx, 0, fz) */
        screen_pose.orientation.x = 0.0f;
        screen_pose.orientation.y = sinf(yaw * 0.5f);
        screen_pose.orientation.z = 0.0f;
        screen_pose.orientation.w = cosf(yaw * 0.5f);
        screen_pose.position.x = loc.pose.position.x + fx * d;
        screen_pose.position.y = loc.pose.position.y;
        screen_pose.position.z = loc.pose.position.z + fz * d;
    }
    need_center = 0;
}

void xr_submit_backbuffer(int w, int h)
{
    XrFrameWaitInfo wi = { XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState fs = { XR_TYPE_FRAME_STATE };
    XrFrameBeginInfo bi = { XR_TYPE_FRAME_BEGIN_INFO };
    XrFrameEndInfo ei = { XR_TYPE_FRAME_END_INFO };
    XrCompositionLayerQuad quad = { XR_TYPE_COMPOSITION_LAYER_QUAD };
    const XrCompositionLayerBaseHeader* layers[1];
    int drawn = 0;

    if (!xr_active()) return;
    poll_events();
    if (!xr_active() || !running) { SDL_Delay(5); return; }   /* not visible yet: do not spin */

    if (!xr_ok(xrWaitFrame(sess, &wi, &fs), "xrWaitFrame")) return;
    if (fs.predictedDisplayPeriod > 0) hz = 1e9 / (double)fs.predictedDisplayPeriod;
    if (!xr_ok(xrBeginFrame(sess, &bi), "xrBeginFrame")) return;

    if (fs.shouldRender && w > 0 && h > 0) {
        if (need_center) center_screen(fs.predictedDisplayTime);
        if ((w != chain_w || h != chain_h) && !make_chain(w, h)) goto end;
        {
            uint32_t idx = 0;
            XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            XrSwapchainImageWaitInfo swi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            if (!xr_ok(xrAcquireSwapchainImage(chain, &ai, &idx), "xrAcquireSwapchainImage")) goto end;
            swi.timeout = XR_INFINITE_DURATION;
            if (!xr_ok(xrWaitSwapchainImage(chain, &swi), "xrWaitSwapchainImage")) goto end;
            glDisable(GL_FRAMEBUFFER_SRGB);
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, blit_fbo);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, images[idx].image, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);   /* the window's back buffer, as just drawn */
            glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            xrReleaseSwapchainImage(chain, &ri);
        }
        quad.layerFlags = 0;   /* opaque: the game's alpha channel is GS alpha, not coverage */
        quad.space = local_space;
        quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
        quad.subImage.swapchain = chain;
        quad.subImage.imageRect.offset.x = 0;
        quad.subImage.imageRect.offset.y = 0;
        quad.subImage.imageRect.extent.width = chain_w;
        quad.subImage.imageRect.extent.height = chain_h;
        quad.subImage.imageArrayIndex = 0;
        quad.pose = screen_pose;
        quad.size.width = (float)g_vr_screen_width / 100.0f;
        quad.size.height = quad.size.width * (float)chain_h / (float)chain_w;
        layers[0] = (const XrCompositionLayerBaseHeader*)&quad;
        drawn = 1;
    }
end:
    ei.displayTime = fs.predictedDisplayTime;
    ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    ei.layerCount = drawn ? 1 : 0;
    ei.layers = drawn ? layers : NULL;
    xr_ok(xrEndFrame(sess, &ei), "xrEndFrame");
}

void xr_shutdown(void)
{
    destroy_chain();
    if (blit_fbo) { glDeleteFramebuffers(1, &blit_fbo); blit_fbo = 0; }
    if (view_space) xrDestroySpace(view_space);
    if (local_space) xrDestroySpace(local_space);
    view_space = local_space = XR_NULL_HANDLE;
    if (sess) { if (running) xrEndSession(sess); xrDestroySession(sess); }
    sess = XR_NULL_HANDLE;
    running = 0;
    if (inst) xrDestroyInstance(inst);
    inst = XR_NULL_HANDLE;
    sys = XR_NULL_SYSTEM_ID;
    hz = 0.0;
}

#endif
