/*
 * VR update check (vr119, ini update_check = 1): at start, the latest release of the VR fork on GitHub
 * (its tag "vr-v<major>.<minor>[.<patch>]"), compared with this build's RECVX_VR_VERSION; a newer one is offered in a
 * message box (open the download page / continue). One HTTPS request, nothing about the player sent; at most
 * ~3 s; without a network, a timeout or anything unexpected it says nothing. Windows (WinHTTP) only.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

void host_log(const char* fmt, ...);   /* (not ps2_platform.h: its PS2 names clash with windows.h) */

#define RECVX_VR_VERSION "1.4.3"
const char* g_vr_version = RECVX_VR_VERSION;   /* (the launcher's foot) */
#define UPDATE_HOST L"api.github.com"
#define UPDATE_PATH L"/repos/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases/latest"
#define UPDATE_PAGE "https://github.com/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases/latest"

int g_update_check = 1;   /* ini update_check */
char g_update_tag[64];    /* vr121: the newer release's tag, for the launcher */
int g_update_newer;       /* ... there is one */
extern int g_launcher;    /* host.c: with the launcher, it shows the news (no message box) */

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>

/* the release's JSON (at most 64 KB kept), or 0 */
static int fetch(char* out, int cap)
{
    HINTERNET s, c = NULL, r = NULL;
    int n = 0, ok = 0;
    s = WinHttpOpen(L"recvx-vr-update-check", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) return 0;
    WinHttpSetTimeouts(s, 3000, 3000, 3000, 3000);
    if ((c = WinHttpConnect(s, UPDATE_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0)) &&
        (r = WinHttpOpenRequest(c, L"GET", UPDATE_PATH, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)) &&
        WinHttpSendRequest(r, L"Accept: application/vnd.github+json\r\n", (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(r, NULL)) {
        DWORD status = 0, sz = sizeof(status), got;
        WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
        if (status == 200) {
            ok = 1;
            while (n < cap - 1 && WinHttpReadData(r, out + n, (DWORD)(cap - 1 - n), &got) && got > 0) n += (int)got;
        }
    }
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);
    out[n] = 0;
    return ok && n > 0;
}

/* "vr-v1.2" / "v1.2" / "1.2" -> 1002000, "v1.3.1" -> 1003001 (vr139: a missing patch number is 0); -1 if not a version */
static int version_of(const char* t)
{
    int ma = 0, mi = 0, pa = 0;
    while (*t && (*t < '0' || *t > '9')) t++;
    if (sscanf(t, "%d.%d.%d", &ma, &mi, &pa) < 1) return -1;
    return (ma * 1000 + mi) * 1000 + pa;
}

void update_check(void)
{
    static char json[65536];
    char tag[64] = "";
    const char* p;
    int have = version_of(RECVX_VR_VERSION), latest;
    if (!g_update_check) return;
    if (!fetch(json, sizeof(json))) { host_log("update: no answer from GitHub (no network?) - not checked\n"); return; }
    if (!(p = strstr(json, "\"tag_name\"")) || !(p = strchr(p + 10, '"'))) { host_log("update: no release tag in the answer\n"); return; }
    p++;
    for (int i = 0; i < (int)sizeof(tag) - 1 && p[i] && p[i] != '"'; i++) tag[i] = p[i], tag[i + 1] = 0;
    latest = version_of(tag);
    host_log("update: this is VR %s, the latest release is %s\n", RECVX_VR_VERSION, tag);
    if (latest > have) {
        SDL_strlcpy(g_update_tag, tag, sizeof(g_update_tag));
        g_update_newer = 1;
        if (g_launcher) return;
    }
    if (latest > have) {
        char msg[512];
        const SDL_MessageBoxButtonData buttons[] = {
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Open the download page" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Continue" },
        };
        SDL_MessageBoxData box;
        int pressed = 0;
        SDL_snprintf(msg, sizeof(msg),
                     "A new version of Resident Evil Code: Veronica X - PC Port VR is out: %s (you have %s).\n\n"
                     "Nova versao disponivel: %s (voce tem a %s).", tag, RECVX_VR_VERSION, tag, RECVX_VR_VERSION);
        SDL_zero(box);
        box.flags = SDL_MESSAGEBOX_INFORMATION;
        box.title = "RE CVX VR - update";
        box.message = msg;
        box.numbuttons = 2;
        box.buttons = buttons;
        if (SDL_ShowMessageBox(&box, &pressed) && pressed == 1) SDL_OpenURL(UPDATE_PAGE);
    }
}
#else
void update_check(void) {}
#endif
