/*
 * F1 overlay (Dear ImGui): display, audio and port settings. While it is open the game
 * keeps running but receives no pad input. Settings are applied immediately and saved
 * to <save_dir>/recvx.ini.
 */
#include "../ps2_platform.h"
extern "C" {
#include "../audio/audio.h"
}
#include "prof.h"
#include "cheat.h"
#include "coop.h"
#include "actors.h"
#include "net/net.h"
#include "net/coop_proto.h"   /* REC_*: what the host keeps for us */
#include "../gs/stream.h"
#include "tps.h"
#include "menu_mouse.h"
#include "bindings.h"
#include "settings.h"
#include "lobby.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

extern "C" void pad_set_game_input_enabled(int on);

static SDL_Window* win;
static bool ready, visible, cheats_visible, coop_visible;
static bool tps_key_wait;   /* the next key pressed becomes the third-person toggle key */

void overlay_init(void* window, void* gl_context)
{
    win = (SDL_Window*)window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;   /* our settings live in recvx.ini */
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(win, (SDL_GLContext)gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ready = true;
    const char* show = SDL_getenv("RECVX_SHOW_OVERLAY");   /* debugging aid (screenshots): 1 F1, 3 the co-op window */
    visible = show && strcmp(show, "3");
    coop_visible = show && !strcmp(show, "3");
}

int overlay_event(const void* e)
{
    const SDL_Event* ev = (const SDL_Event*)e;
    if (!ready) return 0;
    if (tps_key_wait && ev->type == SDL_EVENT_KEY_DOWN && !ev->key.repeat) {
        if (ev->key.scancode != SDL_SCANCODE_ESCAPE) { g_tps.key = ev->key.scancode; settings_save_later(); }
        tps_key_wait = false;
        return 1;
    }
    if (bind_hotkey(ev, B_SETTINGS)) {
        visible = !visible;
        return 1;
    }
    if (bind_hotkey(ev, B_CHEATS)) {
        cheats_visible = !cheats_visible;
        return 1;
    }
    if (bind_hotkey(ev, B_COOP)) {
        coop_visible = !coop_visible;
        return 1;
    }
    ImGui_ImplSDL3_ProcessEvent(ev);
    if (!visible && !cheats_visible && !coop_visible) return 0;
    return (ev->type == SDL_EVENT_KEY_DOWN || ev->type == SDL_EVENT_KEY_UP || ev->type == SDL_EVENT_TEXT_INPUT ||
            ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN || ev->type == SDL_EVENT_MOUSE_BUTTON_UP || ev->type == SDL_EVENT_MOUSE_WHEEL);
}

/* the rows of a page of the shared table (settings.c): all of them, or on the pages that have
   their own windows (CO-OP: F3, EXTRAS: F2) the ones flagged SET_F1. Values are applied by
   settings_poll (next pumped frame) and recvx.ini written once they stop changing. true = changed */
/* the picture calibration test pattern (settings_calib), in the colours the game's picture gets
   (the overlay itself is drawn without the calibration) */
static void calib_pattern(void)
{
    float sq = ImGui::GetFontSize() * 1.5f, gap = sq * 0.5f, w = CALIB_STEPS * (sq + gap) + gap;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto grey = [](int v) { int g = (int)(gs_picture(v / 255.0f) * 255.0f + 0.5f); return IM_COL32(g, g, g, 255); };
    for (int k = 0; k < 2; k++) {
        float x = p.x + k * (w + gap);
        dl->AddRectFilled(ImVec2(x, p.y), ImVec2(x + w, p.y + sq + 2 * gap), grey(k ? 255 : 0));
        for (int i = 0; i < CALIB_STEPS; i++)
            dl->AddRectFilled(ImVec2(x + gap + i * (sq + gap), p.y + gap), ImVec2(x + gap + i * (sq + gap) + sq, p.y + gap + sq), grey(settings_calib[k][i]));
    }
    ImGui::Dummy(ImVec2(2 * w + gap, sq + 2 * gap));
}

static bool settings_imgui(int page, bool all = true)
{
    bool changed = false;
    for (int i = 0; i < g_settings_n; i++) {
        const Setting* s = &g_settings[i];
        if (s->page != page || !(all || (s->flags & SET_F1))) continue;
        if (s->group && all) ImGui::SeparatorText(s->group);
        const char* lock = settings_locked(s);
        char label[96], val[96];
        int v = settings_get(s), hi = settings_hi(s), st = s->step > 0 ? s->step : 1;
        SDL_snprintf(label, sizeof label, "%s%s##set%d", s->label, (s->flags & SET_RESTART) ? " (next start)" : "", i);
        ImGui::BeginDisabled(lock != nullptr);
        if (s->flags & SET_ACTION) {
            if (ImGui::Button(label)) settings_act(s);
        } else if (settings_is_bool(s)) {
            bool b = v != 0;
            if (ImGui::Checkbox(label, &b)) { settings_set(s, b); changed = true; }
        } else if (s->names || (s->name && (s->count || (hi - s->lo) / st <= 10))) {   /* a list (tenths: a slider) */
            if (ImGui::BeginCombo(label, settings_value(s, val, sizeof val), ImGuiComboFlags_HeightLarge)) {
                for (int k = s->lo; k <= hi; k += st) {
                    char nm[96];
                    if (ImGui::Selectable(s->name ? s->name(k, nm, sizeof nm) : s->names[k - s->lo], k == v)) { settings_set(s, k); changed = true; }
                }
                ImGui::EndCombo();
            }
        } else {   /* a number; a value name (tenths) is shown as the slider's text */
            char fmt[96];
            const char* f = s->fmt ? s->fmt : "%d";
            if (s->name) {
                char* o = fmt;
                for (const char* c = s->name(v, val, sizeof val); *c && o < fmt + sizeof fmt - 2; c++) { if (*c == '%') *o++ = '%'; *o++ = *c; }
                *o = 0;
                f = fmt;
            }
            if (ImGui::SliderInt(label, &v, s->lo, hi, f)) {
                settings_set(s, st > 1 ? (v + st / 2) / st * st : v);
                changed = true;
            }
        }
        ImGui::EndDisabled();
        if ((ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ((s->flags & SET_CALIB) && ImGui::IsItemActive())) && (s->help || lock)) {
            ImGui::BeginTooltip();
            ImGui::Text("%s%s%s", lock ? lock : "", lock && s->help ? "\n" : "", s->help ? s->help : "");
            if (s->flags & SET_CALIB) calib_pattern();
            ImGui::EndTooltip();
        }
    }
    return changed;
}

static bool settings_imgui_any(int page)
{
    for (int i = 0; i < g_settings_n; i++) if (g_settings[i].page == page && (g_settings[i].flags & SET_F1)) return true;
    return false;
}

/* Controls: the bindings of both sets; a click on a binding waits for a key (bindings.c) */
static void controls_section(void)
{
    static int dev, cap_row = -1, cap_slot;
    static char msg[96];
    char label[128], nm[64];
    if (cap_row >= 0) {
        int c = bind_capture_poll();
        if (c != -1) {
            if (c != -2) {
                bind_set(cap_row, dev, cap_slot, c == -3 ? 0 : c);
                if (bind_swapped()) SDL_snprintf(msg, sizeof msg, "Swapped with %s", bind_swapped());
                else msg[0] = 0;
                settings_save_later();
            }
            cap_row = -1;
        }
    }
    settings_imgui(SP_CONTROLS);   /* input source, which gamepad */
    if (ImGui::BeginTabBar("##dev")) {
        if (ImGui::BeginTabItem("Keyboard / mouse")) { if (dev != BIND_KB) cap_row = -1; dev = BIND_KB; ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Gamepad")) { if (dev != BIND_PAD) cap_row = -1; dev = BIND_PAD; ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    if (dev == BIND_PAD) ImGui::TextDisabled("%s; the left stick also moves, as on the DS2.", bind_pad_desc());
    ImGui::TextDisabled("Click a binding, then press the key/button (Esc cancels, Delete clears); right click clears.");
    if (msg[0]) ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "%s", msg);
    if (ImGui::BeginTable("##binds", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch, 2.2f);
        ImGui::TableSetupColumn("1", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("2", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        int group = -1;
        for (int i = 0; i < B_COUNT; i++) {
            int r = bind_order[i];
            if (!bind_row_shown(r, dev)) continue;
            if (bind_rows[r].group != group) {
                group = bind_rows[r].group;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (group == BG_GAME) ImGui::SeparatorText("Game (as the KEY CONFIG type sets them)");
                else ImGui::SeparatorText(group == BG_TPS ? "Third person (keyboard and mouse)" : "Hotkeys");
                if (group == BG_GAME) { ImGui::TableSetColumnIndex(1); ImGui::TextDisabled("type %c", 'A' + pc_keytype()); }
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            int conflict = bind_conflict(r, dev);
            if (conflict) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", bind_row_label(r, label, sizeof label));
            else ImGui::TextUnformatted(bind_row_label(r, label, sizeof label));
            if (conflict && ImGui::IsItemHovered()) ImGui::SetTooltip("Also bound to: %s", bind_rows[conflict - 1].name);
            for (int k = 0; k < BIND_SLOTS; k++) {
                ImGui::TableSetColumnIndex(1 + k);
                char b[96];
                if (cap_row == r && cap_slot == k) SDL_snprintf(b, sizeof b, "press... %ds##%d_%d", (bind_capture_left_ms() + 999) / 1000, r, k);
                else SDL_snprintf(b, sizeof b, "%s##%d_%d", bind_code_name(dev, bind_get(r, dev, k), nm, sizeof nm), r, k);
                if (ImGui::Button(b, ImVec2(-FLT_MIN, 0))) { cap_row = r; cap_slot = k; msg[0] = 0; bind_capture_start(dev); }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { bind_set(r, dev, k, 0); settings_save_later(); }
            }
        }
        ImGui::EndTable();
    }
    if (ImGui::Button(dev ? "Reset gamepad to defaults" : "Reset keyboard / mouse to defaults")) { bind_reset(dev); settings_save_later(); msg[0] = 0; }
    ImGui::TextDisabled("Also in the game's OPTION menu: ENHANCED SETTINGS -> CONTROLS.");
}

static void settings_window(void)
{
    bool changed = false;
    int ww, wh;
    SDL_GetWindowSizeInPixels(win, &ww, &wh);

    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Resident Evil Code: Veronica X  -  settings (F1)  -  " RECVX_VERSION, &visible)) { ImGui::End(); return; }

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= settings_imgui(SP_DISPLAY);
        ImGui::TextDisabled("Output %d x %d, %.1f fps presented", ww, wh, ImGui::GetIO().Framerate);
    }
    if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen)) changed |= settings_imgui(SP_GAMEPLAY);
    if (ImGui::CollapsingHeader("Controls")) controls_section();
    if (ImGui::CollapsingHeader("Third person", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= settings_imgui(SP_TPS);
        char key[96];
        SDL_snprintf(key, sizeof key, tps_key_wait ? "Press a key (Esc cancels)" : "Toggle key: %s (change)", SDL_GetScancodeName((SDL_Scancode)g_tps.key));
        if (ImGui::Button(key)) tps_key_wait = true;
        ImGui::TextDisabled(g_tps.free_cam
                            ? "W/A/S/D walk toward that screen direction, Shift run, mouse turns the camera; right mouse aim, left fire;\n"
                              "E/Space action, Q cancel, Tab status, M map (Controls changes them). F1 frees the mouse."
                            : "W/S walk, A/D turn, Shift run, mouse look; right mouse aim, left fire;\n"
                              "E/Space action, Q cancel, Tab status, M map (Controls changes them). F1 frees the mouse.");
    }
    if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) changed |= settings_imgui(SP_AUDIO);
    for (int p = SP_COOP; p <= SP_EXTRAS; p += SP_EXTRAS - SP_COOP)   /* pages with their own windows (F3, F2): the SET_F1 rows */
        if (settings_imgui_any(p) && ImGui::CollapsingHeader(p == SP_COOP ? "Co-op" : "Extras", ImGuiTreeNodeFlags_DefaultOpen))
            changed |= settings_imgui(p, false);

    if (ImGui::CollapsingHeader("Performance")) {
        static ProfFrame h[240];
        bool on = prof_enabled != 0;
        if (ImGui::Checkbox("Measure (small overhead; always on with --profile)", &on)) prof_enabled = on;
        int n = prof_history(h, 240);
        if (n > 0) {
            float total[240], avg[PROF_COUNT] = { 0 }, avg_total = 0, worst = 0;
            int m = n < 60 ? n : 60;
            for (int i = 0; i < n; i++) total[i] = h[i].total;
            for (int i = n - m; i < n; i++) {
                avg_total += h[i].total / m;
                if (h[i].total > worst) worst = h[i].total;
                for (int c = 0; c < PROF_COUNT; c++) avg[c] += h[i].ms[c] / m;
            }
            ImGui::Text("game frame %.2f ms avg, %.2f ms worst (target 33.37 ms = 29.97 Hz)", avg_total, worst);
            ImGui::PlotLines("##ft", total, n, 0, "frame time (ms)", 0.0f, 66.7f, ImVec2(-1, 60));
            for (int c = 0; c < PROF_COUNT; c++) {
                ImGui::Text("%-8s %6.2f ms", prof_names[c], avg[c]);
                ImGui::SameLine(160);
                ImGui::ProgressBar(avg_total > 0 ? avg[c] / avg_total : 0, ImVec2(-1, 0), "");
            }
            ImGui::TextDisabled("'wait' is idle time; run with --profile for a CSV.");
        }
    }
    ImGui::End();
    if (changed) settings_save_later();
}

static void cheat_window(void)
{
    CheatState* c = &g_cheat;
    ImGui::SetNextWindowPos(ImVec2(500, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Cheats (F2)", &cheats_visible)) { ImGui::End(); return; }
    ImGui::Text("%s  HP %d   stage %d  room %d  pos %d", c->player ? "Chris" : "Claire", c->hp, c->stg, c->rom, c->pos);
    bool b = c->inf_health;
    if (ImGui::Checkbox("Infinite health", &b)) c->inf_health = b;
    ImGui::SameLine();
    if (ImGui::Button("Heal now")) __atomic_or_fetch(&c->request, CHEAT_HEAL, __ATOMIC_ACQ_REL);
    b = c->inf_ammo;
    if (ImGui::Checkbox("Infinite ammo", &b)) c->inf_ammo = b;
    {
        static const char* modes[] = { "Off", "Through walls", "Through walls + fly (R/F, PgUp/PgDn, R2/L2)" };
        int m = c->noclip;
        if (ImGui::Combo("Noclip", &m, modes, IM_ARRAYSIZE(modes))) c->noclip = m;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Walls, objects and enemies no longer block you; doors and triggers still work.\n"
                              "Fly: R / Page Up / R2 up, F / Page Down / L2 down.\n"
                              "Turning it off after flying puts you on the floor level below.");
    }

    ImGui::SeparatorText("Give item");
    char label[48];
    snprintf(label, sizeof label, "%3d  %s", c->give_id, c->give_id > 0 && c->give_id < CHEAT_ITEMS ? c->names[c->give_id] : "-");
    if (ImGui::BeginCombo("Item", label, ImGuiComboFlags_HeightLarge)) {
        for (int id = 1; id < CHEAT_ITEMS; id++) {
            if (!c->names[id][0]) continue;
            snprintf(label, sizeof label, "%3d  %s", id, c->names[id]);
            if (ImGui::Selectable(label, id == c->give_id)) c->give_id = id;
        }
        ImGui::EndCombo();
    }
    ImGui::InputInt("Count", &c->give_count);
    c->give_count = SDL_clamp(c->give_count, 1, 999);
    if (ImGui::Button("Add to inventory") && c->give_id) __atomic_or_fetch(&c->request, CHEAT_GIVE, __ATOMIC_ACQ_REL);

    ImGui::SeparatorText("Inventory");
    for (int i = 1; i < 2 + c->slots && i < 12; i++) {
        unsigned id = c->inv[i] >> 16 & 0xFF;
        char nm[16];
        if (!id) continue;
        if (id >= CHEAT_ITEMS || !c->names[id][0]) SDL_snprintf(nm, sizeof nm, "item %u", id);
        ImGui::Text("%s %-24s %u", (unsigned)i == c->inv[0] ? "equipped" : "        ", id < CHEAT_ITEMS && c->names[id][0] ? c->names[id] : nm, c->inv[i] & 0xFFFF);
    }

    ImGui::SeparatorText("Spawn enemy");
    if (c->spawn_idx < 0 || c->spawn_idx >= spawn_count()) c->spawn_idx = 0;
    if (ImGui::BeginCombo("Enemy", spawn_name(c->spawn_idx), ImGuiComboFlags_HeightLarge)) {
        for (int i = 0; i < spawn_count(); i++)
            if (ImGui::Selectable(spawn_name(i), i == c->spawn_idx)) c->spawn_idx = i;
        ImGui::EndCombo();
    }
    if (ImGui::Button("Spawn in front of me")) __atomic_or_fetch(&c->request, CHEAT_SPAWN, __ATOMIC_ACQ_REL);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Loads the enemy from another room if this one has no such enemy.\n"
                          "Bosses and scripted enemies may not work outside their room (docs/ENEMIES.md).\n"
                          "Spawned enemies are gone after leaving the room; their kills are not recorded.");

    ImGui::SeparatorText("Teleport");
    ImGui::TextDisabled("Uses the game's door transition; an invalid room or position may crash.");
    ImGui::InputInt("Stage", &c->tp_stg);
    ImGui::InputInt("Room", &c->tp_rom);
    ImGui::InputInt("Position", &c->tp_pos);
    c->tp_stg = SDL_clamp(c->tp_stg, 0, 9); c->tp_rom = SDL_clamp(c->tp_rom, 0, 99); c->tp_pos = SDL_clamp(c->tp_pos, 0, 31);   /* the table's ranges */
    if (ImGui::Button("Current room")) { c->tp_stg = c->stg; c->tp_rom = c->rom; c->tp_pos = c->pos; }
    ImGui::SameLine();
    if (ImGui::Button("Go")) __atomic_or_fetch(&c->request, CHEAT_TELEPORT, __ATOMIC_ACQ_REL);
    if (c->result) ImGui::TextDisabled("%s", c->result);
    ImGui::End();
}

/* co-op trade (docs/COOP.md "Trading from the F3 window"): offer an item to the partner standing next to you */
static const char* coop_item(unsigned int e, char* buf, size_t n)
{
    unsigned id = (e >> 16) & 0xFF;
    if (id < CHEAT_ITEMS && g_cheat.names[id][0]) snprintf(buf, n, "%s x%u", g_cheat.names[id], e & 0xFFFF);
    else snprintf(buf, n, "item %u x%u", id, e & 0xFFFF);
    return buf;
}

extern "C" float gs_hor_scale(void);

static CoopStatus cs;   /* co-op: what the game thread published (coop_status_get, once per overlay frame) */
static int tag_flags;   /* the partner's head in the shown frame (STREAM_PT_*, 0: not drawn), game screen */
static float tag_x, tag_y;
void overlay_point(int flags, float x, float y) { tag_flags = flags; tag_x = x; tag_y = y; }

static void coop_trade(void)
{
    CoopState* c = &g_coop;
    char b[64];
    ImGui::SeparatorText("Trade");
    if (cs.trade_in) {
        ImGui::Text("Partner offers: %s", coop_item(cs.trade_in, b, sizeof(b)));
        ImGui::BeginDisabled(!cs.trade_near);
        if (ImGui::Button("Accept")) __atomic_store_n(&c->trade_req, COOP_TRADE_ACCEPT, __ATOMIC_RELEASE);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Decline")) __atomic_store_n(&c->trade_req, COOP_TRADE_DECLINE, __ATOMIC_RELEASE);
    }
    if (cs.trade_out) {
        ImGui::Text("You offer: %s (held until the partner answers)", coop_item(cs.trade_out, b, sizeof(b)));
        if (ImGui::Button("Take back")) __atomic_store_n(&c->trade_req, COOP_TRADE_CANCEL, __ATOMIC_RELEASE);
    } else if (!cs.trade_near) {
        ImGui::TextDisabled("Stand next to your partner to offer items.");
    } else {
        int slots = cs.trade_slots < 8 ? 8 : cs.trade_slots > 10 ? 10 : cs.trade_slots;
        if (c->trade_slot < 2 || c->trade_slot >= 2 + slots || !(cs.trade_inv[c->trade_slot] >> 16)) c->trade_slot = 2;
        if (ImGui::BeginCombo("Item", (cs.trade_inv[c->trade_slot] >> 16) ? coop_item(cs.trade_inv[c->trade_slot], b, sizeof(b)) : "-")) {
            for (int s = 2; s < 2 + slots; s++) {
                if (!(cs.trade_inv[s] >> 16)) continue;
                char lb[80];
                snprintf(lb, sizeof(lb), "%s%s##%d", coop_item(cs.trade_inv[s], b, sizeof(b)), (int)cs.trade_inv[0] == s ? " (equipped)" : "", s);
                if (ImGui::Selectable(lb, c->trade_slot == s)) { c->trade_slot = s; c->trade_count = cs.trade_inv[s] & 0xFFFF; }
            }
            ImGui::EndCombo();
        }
        int have = cs.trade_inv[c->trade_slot] & 0xFFFF;
        if (c->trade_count < 1 || c->trade_count > have) c->trade_count = have;
        if (have > 1) ImGui::SliderInt("Count", &c->trade_count, 1, have);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Ammunition and other stackable items can be split; anything else goes whole.");
        if (ImGui::Button("Offer")) __atomic_store_n(&c->trade_req, COOP_TRADE_OFFER, __ATOMIC_RELEASE);
    }
    if (cs.trade_msg[0]) ImGui::TextDisabled("%s", cs.trade_msg);
}

static void coop_window(void)
{
    CoopState* c = &g_coop;
    bool off = cs.state == COOP_OFF, changed = false;
    ImGui::SetNextWindowPos(ImVec2(940, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(cs.at_title ? "Co-op lobby (F3)###coop" : "Co-op (F3)###coop", &coop_visible)) { ImGui::End(); return; }
    static const char* loots[] = { "Shared loot", "Individual loot" };
    ImGui::BeginDisabled(!off);
    static const char* conns[] = { "Automatic", "Direct (P2P)", "Via server" };
    if (ImGui::Combo("Connection", &c->conn, conns, IM_ARRAYSIZE(conns))) changed = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Automatic: with a co-op server set, hosting also opens a room there, and a room code is joined through it:\n"
                          "a direct path first (hole punching), the server's relay when that fails. An address (ip:port#CODE) is\n"
                          "always joined directly. Without a server: exactly as Direct.\n"
                          "Direct (P2P): addresses only, no server contact. Via server: always through the relay.");
    if (c->conn != COOP_CONN_DIRECT) {   /* first how to connect, then only what it needs (as the lobby) */
        if (ImGui::InputText("Co-op server", c->server, sizeof(c->server))) changed = true;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("The rendezvous/relay server (recvx-server, docs/SERVER.md): host name or IP, port 7141 implied\n"
                              "(host:port for another). Both players use the same server. Empty: no server.");
        ImGui::InputText("Room password", c->password, sizeof(c->password), ImGuiInputTextFlags_Password);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Optional, through the server: the host sets it, the joining player types the same. Not saved.");
    }
    if (ImGui::InputInt("Host port", &c->port)) { c->port = c->port < 1 ? 1 : c->port > 65535 ? 65535 : c->port; changed = true; }
    bool by_code = coop_join_by_code(c);   /* Via server, or Automatic with a server: a room code (as the lobby's CODE row) */
    if (ImGui::InputText(by_code ? "Room code###join" : "Join address###join", c->join, sizeof(c->join))) changed = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(c->conn == COOP_CONN_SERVER ? "The room code the host's lobby or F3 shows (XXXX-XXXX; case and dash do not matter),\n"
                                                        "joined through the co-op server above."
                          : by_code ? "The room code the host's lobby or F3 shows (XXXX-XXXX), joined through the co-op server above.\n"
                                      "An address (ip:port#CODE, the host's \"Copy join address\") still joins directly."
                                    : "The host's address and join code, as its \"Copy join address\" gives them: ip:port#CODE.\n"
                                      "For a room code (XXXX-XXXX) choose Automatic or Via server and set the co-op server.");
    if (ImGui::Combo("Loot (host)", &c->loot, loots, IM_ARRAYSIZE(loots))) changed = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Shared: each item lies there once - whoever takes it has it, so you split the supplies.\n"
                          "Individual: each player takes their own copy. Key items, locks and story flags go to both.\n"
                          "The host's choice applies.");
    bool important = c->important != 0;
    if (ImGui::Checkbox("Shared loot: important items for both (host)", &important)) { c->important = important; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("With shared loot: weapons and ink ribbons lie there for each player (taking one leaves the\n"
                          "partner's where it lies). Off: they are one pickup like ammo and herbs. The host's choice applies.");
    bool upnp = c->upnp != 0;
    if (ImGui::Checkbox("Open port automatically (UPnP)", &upnp)) { c->upnp = upnp; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Host: while you host, ask your router (UPnP, or NAT-PMP) to forward the host port to this PC,\n"
                          "so a friend can join over the internet. The forwarding is removed when you stop.");
    ImGui::EndDisabled();
    bool remember = c->remember != 0;   /* also during a session */
    if (ImGui::Checkbox("Remember partner inventories (host)", &remember)) { c->remember = remember; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Host: keep each partner's items (per player, not per name) when it leaves, saved with your saves.\n"
                          "It can join again with them (\"Saved with the host\"); after a crash it gets them anyway.");
    {   /* the joining player's inventory (as the lobby's ITEMS row) */
        static const char* fixed[] = { "My current game (in game)", "Saved with the host", "New inventory" };
        char cur[48], lab[48];
        int v = c->items;
        auto label = [&](int i, char* b, size_t n) {
            if (i < 0) snprintf(b, n, "%s%s", fixed[i + 3], i == COOP_ITEMS_HOST && cs.state != COOP_OFF && !cs.host && cs.peer_rec >= 0
                                                              ? cs.peer_rec == REC_HAVE ? " (available)" : cs.peer_rec == REC_OFF ? " (host: off)" : " (none)" : "");
            else snprintf(b, n, "Save %02d", i + 1);
        };
        label(v, cur, sizeof(cur));
        ImGui::BeginDisabled(cs.state != COOP_OFF && cs.host);
        if (ImGui::BeginCombo("Items when joining", cur)) {
            for (int i = COOP_ITEMS_CURRENT, slots = lobby_save_slots(); i < slots; i++) {
                label(i, lab, sizeof(lab));
                if (ImGui::Selectable(lab, i == v)) { c->items = i; changed = true; }
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("The joining player's inventory: your game in progress, what the host kept for you (none there: a new\n"
                              "inventory), a new inventory, or the items of one of your saves (an empty slot: a new inventory).\n"
                              "After your game crashed, rejoining brings what the host kept anyway.");
    }
    bool tfreeze = c->trade_freeze != 0;   /* also during a session */
    if (ImGui::Checkbox("Freeze enemies while trading (host)", &tfreeze)) { c->trade_freeze = tfreeze; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("While both players are in the trade screen (trade request key, T / left stick click by default)\n"
                          "the room's enemies stand still. The host's choice applies.");
    bool skipsync = c->skip_sync != 0;   /* also during a session */
    if (ImGui::Checkbox("Co-op: synchronized cutscene skip (host)", &skipsync)) { c->skip_sync = skipsync; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("A cutscene both players watch is skipped once both pressed skip; the partner sees that you want to.\n"
                          "The host's choice applies.");
    bool noribbon = g_config.no_ink_ribbon != 0;   /* the GAMEPLAY setting; in co-op the host's counts */
    if (ImGui::Checkbox("No ink ribbons needed (host)", &noribbon)) { g_config.no_ink_ribbon = noribbon; changed = true; }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Typewriters save without an ink ribbon and use none up (also F1 Gameplay, in single player).\n"
                          "The host's choice applies.");
    if (ImGui::InputText("Your name", c->name, sizeof(c->name), ImGuiInputTextFlags_CallbackCharFilter,
                         [](ImGuiInputTextCallbackData* d) -> int { return !(d->EventChar >= 32 && d->EventChar < 127 && (isalnum(d->EventChar) || strchr(" -_.!?'()+", d->EventChar))); }))
        changed = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("What your partner sees above your head and in the lobby.");
    bool tagopt = c->nametag != 0;
    if (ImGui::Checkbox("Show the partner's name above its head", &tagopt)) { c->nametag = tagopt; changed = true; }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("In game: the partner's name over its head, or at the screen edge pointing to it\nwhen it is in your room but out of view. Its cutscene/menu label shows either way. (F1 too)");
    bool follow = c->follow != 0;
    ImGui::BeginDisabled(cs.state != COOP_OFF && cs.host);   /* the joining player's choice: the host's own has no effect */
    if (ImGui::Checkbox("Follow host (joining player)", &follow)) { c->follow = follow; changed = true; }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("The joining player's choice (the host's has no effect). On: you go through every door the host takes.\n"
                          "Off: you stay where you are and may explore other rooms (their enemies run in your game).\n"
                          "Either way you are moved to the host when: you join or rejoin, the host's game (re)starts or continues,\n"
                          "your own game restarts (continue), the story switches the character, or a story cutscene of the host\n"
                          "(15 s or longer) ends. Short cinemas - levers, shutters, doors, camera pans - move nobody.");
    if (cs.state == COOP_CONNECTED) ImGui::TextDisabled("Session: %s", loots[cs.loot_now ? 1 : 0]);
    auto appear = [](void*, int i) -> const char* { return appear_name(appear_at(i)); };
    int pos = appear_pos(c->chr);   /* the list leaves out event doubles (actors.c) */
    if (ImGui::Combo("You appear as", &pos, appear, nullptr, appear_count(), 20)) { c->chr = appear_at(pos); changed = true; }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Your model, also in single player and in cutscenes; in co-op your partner sees it too (takes effect at once;\nyour motions, weapons and voice stay the character's). Humans and humanoid monsters move with your motions;\nother creatures play their own idle animation.");
    if (off) {
        /* at the title the window closes, so the pad drives the game's menu again */
        if (ImGui::Button("Host")) { __atomic_store_n(&c->request, COOP_REQ_HOST, __ATOMIC_RELEASE); coop_visible = !cs.at_title; }
        ImGui::SameLine();
        if (ImGui::Button("Join")) { __atomic_store_n(&c->request, COOP_REQ_JOIN, __ATOMIC_RELEASE); coop_visible = !cs.at_title; }
    } else {
        if (ImGui::Button("Disconnect")) __atomic_store_n(&c->request, COOP_REQ_STOP, __ATOMIC_RELEASE);
        if (cs.host && (cs.state == COOP_CONNECTED || cs.state == COOP_LOST)) {
            ImGui::SameLine();
            if (ImGui::Button("Remove stuck player")) __atomic_store_n(&c->request, COOP_REQ_KICK, __ATOMIC_RELEASE);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Ends the session with your partner (its game crashed or hangs) and frees its place, also on the\n"
                                  "co-op server: the same join address or room code works again at once.");
        }
    }
    ImGui::TextWrapped("%s", cs.status[0] ? cs.status : "not connected");
    if (cs.state != COOP_OFF && cs.host && cs.code[0]) ImGui::Text("Join code: %.4s-%.4s", cs.code, cs.code + 4);
    if (cs.state != COOP_OFF && cs.host && cs.room[0]) {
        char room[16];
        snprintf(room, sizeof(room), "%.4s-%.4s", cs.room, cs.room + 4);
        ImGui::Text("Room code: %s", room);
        ImGui::SameLine();
        if (ImGui::Button("Copy room code")) SDL_SetClipboardText(room);
    }
    if (cs.state == COOP_CONNECTED && cs.peer_in_game) coop_trade();
    PortmapStatus pm;
    portmap_status(&pm);
    if (pm.local_ip[0] || pm.msg[0]) {   /* hosting */
        char addr[80];
        ImGui::SeparatorText("Internet");
        ImGui::Text("This PC (LAN): %s", pm.local_ip[0] ? pm.local_ip : "?");
        ImGui::Text("Public address: %s", pm.public_ip[0] ? pm.public_ip : pm.state == PM_BUSY ? "looking..." : "unknown");
        if (pm.state == PM_FAILED) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
        if (pm.msg[0]) ImGui::TextWrapped("%s", pm.msg);
        if (pm.state == PM_FAILED) ImGui::PopStyleColor();
        if (pm.public_ip[0]) snprintf(addr, sizeof(addr), "%s:%d#%s", pm.public_ip, pm.public_port, cs.code);
        else snprintf(addr, sizeof(addr), "%s:%d#%s", pm.local_ip, c->port, cs.code);
        if (ImGui::Button(pm.public_ip[0] ? "Copy join address" : "Copy LAN address")) SDL_SetClipboardText(addr);
        ImGui::SameLine();
        ImGui::TextDisabled("%s", addr);
    }
    if (cs.at_title)
        ImGui::TextDisabled("Host: choose New Game or Load Game in the title menu; the partner joins your game.\n"
                            "Join: stay at the title; you enter the host's game once it has started.");
    ImGui::End();
    if (changed) settings_save_later();
}

/* Above the partner's head (co-op): its name (option) and, while it is busy, what with, one
   box. Off screen or behind the camera (same room): a marker at the edge of the view pointing
   at it, with the name. Game screen (640x480, 4:3 area) to the window as gs_gl.c presents it;
   the text scales with the window height (FontScaleDpi). */
static void name_tag(void)
{
    static const char* what[] = { "", "cutscene", "menu", "item box", "trading" };
    ImVec2 ds = ImGui::GetIO().DisplaySize;
    float hs = gs_hor_scale(), a = 4.0f / 3.0f * hs, vw = ds.y * a, vh = ds.y;
    if (vw > ds.x) { vw = ds.x; vh = ds.x / a; }
    if (g_config.aspect == ASPECT_STRETCH && hs == 1.0f) { vw = ds.x; vh = ds.y; }
    float x0 = (ds.x - vw) * 0.5f, y0 = (ds.y - vh) * 0.5f;
    float x = x0 + (0.5f + (tag_x - 320.0f) / (640.0f * hs)) * vw;
    float y = y0 + tag_y / 480.0f * vh;
    bool behind = tag_flags == STREAM_PT_BEHIND;
    const char* name = g_coop.nametag ? (cs.peer_name[0] ? cs.peer_name : "Partner") : nullptr;
    const char* busy = cs.trade_hint[0] ? cs.trade_hint : cs.busy_kind > 0 && cs.busy_kind < 5 ? what[cs.busy_kind] : nullptr;   /* it asks us to trade / is busy */
    ImFont* font = ImGui::GetFont();
    float fs = ImGui::GetFontSize() * 1.25f, bs = ImGui::GetFontSize();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    bool off = behind || x < x0 || x > x0 + vw || y < y0 || y > y0 + vh;
    if (!off) {
        ImVec2 ns = name ? font->CalcTextSizeA(fs, FLT_MAX, 0, name) : ImVec2(0, 0);
        ImVec2 bz = busy ? font->CalcTextSizeA(bs, FLT_MAX, 0, busy) : ImVec2(0, 0);
        float w = (ns.x > bz.x ? ns.x : bz.x) + 10, h = ns.y + bz.y + 6, top = y - h - 2;
        dl->AddRectFilled(ImVec2(x - w * 0.5f, top), ImVec2(x + w * 0.5f, y - 2), IM_COL32(0, 0, 0, 140), 4.0f);
        if (name) dl->AddText(font, fs, ImVec2(x - ns.x * 0.5f, top + 3), IM_COL32(255, 255, 255, 240), name);
        if (busy) dl->AddText(font, bs, ImVec2(x - bz.x * 0.5f, top + 3 + ns.y), IM_COL32(255, 230, 120, 255), busy);
        return;
    }
    if (!name) return;
    /* direction from the view's centre (mirrored when behind the camera), to the edge */
    float cx = x0 + vw * 0.5f, cy = y0 + vh * 0.5f, dx = x - cx, dy = y - cy;
    if (behind) { dx = -dx; dy = -dy; if (dy < 0) dy = -dy; }   /* behind: towards the bottom */
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) { dx = 0; dy = 1; len = 1; }
    dx /= len; dy /= len;
    float m = fs * 1.2f, kx = dx ? ((dx > 0 ? vw : -vw) * 0.5f - (dx > 0 ? m : -m)) / dx : 1e9f, ky = dy ? ((dy > 0 ? vh : -vh) * 0.5f - (dy > 0 ? m : -m)) / dy : 1e9f;
    float k = kx < ky ? kx : ky, px = cx + dx * k, py = cy + dy * k, r = fs * 0.5f;
    ImVec2 tip(px + dx * r, py + dy * r), b1(px - dy * r * 0.6f, py + dx * r * 0.6f), b2(px + dy * r * 0.6f, py - dx * r * 0.6f);
    dl->AddTriangleFilled(tip, b1, b2, IM_COL32(255, 230, 120, 220));
    ImVec2 ns = font->CalcTextSizeA(bs, FLT_MAX, 0, name);
    float tx = px - dx * (r + 4) - ns.x * 0.5f - dx * ns.x * 0.5f, ty = py - dy * (r + 4) - ns.y * 0.5f - dy * ns.y * 0.5f;
    dl->AddRectFilled(ImVec2(tx - 4, ty - 2), ImVec2(tx + ns.x + 4, ty + ns.y + 2), IM_COL32(0, 0, 0, 140), 4.0f);
    dl->AddText(font, bs, ImVec2(tx, ty), IM_COL32(255, 255, 255, 230), name);
}

/* the title screen's way into co-op: a hint, and what the session waits for */
static const char* lobby_hint(void)
{
    if (!cs.at_title || coop_visible || cs.lobby) return nullptr;
    switch (cs.state) {
    case COOP_OFF: return "Online co-op: MULTIPLAYER in the menu, or F3";
    case COOP_HOSTING: return "Co-op: hosting - choose New Game or Load Game; your partner joins it (F3)";
    case COOP_CONNECTED: return "Co-op: connected - the host starts or loads a game, the client joins it (F3)";
    default: return "Co-op: waiting for the host (F3)";
    }
}

void overlay_render(void)
{
    if (!ready) return;
    coop_status_get(&cs);
    bool windows = visible || cheats_visible || coop_visible;
    pad_set_game_input_enabled(!windows);
    tps_update_mouse_mode(win, windows);
    menu_mouse_update(win, windows);
    /* without a window ImGui leaves the cursor alone (menu_mouse_update shows/hides it) */
    if (windows) ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    else ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    bool crosshair = g_tps.on && g_tps.crosshair && g_tps.cam_active && g_tps.aiming;
    const char* hint = lobby_hint();
    bool tag = coop_on && tag_flags && !cs.trade_open && (g_coop.nametag || cs.busy_kind || cs.trade_hint[0]);   /* co-op: the partner is drawn in the frame shown */
    bool ping = g_config.show_ping && coop_on && cs.state == COOP_CONNECTED;   /* co-op: next to the frame rate */
    extern const char* g_vr_status;   /* gs_gl.c: why VR is on the cinema screen (NULL: stereo or no VR) */
    extern int g_vr_debug_log;        /* glue/vr_glue.c: ini vr_debug_log */
    const char* vrs = g_vr_debug_log ? g_vr_status : nullptr;   /* a diagnosis aid: shown only with the debug log */
    if (!visible && !cheats_visible && !coop_visible && !g_config.show_fps && !ping && !crosshair && !hint && !tag && !vrs) return;
    {   /* readable on large displays: scale text with the window height (1080p = 1x) */
        int ww, wh;
        SDL_GetWindowSizeInPixels(win, &ww, &wh);
        ImGui::GetStyle().FontScaleDpi = wh > 1080 ? wh / 1080.0f : 1.0f;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    if (visible) settings_window();
    if (cheats_visible) cheat_window();
    if (coop_visible) coop_window();
    if (crosshair) {   /* third person: the screen centre is where the shot goes (tps_glue.c) */
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 c(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        float s = ImGui::GetIO().DisplaySize.y / 120.0f, g = s * 0.6f, t = s > 8 ? 2.0f : 1.5f;
        ImU32 col = IM_COL32(255, 60, 40, 220), shadow = IM_COL32(0, 0, 0, 160);
        for (int pass = 0; pass < 2; pass++) {
            ImU32 k = pass ? col : shadow;
            float o = pass ? 0.0f : 1.0f;
            dl->AddLine(ImVec2(c.x - g - s + o, c.y + o), ImVec2(c.x - g + o, c.y + o), k, t);
            dl->AddLine(ImVec2(c.x + g + o, c.y + o), ImVec2(c.x + g + s + o, c.y + o), k, t);
            dl->AddLine(ImVec2(c.x + o, c.y - g - s + o), ImVec2(c.x + o, c.y - g + o), k, t);
            dl->AddLine(ImVec2(c.x + o, c.y + g + o), ImVec2(c.x + o, c.y + g + s + o), k, t);
            dl->AddCircleFilled(ImVec2(c.x + o, c.y + o), t, k);
        }
    }
    if (tag) name_tag();
    if (hint) {
        ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 10), ImGuiCond_Always, ImVec2(0, 1));
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::Begin("##coophint", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::TextUnformatted(hint);
        ImGui::End();
    }
    if (vrs) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always, ImVec2(0, 0));
        ImGui::SetNextWindowBgAlpha(0.6f);
        ImGui::Begin("##vrstatus", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                                            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::Text("VR: %s", vrs);
        ImGui::End();
    }
    if (g_config.show_fps || ping) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1, 0));
        ImGui::SetNextWindowBgAlpha(0.4f);
        ImGui::Begin("##fps", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                                       ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);
        if (g_config.show_fps && ping) ImGui::Text("%.1f fps   ping %d ms", ImGui::GetIO().Framerate, cs.ping_ms);
        else if (ping) ImGui::Text("ping %d ms", cs.ping_ms);
        else ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
