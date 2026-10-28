/* Android (Quest): the GL calls of one eye's replay recorded and issued again for the other eye (gs_gl.c
   present_vr_stereo_). The two replays of a headset image are the same frame at the same time: everything the GPU
   is told is the same but the eye's matrix and frustum (uniforms u_vr_m0..3, u_vr_dst), so the second eye does not
   run the game frame's stream again on the CPU (the profiler: the replays are most of the render thread's work).
   While recording, the GL entry points (gl_loader.h pointers) are replaced: state and draw calls are kept; object
   creation, uploads and queries run once; calls whose effect a second issue would not reproduce (deleting objects,
   respecifying the vertex stream's buffer, vertex formats, reading pixels) mark the recording unusable - that eye
   then replays as before. */
#ifdef __ANDROID__
#include "../render/gl_loader.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

void host_log(const char* fmt, ...);

enum { OP_BINDTEX, OP_TEXPARF, OP_TEXPARI, OP_ACTIVETEX, OP_BINDFB, OP_FBTEX, OP_BLIT, OP_VIEWPORT, OP_SCISSOR, OP_ENABLE,
       OP_DISABLE, OP_CLEAR, OP_CLEARCOLOR, OP_CLEARDEPTH, OP_DEPTHFUNC, OP_DEPTHMASK, OP_COLORMASK, OP_BLENDFUNC, OP_BLENDEQ,
       OP_BLENDCOLOR, OP_USEPROG, OP_U1I, OP_U1F, OP_U2F, OP_U4F, OP_U4I, OP_BINDVAO, OP_BINDBUF, OP_DRAW, OP_PIXSTORE };
typedef struct { uint32_t op; uint32_t a[10]; } Call;
static Call* calls;
static size_t ncalls, ccalls;
static int recording, valid;
static const char* why_invalid;
int glrec_buffers_ok;   /* gs_gl.c: the vertex stream is the persistent ring (no glBufferData on it): buffer uploads may run once */

static PFN_glBindTexture r_BindTexture; static PFN_glTexParameterf r_TexParameterf; static PFN_glTexParameteri r_TexParameteri;
static PFN_glActiveTexture r_ActiveTexture; static PFN_glBindFramebuffer r_BindFramebuffer; static PFN_glFramebufferTexture2D r_FramebufferTexture2D;
static PFN_glBlitFramebuffer r_BlitFramebuffer; static PFN_glViewport r_Viewport; static PFN_glScissor r_Scissor; static PFN_glEnable r_Enable;
static PFN_glDisable r_Disable; static PFN_glClear r_Clear; static PFN_glClearColor r_ClearColor; static PFN_glClearDepth r_ClearDepth;
static PFN_glDepthFunc r_DepthFunc; static PFN_glDepthMask r_DepthMask; static PFN_glColorMask r_ColorMask; static PFN_glBlendFuncSeparate r_BlendFuncSeparate;
static PFN_glBlendEquationSeparate r_BlendEquationSeparate; static PFN_glBlendColor r_BlendColor; static PFN_glUseProgram r_UseProgram;
static PFN_glUniform1i r_Uniform1i; static PFN_glUniform1f r_Uniform1f; static PFN_glUniform2f r_Uniform2f; static PFN_glUniform4f r_Uniform4f;
static PFN_glUniform4i r_Uniform4i; static PFN_glBindVertexArray r_BindVertexArray; static PFN_glBindBuffer r_BindBuffer; static PFN_glDrawArrays r_DrawArrays;
static PFN_glPixelStorei r_PixelStorei;
static PFN_glDeleteTextures r_DeleteTextures; static PFN_glDeleteFramebuffers r_DeleteFramebuffers; static PFN_glBufferData r_BufferData;
static PFN_glBufferSubData r_BufferSubData; static PFN_glEnableVertexAttribArray r_EnableVertexAttribArray; static PFN_glVertexAttribPointer r_VertexAttribPointer;
static PFN_glVertexAttribIPointer r_VertexAttribIPointer; static PFN_glReadPixels r_ReadPixels;

static uint32_t fu(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float uf(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
static void bad(const char* why) { if (valid) why_invalid = why; valid = 0; }
static Call* put(uint32_t op)
{
    if (ncalls == ccalls) {
        size_t nc = ccalls ? ccalls * 2 : 4096;
        Call* n = realloc(calls, nc * sizeof(Call));
        if (!n) { bad("out of memory"); ncalls = 0; return NULL; }
        calls = n; ccalls = nc;
    }
    calls[ncalls].op = op;
    return &calls[ncalls++];
}
#define REC(op, ...) do { Call* c_ = put(op); if (c_) { uint32_t v_[] = { __VA_ARGS__ }; memcpy(c_->a, v_, sizeof(v_)); } } while (0)

static void GLAPIENTRY w_BindTexture(GLenum t, GLuint x) { REC(OP_BINDTEX, t, x); r_BindTexture(t, x); }
static void GLAPIENTRY w_TexParameterf(GLenum t, GLenum p, GLfloat v) { REC(OP_TEXPARF, t, p, fu(v)); r_TexParameterf(t, p, v); }
static void GLAPIENTRY w_TexParameteri(GLenum t, GLenum p, GLint v) { REC(OP_TEXPARI, t, p, (uint32_t)v); r_TexParameteri(t, p, v); }
static void GLAPIENTRY w_ActiveTexture(GLenum t) { REC(OP_ACTIVETEX, t); r_ActiveTexture(t); }
static void GLAPIENTRY w_BindFramebuffer(GLenum t, GLuint f) { REC(OP_BINDFB, t, f); r_BindFramebuffer(t, f); }
static void GLAPIENTRY w_FramebufferTexture2D(GLenum t, GLenum a, GLenum tt, GLuint x, GLint l) { REC(OP_FBTEX, t, a, tt, x, (uint32_t)l); r_FramebufferTexture2D(t, a, tt, x, l); }
static void GLAPIENTRY w_BlitFramebuffer(GLint a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLint h, GLbitfield m, GLenum fl)
{ REC(OP_BLIT, (uint32_t)a, (uint32_t)b, (uint32_t)c, (uint32_t)d, (uint32_t)e, (uint32_t)f, (uint32_t)g, (uint32_t)h, m, fl); r_BlitFramebuffer(a, b, c, d, e, f, g, h, m, fl); }
static void GLAPIENTRY w_Viewport(GLint x, GLint y, GLsizei w, GLsizei h) { REC(OP_VIEWPORT, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h); r_Viewport(x, y, w, h); }
static void GLAPIENTRY w_Scissor(GLint x, GLint y, GLsizei w, GLsizei h) { REC(OP_SCISSOR, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h); r_Scissor(x, y, w, h); }
static void GLAPIENTRY w_Enable(GLenum c) { REC(OP_ENABLE, c); r_Enable(c); }
static void GLAPIENTRY w_Disable(GLenum c) { REC(OP_DISABLE, c); r_Disable(c); }
static void GLAPIENTRY w_Clear(GLbitfield m) { REC(OP_CLEAR, m); r_Clear(m); }
static void GLAPIENTRY w_ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { REC(OP_CLEARCOLOR, fu(r), fu(g), fu(b), fu(a)); r_ClearColor(r, g, b, a); }
static void GLAPIENTRY w_ClearDepth(GLdouble d) { uint32_t u[2]; memcpy(u, &d, 8); REC(OP_CLEARDEPTH, u[0], u[1]); r_ClearDepth(d); }
static void GLAPIENTRY w_DepthFunc(GLenum f) { REC(OP_DEPTHFUNC, f); r_DepthFunc(f); }
static void GLAPIENTRY w_DepthMask(GLboolean f) { REC(OP_DEPTHMASK, f); r_DepthMask(f); }
static void GLAPIENTRY w_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { REC(OP_COLORMASK, r, g, b, a); r_ColorMask(r, g, b, a); }
static void GLAPIENTRY w_BlendFuncSeparate(GLenum a, GLenum b, GLenum c, GLenum d) { REC(OP_BLENDFUNC, a, b, c, d); r_BlendFuncSeparate(a, b, c, d); }
static void GLAPIENTRY w_BlendEquationSeparate(GLenum a, GLenum b) { REC(OP_BLENDEQ, a, b); r_BlendEquationSeparate(a, b); }
static void GLAPIENTRY w_BlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { REC(OP_BLENDCOLOR, fu(r), fu(g), fu(b), fu(a)); r_BlendColor(r, g, b, a); }
static void GLAPIENTRY w_UseProgram(GLuint p) { REC(OP_USEPROG, p); r_UseProgram(p); }
static void GLAPIENTRY w_Uniform1i(GLint l, GLint a) { REC(OP_U1I, (uint32_t)l, (uint32_t)a); r_Uniform1i(l, a); }
static void GLAPIENTRY w_Uniform1f(GLint l, GLfloat a) { REC(OP_U1F, (uint32_t)l, fu(a)); r_Uniform1f(l, a); }
static void GLAPIENTRY w_Uniform2f(GLint l, GLfloat a, GLfloat b) { REC(OP_U2F, (uint32_t)l, fu(a), fu(b)); r_Uniform2f(l, a, b); }
static void GLAPIENTRY w_Uniform4f(GLint l, GLfloat a, GLfloat b, GLfloat c, GLfloat d) { REC(OP_U4F, (uint32_t)l, fu(a), fu(b), fu(c), fu(d)); r_Uniform4f(l, a, b, c, d); }
static void GLAPIENTRY w_Uniform4i(GLint l, GLint a, GLint b, GLint c, GLint d) { REC(OP_U4I, (uint32_t)l, (uint32_t)a, (uint32_t)b, (uint32_t)c, (uint32_t)d); r_Uniform4i(l, a, b, c, d); }
static void GLAPIENTRY w_BindVertexArray(GLuint a) { REC(OP_BINDVAO, a); r_BindVertexArray(a); }
static void GLAPIENTRY w_BindBuffer(GLenum t, GLuint b) { REC(OP_BINDBUF, t, b); r_BindBuffer(t, b); }
static void GLAPIENTRY w_DrawArrays(GLenum m, GLint f, GLsizei n) { REC(OP_DRAW, m, (uint32_t)f, (uint32_t)n); r_DrawArrays(m, f, n); }
static void GLAPIENTRY w_PixelStorei(GLenum p, GLint v) { REC(OP_PIXSTORE, p, (uint32_t)v); r_PixelStorei(p, v); }
/* run, and the recording cannot be issued again */
static void GLAPIENTRY w_DeleteTextures(GLsizei n, const GLuint* t) { bad("a texture deleted"); r_DeleteTextures(n, t); }
static void GLAPIENTRY w_DeleteFramebuffers(GLsizei n, const GLuint* f) { bad("a framebuffer deleted"); r_DeleteFramebuffers(n, f); }
static void GLAPIENTRY w_BufferData(GLenum t, GLsizeiptr s, const void* d, GLenum u) { if (!glrec_buffers_ok) bad("vertex buffer respecified"); r_BufferData(t, s, d, u); }
static void GLAPIENTRY w_BufferSubData(GLenum t, GLintptr o, GLsizeiptr s, const void* d) { if (!glrec_buffers_ok) bad("vertex buffer upload"); r_BufferSubData(t, o, s, d); }
static void GLAPIENTRY w_EnableVertexAttribArray(GLuint i) { bad("vertex format"); r_EnableVertexAttribArray(i); }
static void GLAPIENTRY w_VertexAttribPointer(GLuint i, GLint s, GLenum t, GLboolean n, GLsizei st, const void* p) { bad("vertex format"); r_VertexAttribPointer(i, s, t, n, st, p); }
static void GLAPIENTRY w_VertexAttribIPointer(GLuint i, GLint s, GLenum t, GLsizei st, const void* p) { bad("vertex format"); r_VertexAttribIPointer(i, s, t, st, p); }
static void GLAPIENTRY w_ReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, void* p) { bad("pixels read"); r_ReadPixels(x, y, w, h, f, t, p); }

#define SWAP(name) do { r_##name = gl_##name; gl_##name = w_##name; } while (0)
#define BACK(name) do { gl_##name = r_##name; } while (0)
#define ALL(X) X(BindTexture); X(TexParameterf); X(TexParameteri); X(ActiveTexture); X(BindFramebuffer); X(FramebufferTexture2D); \
    X(BlitFramebuffer); X(Viewport); X(Scissor); X(Enable); X(Disable); X(Clear); X(ClearColor); X(ClearDepth); X(DepthFunc); \
    X(DepthMask); X(ColorMask); X(BlendFuncSeparate); X(BlendEquationSeparate); X(BlendColor); X(UseProgram); X(Uniform1i); \
    X(Uniform1f); X(Uniform2f); X(Uniform4f); X(Uniform4i); X(BindVertexArray); X(BindBuffer); X(DrawArrays); X(PixelStorei); \
    X(DeleteTextures); X(DeleteFramebuffers); X(BufferData); X(BufferSubData); X(EnableVertexAttribArray); X(VertexAttribPointer); \
    X(VertexAttribIPointer); X(ReadPixels)

void glrec_begin(void)
{
    if (recording) return;
    ncalls = 0; valid = 1; why_invalid = NULL;
    ALL(SWAP);
    recording = 1;
}

/* 1: the recording can be issued again */
int glrec_end(void)
{
    if (!recording) return 0;
    ALL(BACK);
    recording = 0;
    return valid;
}
void glrec_invalidate(const char* why) { bad(why); }
const char* glrec_why(void) { return why_invalid ? why_invalid : "?"; }
size_t glrec_count(void) { return ncalls; }

/* issue the recording again; Uniform4f to the n locations loc[] takes vals[] instead */
void glrec_replay(const GLint* loc, const float (*vals)[4], int n)
{
    for (size_t i = 0; i < ncalls && valid; i++) {
        const uint32_t* a = calls[i].a;
        switch (calls[i].op) {
        case OP_BINDTEX: r_BindTexture(a[0], a[1]); break;
        case OP_TEXPARF: r_TexParameterf(a[0], a[1], uf(a[2])); break;
        case OP_TEXPARI: r_TexParameteri(a[0], a[1], (GLint)a[2]); break;
        case OP_ACTIVETEX: r_ActiveTexture(a[0]); break;
        case OP_BINDFB: r_BindFramebuffer(a[0], a[1]); break;
        case OP_FBTEX: r_FramebufferTexture2D(a[0], a[1], a[2], a[3], (GLint)a[4]); break;
        case OP_BLIT: r_BlitFramebuffer((GLint)a[0], (GLint)a[1], (GLint)a[2], (GLint)a[3], (GLint)a[4], (GLint)a[5], (GLint)a[6], (GLint)a[7], a[8], a[9]); break;
        case OP_VIEWPORT: r_Viewport((GLint)a[0], (GLint)a[1], (GLsizei)a[2], (GLsizei)a[3]); break;
        case OP_SCISSOR: r_Scissor((GLint)a[0], (GLint)a[1], (GLsizei)a[2], (GLsizei)a[3]); break;
        case OP_ENABLE: r_Enable(a[0]); break;
        case OP_DISABLE: r_Disable(a[0]); break;
        case OP_CLEAR: r_Clear(a[0]); break;
        case OP_CLEARCOLOR: r_ClearColor(uf(a[0]), uf(a[1]), uf(a[2]), uf(a[3])); break;
        case OP_CLEARDEPTH: { GLdouble d; memcpy(&d, a, 8); r_ClearDepth(d); break; }
        case OP_DEPTHFUNC: r_DepthFunc(a[0]); break;
        case OP_DEPTHMASK: r_DepthMask((GLboolean)a[0]); break;
        case OP_COLORMASK: r_ColorMask((GLboolean)a[0], (GLboolean)a[1], (GLboolean)a[2], (GLboolean)a[3]); break;
        case OP_BLENDFUNC: r_BlendFuncSeparate(a[0], a[1], a[2], a[3]); break;
        case OP_BLENDEQ: r_BlendEquationSeparate(a[0], a[1]); break;
        case OP_BLENDCOLOR: r_BlendColor(uf(a[0]), uf(a[1]), uf(a[2]), uf(a[3])); break;
        case OP_USEPROG: r_UseProgram(a[0]); break;
        case OP_U1I: r_Uniform1i((GLint)a[0], (GLint)a[1]); break;
        case OP_U1F: r_Uniform1f((GLint)a[0], uf(a[1])); break;
        case OP_U2F: r_Uniform2f((GLint)a[0], uf(a[1]), uf(a[2])); break;
        case OP_U4F: {
            int k = 0;
            while (k < n && loc[k] != (GLint)a[0]) k++;
            if (k < n) r_Uniform4f((GLint)a[0], vals[k][0], vals[k][1], vals[k][2], vals[k][3]);
            else r_Uniform4f((GLint)a[0], uf(a[1]), uf(a[2]), uf(a[3]), uf(a[4]));
            break;
        }
        case OP_U4I: r_Uniform4i((GLint)a[0], (GLint)a[1], (GLint)a[2], (GLint)a[3], (GLint)a[4]); break;
        case OP_BINDVAO: r_BindVertexArray(a[0]); break;
        case OP_BINDBUF: r_BindBuffer(a[0], a[1]); break;
        case OP_DRAW: r_DrawArrays(a[0], (GLint)a[1], (GLsizei)a[2]); break;
        case OP_PIXSTORE: r_PixelStorei(a[0], (GLint)a[1]); break;
        }
    }
}
#else
typedef int gl_record_unused;
#endif
