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
extern "C" int xr_active(void);   /* vr132 */
extern "C" {
#include "xr.h"
}

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
#include <vector>
#include <sys/stat.h>
#include <string>

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
#ifdef __ANDROID__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330 core");
#endif
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
        cheats_visible = g_extras_unlocked && !cheats_visible;   /* vr218: only once unlocked */
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

/* one row of the settings list (vr121: also the launcher's graphics tab) */
static bool setting_widget(int i)
{
    const Setting* s = &g_settings[i];
    bool changed = false;
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
    return changed;
}

static bool settings_imgui(int page, bool all = true)
{
    bool changed = false;
    for (int i = 0; i < g_settings_n; i++) {
        const Setting* s = &g_settings[i];
        if (s->page != page || !(all || (s->flags & SET_F1))) continue;
        if (s->group && all) ImGui::SeparatorText(s->group);
        changed |= setting_widget(i);
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

    if (ImGui::CollapsingHeader("VR", ImGuiTreeNodeFlags_DefaultOpen)) changed |= settings_imgui(SP_VR);   /* vr116 */
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
        if ((p != SP_EXTRAS || g_extras_unlocked) && settings_imgui_any(p) && ImGui::CollapsingHeader(p == SP_COOP ? "Co-op" : "Extras", ImGuiTreeNodeFlags_DefaultOpen))
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
    bool crosshair = g_tps.on && g_tps.crosshair && g_tps.cam_active && g_tps.aiming && !xr_active();   /* vr132: none in VR (it was drawn over the mirror) */
    const char* hint = lobby_hint();
    bool tag = coop_on && tag_flags && !cs.trade_open && (g_coop.nametag || cs.busy_kind || cs.trade_hint[0]);   /* co-op: the partner is drawn in the frame shown */
    bool ping = g_config.show_ping && coop_on && cs.state == COOP_CONNECTED;   /* co-op: next to the frame rate */
    extern const char* g_vr_status;   /* gs_gl.c: why VR is on the cinema screen (NULL: stereo or no VR) */
    extern int g_vr_debug_log;        /* glue/vr_glue.c: ini vr_debug_log */
    const char* vrs = nullptr; (void)g_vr_debug_log; (void)g_vr_status;   /* vr188: the tag is no longer drawn in the picture (the reason stays in the log) */
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

/* vr122: the launcher, after the classic PC ones - Max Payne 1/2 (a small window, the characters' portrait the
   whole height on one side, Play / Options / Quit beside it) and Tomb Raider 2013 (the logo on top, the settings in
   tabs). The portrait is launcher.png next to the game if there (the player's own picture - no game art ships with
   the mod), else a dark panel. One PLAY button: the game is played in VR. The options on their own page, in
   Portuguese. Main thread, before xr_init; the game's window itself, small and centred, given back its own size /
   full screen on PLAY. ini launcher = 1 (--no-launcher skips it). Returns 0 to quit, 1 to play. */
extern "C" int g_vr_request, g_launcher, g_tex_hd, g_tex_hd_per_frame, g_tex_loaders, g_tex_upload, g_vr_turn, g_vr_comfort, g_vr_sight, g_vr_res;
extern "C" char g_update_tag[64];
extern "C" int g_update_newer;
extern "C" unsigned char* stbi_load(const char* filename, int* x, int* y, int* comp, int req_comp);
extern "C" void stbi_image_free(void* p);
extern "C" {
#include "../render/gl_loader.h"
}

static ImFont *lf_ui, *lf_bold, *lf_title;

static const Setting* setting_by_label(const char* label)
{
    for (int i = 0; i < g_settings_n; i++)
        if (!strcmp(g_settings[i].label, label)) return &g_settings[i];
    return nullptr;
}

/* vr134: the launcher in English (the default), Portuguese or Spanish (ini launcher_lang 0, 1, 2) */
enum { L_PLAY, L_HINT, L_OPTIONS, L_EXIT, L_TAB_VR, L_TAB_GFX, L_TAB_TEX, L_TURN, L_COMFORT, L_SIGHT, L_VRRES, L_FULLSCREEN, L_WINSIZE, L_INTRES, L_VSYNC, L_FPSLIM, L_TEXHD, L_PERFRAME, L_PERFRAME_HELP, L_SHOW, L_BACK, L_DEFAULT, L_UPDATE, L_FOOT1, L_FOOT2, L_SMOOTH, L_OFF, L_MEDIUM, L_STRONG, L_LASER, L_DOT, L_NOSIGHT, L_N };
static const char* const ltext[3][L_N] = {
    {   /* en */
        "PLAY",
        "Put on the headset: the game opens straight in VR.",
        "OPTIONS",
        "EXIT",
        "VR",
        "GRAPHICS",
        "TEXTURES",
        "Turn (right stick)",
        "Dark edges while walking",
        "Sight",
        "Headset resolution",
        "Fullscreen",
        "Window size",
        "Internal resolution",
        "VSync",
        "Frame rate limit",
        "HD textures",
        "Reading threads",
        "Upload in the background: the HD textures go to the video card on another CPU. Automatic: on with NVIDIA and AMD.",
        "Show this launcher when the game starts",
        "BACK",
        "DEFAULT",
        "Download update %s",
        "VR %s  \xC2\xB7  VR mod by neto1495mn-sudo",
        "PC port: aikatore  \xC2\xB7  decompilation: AshfordFamily",
        "Smooth",
        "Off",
        "Medium",
        "Strong",
        "Laser",
        "Dot",
        "No sight",
    },
    {   /* pt */
        "JOGAR",
        "Coloque o \xC3\xB3" "culos: o jogo abre direto no VR.",
        "OP\xC3\x87\xC3\x95" "ES",
        "SAIR",
        "VR",
        "GR\xC3\x81" "FICO",
        "TEXTURAS",
        "Giro (anal\xC3\xB3gico direito)",
        "Bordas escuras ao andar",
        "Mira",
        "Resolu\xC3\xA7\xC3\xA3o no \xC3\xB3" "culos",
        "Tela cheia",
        "Tamanho da janela",
        "Resolu\xC3\xA7\xC3\xA3o interna",
        "VSync",
        "Limite de quadros",
        "Texturas HD",
        "Threads de leitura",
        "Envio em segundo plano: as texturas HD v\xC3\xA3o para a placa de v\xC3\xAD" "deo em outra CPU. Autom\xC3\xA1tico: liga em NVIDIA e AMD.",
        "Mostrar este launcher ao abrir o jogo",
        "VOLTAR",
        "PADR\xC3\x83O",
        "Baixar atualiza\xC3\xA7\xC3\xA3o %s",
        "VR %s  \xC2\xB7  mod VR por neto1495mn-sudo",
        "port PC: aikatore  \xC2\xB7  decompila\xC3\xA7\xC3\xA3o: AshfordFamily",
        "Suave",
        "Desligado",
        "M\xC3\xA9" "dio",
        "Forte",
        "Laser",
        "Ponto",
        "Sem mira",
    },
    {   /* es */
        "JUGAR",
        "Ponte el visor: el juego se abre directamente en VR.",
        "OPCIONES",
        "SALIR",
        "VR",
        "GR\xC3\x81" "FICOS",
        "TEXTURAS",
        "Giro (stick derecho)",
        "Bordes oscuros al caminar",
        "Mira",
        "Resoluci\xC3\xB3n en el visor",
        "Pantalla completa",
        "Tama\xC3\xB1o de ventana",
        "Resoluci\xC3\xB3n interna",
        "VSync",
        "L\xC3\xADmite de fotogramas",
        "Texturas HD",
        "Hilos de lectura",
        "Env\xC3\xADo en segundo plano: las texturas HD van a la tarjeta de v\xC3\xAD" "deo en otra CPU. Autom\xC3\xA1tico: activo con NVIDIA y AMD.",
        "Mostrar este launcher al iniciar el juego",
        "VOLVER",
        "PREDETERMINADO",
        "Descargar actualizaci\xC3\xB3n %s",
        "VR %s  \xC2\xB7  mod VR por neto1495mn-sudo",
        "port de PC: aikatore  \xC2\xB7  descompilaci\xC3\xB3n: AshfordFamily",
        "Suave",
        "Desactivado",
        "Medio",
        "Fuerte",
        "L\xC3\xA1ser",
        "Punto",
        "Sin mira",
    },
};
extern "C" int g_launcher_lang;
extern "C" const char* g_vr_version;
static const char* L(int k) { return ltext[g_launcher_lang >= 0 && g_launcher_lang < 3 ? g_launcher_lang : 0][k]; }

static void opt_label(const char* text)   /* Tomb Raider's two columns: the name, then its control */
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
    ImGui::SameLine(200.0f);
    ImGui::SetNextItemWidth(-1.0f);   /* (to the panel's edge) */
}

static bool opt_combo(const char* text, const char* const* names, int n, int* idx)
{
    bool ch = false;
    opt_label(text);
    ImGui::PushID(text);
    if (ImGui::BeginCombo("##c", names[*idx])) {
        for (int k = 0; k < n; k++)
            if (ImGui::Selectable(names[k], k == *idx)) { *idx = k; ch = true; }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return ch;
}

static void opt_setting(const char* text, const char* label)   /* a list of the menus' settings, with our own name */
{
    const Setting* s = setting_by_label(label);
    char val[96];
    if (!s) return;
    opt_label(text);
    ImGui::PushID(label);
    ImGui::BeginDisabled(settings_locked(s) != nullptr);
    if (settings_is_bool(s)) {
        bool b = settings_get(s) != 0;
        if (ImGui::Checkbox("##b", &b)) settings_set(s, b);
    } else if (ImGui::BeginCombo("##c", settings_value(s, val, sizeof val))) {
        int v = settings_get(s), st = s->step > 0 ? s->step : 1;
        for (int k = s->lo; k <= settings_hi(s); k += st) {
            char nm[96];
            if (ImGui::Selectable(s->name ? s->name(k, nm, sizeof nm) : s->names ? s->names[k - s->lo] : (SDL_snprintf(nm, sizeof nm, "%d", k), nm), k == v)) settings_set(s, k);
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    ImGui::PopID();
}

/* a button drawn by hand: primary = red, filled; else dark with an outline */
static bool l_button(const char* text, ImVec2 pos, ImVec2 size, bool primary, float fsize)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(pos);
    bool pressed = ImGui::InvisibleButton(text, size);
    bool hov = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
    ImVec2 b(pos.x + size.x, pos.y + size.y);
    if (primary) {
        ImU32 top = hov ? IM_COL32(215, 32, 38, 255) : IM_COL32(175, 22, 28, 255), bot = hov ? IM_COL32(120, 10, 14, 255) : IM_COL32(95, 8, 12, 255);
        if (hov) dl->AddRectFilled(ImVec2(pos.x - 4, pos.y - 4), ImVec2(b.x + 4, b.y + 4), IM_COL32(220, 30, 36, 50), 6.0f);   /* glow */
        dl->AddRectFilledMultiColor(pos, b, top, top, bot, bot);
        dl->AddRect(pos, b, IM_COL32(255, 120, 120, held ? 255 : 120), 0.0f, 0, 1.5f);
    } else {
        dl->AddRectFilled(pos, b, hov ? IM_COL32(60, 14, 18, 230) : IM_COL32(16, 16, 20, 200));
        dl->AddRect(pos, b, hov ? IM_COL32(220, 40, 46, 255) : IM_COL32(120, 110, 105, 140), 0.0f, 0, 1.2f);
    }
    ImGui::PushFont(lf_bold, fsize);
    ImVec2 ts = ImGui::CalcTextSize(text);
    dl->AddText(ImVec2(pos.x + (size.x - ts.x) * 0.5f, pos.y + (size.y - ts.y) * 0.5f), IM_COL32(255, 250, 245, 255), text);
    ImGui::PopFont();
    return pressed;
}

static void l_text(ImDrawList* dl, ImFont* f, float size, ImVec2 p, ImU32 col, const char* t, float spacing = 0.0f)
{
    ImGui::PushFont(f, size);
    if (spacing <= 0.0f) dl->AddText(p, col, t);
    else for (const char* c = t; *c; c++) { char s[2] = { *c, 0 }; dl->AddText(p, col, s); p.x += ImGui::CalcTextSize(s).x + spacing; }
    ImGui::PopFont();
}

extern "C" int overlay_launcher(void)
{
    static const float LW = 980.0f, LH = 600.0f;
    const char* turns[] = { "", "15\xC2\xB0", "30\xC2\xB0", "45\xC2\xB0", "60\xC2\xB0", "90\xC2\xB0" };
    static const int turn_v[] = { 0, 15, 30, 45, 60, 90 };
    const char* comforts[3];
    static const int comfort_v[] = { 0, 2, 3 };
    const char* sights[3];
    static const int sight_v[] = { 2, 1, 0 };
    bool options = false, show = g_launcher != 0;
    int result = -1, tab = 0;
    GLuint pic = 0;
    int pw = 0, ph = 0;
    if (!ready) return 1;
    {   /* fonts: Windows' own (Segoe UI, Bahnschrift), else ImGui's */
        ImGuiIO& io = ImGui::GetIO();
        const char* wd = SDL_getenv("WINDIR");
        char p[512];
        SDL_snprintf(p, sizeof p, "%s\\Fonts\\segoeui.ttf", wd ? wd : "C:\\Windows");
        if (FILE* f = fopen(p, "rb")) { fclose(f); lf_ui = io.Fonts->AddFontFromFileTTF(p, 18.0f); }
        SDL_snprintf(p, sizeof p, "%s\\Fonts\\seguisb.ttf", wd ? wd : "C:\\Windows");
        if (FILE* f = fopen(p, "rb")) { fclose(f); lf_bold = io.Fonts->AddFontFromFileTTF(p, 22.0f); }
        SDL_snprintf(p, sizeof p, "%s\\Fonts\\bahnschrift.ttf", wd ? wd : "C:\\Windows");
        if (FILE* f = fopen(p, "rb")) { fclose(f); lf_title = io.Fonts->AddFontFromFileTTF(p, 46.0f); }
        if (!lf_ui) lf_ui = io.Fonts->AddFontDefault();
        if (!lf_bold) lf_bold = lf_ui;
        if (!lf_title) lf_title = lf_bold;
    }
    {   /* the portrait: launcher.png next to the game */
        const char* base = SDL_GetBasePath();
        char path[1024];
        int n = 0;
        SDL_snprintf(path, sizeof path, "%slauncher.png", base ? base : "");
        if (unsigned char* px = stbi_load(path, &pw, &ph, &n, 4)) {
            glGenTextures(1, &pic);
            glBindTexture(GL_TEXTURE_2D, pic);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
            stbi_image_free(px);
            host_log("launcher: picture %s (%dx%d)\n", path, pw, ph);
        }
    }
    SDL_SetWindowTitle(win, "Resident Evil Code: Veronica X - PC Port VR");
    SDL_SetWindowFullscreen(win, false);
    SDL_SetWindowResizable(win, false);
    SDL_SetWindowSize(win, (int)LW, (int)LH);
    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SyncWindow(win);

    while (result < 0) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) result = 0;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.scancode == SDL_SCANCODE_ESCAPE) { if (options) options = false; else result = 0; }
        }
        ImGui::GetStyle().FontScaleDpi = 1.0f;
        turns[0] = L(L_SMOOTH); comforts[0] = L(L_OFF); comforts[1] = L(L_MEDIUM); comforts[2] = L(L_STRONG);
        sights[0] = L(L_LASER); sights[1] = L(L_DOT); sights[2] = L(L_NOSIGHT);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImVec2 ds = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ds);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(11, 10, 13, 255));
        ImGui::Begin("##launcher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float artw = pic ? ds.y * pw / ph : ds.x * 0.42f;   /* the portrait's width at the window's height */
        float rx = artw - 30.0f;                             /* where the right side's content starts */
        {   /* the right side: a faint red glow, as from the game's own menus */
            for (int i = 0; i < 7; i++)
                dl->AddCircleFilled(ImVec2(ds.x * 0.86f, ds.y * 0.12f), ds.y * (0.75f - i * 0.09f), IM_COL32(110, 8, 12, 6 + i * 3), 64);
        }
        if (pic) {   /* the portrait, the whole height, fading into the dark at its right edge */
            dl->AddImage((ImTextureID)(intptr_t)pic, ImVec2(0, 0), ImVec2(artw, ds.y));
            dl->AddRectFilledMultiColor(ImVec2(artw - 170, 0), ImVec2(artw + 2, ds.y), IM_COL32(11, 10, 13, 0), IM_COL32(11, 10, 13, 255), IM_COL32(11, 10, 13, 255), IM_COL32(11, 10, 13, 0));
            dl->AddRectFilledMultiColor(ImVec2(0, ds.y - 120), ImVec2(artw, ds.y), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 170), IM_COL32(0, 0, 0, 170));
        } else {   /* no picture: a dark panel with the red glow */
            dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(artw, ds.y), IM_COL32(30, 6, 8, 255), IM_COL32(11, 10, 13, 255), IM_COL32(11, 10, 13, 255), IM_COL32(45, 8, 10, 255));
        }
        {   /* the title (Tomb Raider's logo strip, here on the right) */
            float x = rx + 20.0f;
            l_text(dl, lf_ui, 15.0f, ImVec2(x + 2, 38), IM_COL32(190, 182, 175, 255), "RESIDENT EVIL", 6.0f);
            l_text(dl, lf_title, 50.0f, ImVec2(x, 58), IM_COL32(245, 240, 232, 255), "CODE: VERONICA X");
            dl->AddRectFilled(ImVec2(x + 2, 120), ImVec2(x + 70, 123), IM_COL32(210, 30, 36, 255));
            l_text(dl, lf_bold, 20.0f, ImVec2(x + 80, 109), IM_COL32(225, 40, 46, 255), "PC PORT VR", 3.0f);
        }
        {   /* vr134: the language, top right - EN PT ES, the chosen one red and underlined (kept in the ini) */
            static const char* const langs[] = { "EN", "PT", "ES" };
            float lx = ds.x - 132.0f;
            ImGui::PushFont(lf_bold, 16.0f);
            for (int k = 0; k < 3; k++) {
                ImVec2 ts = ImGui::CalcTextSize(langs[k]), lp(lx, 16.0f);
                ImGui::SetCursorScreenPos(lp);
                if (ImGui::InvisibleButton(langs[k], ImVec2(ts.x + 6, ts.y + 6)) && g_launcher_lang != k) { g_launcher_lang = k; settings_save_later(); }
                bool hov = ImGui::IsItemHovered();
                dl->AddText(lp, k == g_launcher_lang ? IM_COL32(235, 45, 50, 255) : hov ? IM_COL32(245, 240, 235, 255) : IM_COL32(150, 142, 136, 255), langs[k]);
                if (k == g_launcher_lang) dl->AddRectFilled(ImVec2(lp.x, lp.y + ts.y + 2), ImVec2(lp.x + ts.x, lp.y + ts.y + 4), IM_COL32(220, 35, 40, 255));
                lx += ts.x + 18.0f;
            }
            ImGui::PopFont();
        }
        if (!options) {
            float x = rx + 20.0f, w = ds.x - x - 60.0f;
            if (l_button(L(L_PLAY), ImVec2(x, 190), ImVec2(w, 74), true, 30.0f) || ImGui::IsKeyPressed(ImGuiKey_Enter)) result = 1;
            l_text(dl, lf_ui, 15.0f, ImVec2(x + 2, 272), IM_COL32(170, 160, 155, 255), L(L_HINT));
            if (l_button(L(L_OPTIONS), ImVec2(x, 316), ImVec2((w - 14) * 0.5f, 50), false, 21.0f)) options = true;
            if (l_button(L(L_EXIT), ImVec2(x + (w - 14) * 0.5f + 14, 316), ImVec2((w - 14) * 0.5f, 50), false, 21.0f)) result = 0;
        } else {
            float x = rx + 20.0f, w = ds.x - x - 40.0f;
            ImGui::SetCursorScreenPos(ImVec2(x, 150));
            ImGui::PushFont(lf_ui, 17.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(18, 17, 21, 235));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(90, 30, 34, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(32, 30, 36, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(55, 24, 28, 255));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(75, 22, 26, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(120, 18, 22, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(165, 26, 32, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(195, 32, 38, 255));
            ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(120, 18, 22, 255));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(150, 24, 30, 255));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(180, 30, 36, 255));
            ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(235, 45, 50, 255));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(200, 30, 36, 255));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(240, 50, 56, 255));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(20, 19, 24, 250));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(230, 225, 220, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22, 18));
            ImGui::BeginChild("##opts", ImVec2(w, ds.y - 150 - 96), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
            {   /* the tabs, drawn as Tomb Raider's: words, the chosen one red and underlined */
                const char* tabs[] = { L(L_TAB_VR), L(L_TAB_GFX), L(L_TAB_TEX) };
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImDrawList* cl = ImGui::GetWindowDrawList();
                for (int t = 0; t < 3; t++) {
                    ImGui::PushFont(lf_bold, 19.0f);
                    ImVec2 ts = ImGui::CalcTextSize(tabs[t]);
                    ImGui::SetCursorScreenPos(p);
                    if (ImGui::InvisibleButton(tabs[t], ImVec2(ts.x + 4, ts.y + 8))) tab = t;
                    bool hov = ImGui::IsItemHovered();
                    cl->AddText(p, t == tab ? IM_COL32(235, 45, 50, 255) : hov ? IM_COL32(245, 240, 235, 255) : IM_COL32(150, 142, 136, 255), tabs[t]);
                    if (t == tab) cl->AddRectFilled(ImVec2(p.x, p.y + ts.y + 4), ImVec2(p.x + ts.x, p.y + ts.y + 7), IM_COL32(220, 35, 40, 255));
                    p.x += ts.x + 34;
                    ImGui::PopFont();
                }
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::Separator();
                ImGui::Spacing();
            }
            if (tab == 0) {
                int i;
                for (i = 0; i < 6 && turn_v[i] != g_vr_turn; i++) {}
                if (i == 6) i = 3;
                if (opt_combo(L(L_TURN), turns, 6, &i)) { g_vr_turn = turn_v[i]; settings_save_later(); }
                i = g_vr_comfort <= 0 ? 0 : g_vr_comfort <= 2 ? 1 : 2;
                if (opt_combo(L(L_COMFORT), comforts, 3, &i)) { g_vr_comfort = comfort_v[i]; settings_save_later(); }
                i = g_vr_sight >= 2 ? 0 : g_vr_sight == 1 ? 1 : 2;
                if (opt_combo(L(L_SIGHT), sights, 3, &i)) { g_vr_sight = sight_v[i]; settings_save_later(); }
                opt_label(L(L_VRRES));
                if (ImGui::SliderInt("##res", &g_vr_res, 50, 200, "%d%%")) { g_vr_res = (g_vr_res + 5) / 10 * 10; settings_save_later(); }
            } else if (tab == 1) {
                opt_setting(L(L_FULLSCREEN), "Fullscreen");
                opt_setting(L(L_WINSIZE), "Window size");
                opt_setting(L(L_INTRES), "Internal resolution");
                opt_setting(L(L_VSYNC), "VSync");
                opt_setting(L(L_FPSLIM), "Frame rate limit");
            } else {
                bool hd = g_tex_hd != 0;
                opt_label(L(L_TEXHD));
                if (ImGui::Checkbox("##hd", &hd)) { g_tex_hd = hd; settings_save_later(); }
                /* vr157: no "per frame" limiter - the replacements load in the background (texreplace.c);
                   vr160: how many threads read them, and the upload thread */
                opt_label(L(L_PERFRAME));
                if (ImGui::SliderInt("##rt", &g_tex_loaders, 4, 6)) settings_save_later();
                {
                    static const char* const up_names[3][3] = { { "Automatic", "On", "Off" }, { "Autom\xC3\xA1tico", "Ligado", "Desligado" },
                                                                { "Autom\xC3\xA1tico", "Activado", "Desactivado" } };
                    static const char* const up_label[3] = { "Background upload", "Envio em segundo plano", "Env\xC3\xADo en segundo plano" };
                    int lg = g_launcher_lang >= 0 && g_launcher_lang < 3 ? g_launcher_lang : 0, u = g_tex_upload >= 0 && g_tex_upload < 3 ? g_tex_upload : 0;
                    if (opt_combo(up_label[lg], up_names[lg], 3, &u)) { g_tex_upload = u; settings_save_later(); }
                }
                ImGui::TextDisabled("%s", L(L_PERFRAME_HELP));
            }
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 52);
            if (ImGui::Checkbox(L(L_SHOW), &show)) settings_save_later();
            ImGui::EndChild();
            ImGui::PopStyleVar(4);
            ImGui::PopStyleColor(16);
            ImGui::PopFont();
            if (l_button(L(L_BACK), ImVec2(x, ds.y - 80), ImVec2(180, 46), true, 20.0f)) options = false;
            float dw;
            ImGui::PushFont(lf_bold, 20.0f); dw = ImGui::CalcTextSize(L(L_DEFAULT)).x + 44.0f; ImGui::PopFont();
            if (l_button(L(L_DEFAULT), ImVec2(x + 194, ds.y - 80), ImVec2(dw > 160.0f ? dw : 160.0f, 46), false, 20.0f)) {   /* as the release has them */
                g_vr_turn = 45; g_vr_comfort = 0; g_vr_sight = 2; g_vr_res = 100; g_tex_hd = 1; g_tex_hd_per_frame = 2; g_tex_loaders = 4; g_tex_upload = 0;
                settings_save_later();
            }
        }
        {   /* the foot: a newer release, the version and credits */
            float x = rx + 20.0f;
            if (g_update_newer) {   /* vr144: a green button - under Options/Exit, or at the end of Back/Default */
                char line[128];
                SDL_snprintf(line, sizeof line, L(L_UPDATE), g_update_tag);
                float fs = options ? 17.0f : 19.0f;
                ImGui::PushFont(lf_bold, fs);
                ImVec2 ts = ImGui::CalcTextSize(line);
                ImVec2 sz(ds.x - x - 60.0f, 48.0f), pos(x, 384.0f);
                if (options) {   /* what's left of the row after Default; too narrow for the words = only the version */
                    float dw = ImGui::CalcTextSize(L(L_DEFAULT)).x * 20.0f / fs + 44.0f;
                    pos = ImVec2(x + 194 + (dw > 160.0f ? dw : 160.0f) + 14, ds.y - 80.0f);
                    sz = ImVec2(ds.x - 40.0f - pos.x, 46.0f);
                    if (ts.x + 64.0f > sz.x) { SDL_strlcpy(line, g_update_tag, sizeof line); ts = ImGui::CalcTextSize(line); }
                }
                ImGui::PopFont();
                ImGui::SetCursorScreenPos(pos);
                if (ImGui::InvisibleButton("##update", sz)) SDL_OpenURL("https://github.com/neto1495mn-sudo/recvx-enhanced-port-pcvr/releases/latest");
                bool hov = ImGui::IsItemHovered(), held = ImGui::IsItemActive();
                ImVec2 b(pos.x + sz.x, pos.y + sz.y);
                if (hov) dl->AddRectFilled(ImVec2(pos.x - 4, pos.y - 4), ImVec2(b.x + 4, b.y + 4), IM_COL32(60, 200, 100, 50), 6.0f);   /* glow */
                dl->AddRectFilled(pos, b, hov ? IM_COL32(39, 168, 71, 255) : IM_COL32(31, 138, 58, 255));
                dl->AddRect(pos, b, IM_COL32(111, 224, 140, held ? 255 : 200), 0.0f, 0, 1.5f);
                float cx = pos.x + (sz.x - ts.x - 44.0f) * 0.5f, cy = pos.y + sz.y * 0.5f;
                float a = 0.6f + 0.4f * SDL_sinf((float)ImGui::GetTime() * 4.5f);   /* the dot blinks slowly */
                dl->AddCircleFilled(ImVec2(cx + 4, cy), 4.5f, IM_COL32(184, 255, 203, (int)(a * 255)));
                ImU32 ic = IM_COL32(255, 250, 245, 255);   /* the download arrow, drawn: the fonts have no icons */
                float ix = cx + 25;
                dl->AddLine(ImVec2(ix, cy - 8), ImVec2(ix, cy + 3), ic, 2.0f);
                dl->AddTriangleFilled(ImVec2(ix - 5, cy), ImVec2(ix + 5, cy), ImVec2(ix, cy + 6), ic);
                dl->AddLine(ImVec2(ix - 7, cy + 9), ImVec2(ix + 7, cy + 9), ic, 2.0f);
                l_text(dl, lf_bold, fs, ImVec2(cx + 44, cy - ts.y * 0.5f), ic, line);
            }
            if (!options) {
                char f1[128];
                SDL_snprintf(f1, sizeof f1, L(L_FOOT1), g_vr_version);
                l_text(dl, lf_ui, 14.0f, ImVec2(x, ds.y - 66), IM_COL32(150, 142, 136, 255), f1);
                l_text(dl, lf_ui, 14.0f, ImVec2(x, ds.y - 44), IM_COL32(120, 114, 108, 255), L(L_FOOT2));
            }
        }
        ImGui::End();
        ImGui::Render();
        glViewport(0, 0, (int)ds.x, (int)ds.y);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }
    if (pic) glDeleteTextures(1, &pic);
    g_vr_request = 1;   /* (one PLAY: VR) */
    g_launcher = show;
    config_save();
    if (result) {   /* the game's own window back */
        SDL_SetWindowResizable(win, true);
        SDL_SetWindowTitle(win, "Resident Evil Code: Veronica X");
        settings_apply_window();
        if (!g_config.fullscreen) SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    return result;
}

/* ---- the in-headset frame-rate readout (ini vr_fps, VR settings "Show frame rate"): a small panel drawn with ImGui into its own
   picture (512 x 96, transparent around a dark rounded box), which xr.c shows as a head-locked quad. "Real" is the game's own
   frames per second (30), "Interpolated" what the headset gets (90). Called a couple of times a second, never from inside an
   ImGui frame. Returns the GL texture (0 if the overlay is not ready). */
extern "C" unsigned overlay_fps_panel(int real, int shown, int* w, int* h)
{
    static GLuint fbo, tex;
    static int lang = -1;
    const int PWID = 512, PHEI = 96;
    char line[96];
    if (!ready) return 0;
    if (lang < 0) {   /* the device's language, as the launcher: pt, es, else English */
        int nl = 0;
        SDL_Locale** loc = SDL_GetPreferredLocales(&nl);
        lang = 0;
        if (loc && nl > 0 && loc[0] && loc[0]->language) {
            if (!SDL_strcasecmp(loc[0]->language, "pt")) lang = 1;
            else if (!SDL_strcasecmp(loc[0]->language, "es")) lang = 2;
        }
        SDL_free(loc);
    }
    if (!fbo) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, PWID, PHEI, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    *w = PWID; *h = PHEI;
    SDL_snprintf(line, sizeof line, "Real %d    %s %d", real, lang == 0 ? "Interpolated" : "Interpolado", shown);
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 old_ds = io.DisplaySize, old_fs = io.DisplayFramebufferScale;
    io.DisplaySize = ImVec2((float)PWID, (float)PHEI);
    io.DisplayFramebufferScale = ImVec2(1, 1);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)PWID, (float)PHEI));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    ImGui::Begin("##fpspanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* f = ImGui::GetFont();
        float fs = 34.0f;
        ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.0f, line);
        dl->AddRectFilled(ImVec2(2, 2), ImVec2(PWID - 2.0f, PHEI - 2.0f), IM_COL32(8, 7, 9, 190), 26.0f);
        dl->AddRect(ImVec2(2, 2), ImVec2(PWID - 2.0f, PHEI - 2.0f), IM_COL32(190, 40, 48, 255), 26.0f, 0, 2.5f);
        dl->AddText(f, fs, ImVec2((PWID - ts.x) * 0.5f, (PHEI - ts.y) * 0.5f), IM_COL32(245, 240, 235, 255), line);
    }
    ImGui::End();
    ImGui::Render();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, PWID, PHEI);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    io.DisplaySize = old_ds;
    io.DisplayFramebufferScale = old_fs;
    return tex;
}

#ifdef __ANDROID__
/* ---- Quest: the launcher in the headset. A 16:9 panel on the cinema screen (black all around, fixed in front), the right
   controller points at it (xr_quad_pointer), the trigger clicks. First start: the disc image through the launcher's own
   file browser (the "all files" permission asked first); later: Play / Settings tabs, change the disc, add a folder of
   textures, the update from GitHub. The language is the device's (pt, es, else English). Runs after xr_init, before the
   game starts. The layout follows the approved mockup (1000 px wide there, 1280 here: every size is the mockup's x 1.28);
   it is drawn at twice that resolution so the text is sharp in the headset. */
extern "C" {
const char* xr_headset_name(void);
int xr_frame_begin(void);
void xr_frame_end_quad_rect(int x, int y, int w, int h);
void xr_frame_end_none(void);
int xr_quad_pointer(float* u, float* v, int* down);
void xr_quad_source(unsigned fbo);
float gs_vr_grade_ig(void);
void xr_screen_lock(int on, int angle);
int xr_input(VrInput* out);
int host_disc_ok(void);
int host_disc_set(const char* path);
int and_all_files(void);
void and_all_files_request(void);
int and_can_install(void);
void and_request_install(void);
void and_update_check(const char* cur);
int and_update_state(void);
int and_update_pct(void);
void and_update_download(void);
void and_update_tag(char* out, int n);
extern const char* g_vr_version;
extern char g_tex_dir[1024];
extern const unsigned char g_launcher_umbrella[];
extern const int g_launcher_umbrella_size;
extern const unsigned char g_launcher_band[];
extern const int g_launcher_band_size;
extern const unsigned char g_launcher_font[];
extern const int g_launcher_font_size;
unsigned char* stbi_load_from_memory(const unsigned char* buffer, int len, int* x, int* y, int* comp, int req_comp);
}
enum { Q_PLAY, Q_TAB_PLAY, Q_TAB_SET, Q_READY, Q_CHANGE_DISC, Q_CHOOSE_DISC, Q_DISC_TITLE, Q_DISC_TEXT, Q_DISC_BAD, Q_RES, Q_REC, Q_TURN, Q_COMFORT, Q_SIGHT, Q_HDTEX,
       Q_SMOOTH, Q_OFF, Q_MEDIUM, Q_STRONG, Q_LASER, Q_DOT, Q_NOSIGHT, Q_ON, Q_APPLIED, Q_EXIT, Q_ADD_TEX, Q_TEX_OK, Q_PERM_TITLE, Q_PERM_TEXT, Q_PERM_BTN, Q_BACK, Q_UP,
       Q_USE_FOLDER, Q_EMPTY, Q_UPD_NEW, Q_UPD_GET, Q_UPD_PERM, Q_UPD_DOWN, Q_UPD_INSTALL, Q_UPD_FAIL, Q_UPD_RETRY, Q_PICK_ISO, Q_PICK_DIR, Q_STORAGE, Q_DOCUMENTS, Q_CANCEL, Q_NO_ISO, Q_HINT_ISO, Q_HINT_DIR, Q_HINT_PICK, Q_TAB_DISC, Q_TAB_TEX, Q_HINT_POINT, Q_DISC_PAGE, Q_DISC_OK, Q_TEX_PAGE, Q_F_DEVICE, Q_OKSHORT, Q_N };
static const char* const q_text[3][Q_N] = {
    { "PLAY", "Play", "Settings", "Ready to play", "Change disc", "Choose disc", "Choose the game disc", "Select the .iso of your Resident Evil Code: Veronica X (SLUS-20184). You only do this once.",
      "That file could not be opened. Choose another one.", "Resolution", "recommended", "Turning", "Comfort", "Aim", "HD textures",
      "Smooth", "Off", "Medium", "Strong", "Laser", "Dot", "None", "On", "recommended settings applied", "Exit", "Add textures", "Textures folder added",
      "Allow access to files", "The launcher needs permission to see your files, so you can choose the disc and the textures. On the next screen, turn the option on for this app, then come back.",
      "Allow", "Back", "Up", "Use this folder", "Nothing here", "New version %s available", "Download and install",
      "To install it, allow this app to install updates. Turn the option on, then come back.", "Downloading...", "Confirm the installation in the system window", "The update failed", "Try again",
      "Choose the disc (.iso)", "Choose the textures folder", "Storage", "Documents", "Cancel", "No disc images here", "Open the folders until you find your .iso file.", "This folder is empty. Open another folder, or use this one.", "Tap a .iso file to choose it", "Disc", "Textures", "Point with the right controller and pull the trigger to interact.", "Choose another disc image, for example one in another language.", "Disc ready", "Optional. Add a folder with replacement textures.", "Device", "Ready" },
    { "JOGAR", "Jogar", "Configurações", "Pronto para jogar", "Trocar disco", "Escolher disco", "Escolha o disco do jogo", "Selecione o .iso do seu Resident Evil Code: Veronica X (SLUS-20184). Você só faz isso uma vez.",
      "Esse arquivo não pôde ser aberto. Escolha outro.", "Resolução", "recomendado", "Giro", "Conforto", "Mira", "Texturas HD",
      "Suave", "Desligado", "Médio", "Forte", "Laser", "Ponto", "Sem mira", "Ligado", "configuração recomendada aplicada", "Sair", "Adicionar texturas", "Pasta de texturas adicionada",
      "Permitir acesso aos arquivos", "O launcher precisa de permissão para ver seus arquivos, para você escolher o disco e as texturas. Na próxima tela, ative a opção deste app e volte aqui.",
      "Permitir", "Voltar", "Subir", "Usar esta pasta", "Nada aqui", "Nova versão %s disponível", "Baixar e instalar",
      "Para instalar, permita que este app instale atualizações. Ative a opção e volte aqui.", "Baixando...", "Confirme a instalação na janela do sistema", "A atualização falhou", "Tentar de novo",
      "Escolha o disco (.iso)", "Escolha a pasta de texturas", "Armazenamento", "Documentos", "Cancelar", "Nenhum disco (.iso) aqui", "Abra as pastas até achar o seu arquivo .iso.", "Esta pasta está vazia. Abra outra pasta, ou use esta.", "Toque em um arquivo .iso para escolher", "Disco", "Texturas", "Apontar com o controle direito e puxar o gatilho para interagir.", "Escolha outra imagem de disco, por exemplo de outro idioma.", "Disco pronto", "Opcional. Adicione uma pasta com texturas de substituição.", "Aparelho", "Pronto" },
    { "JUGAR", "Jugar", "Configuración", "Listo para jugar", "Cambiar disco", "Elegir disco", "Elige el disco del juego", "Selecciona el .iso de tu Resident Evil Code: Veronica X (SLUS-20184). Solo lo haces una vez.",
      "No se pudo abrir ese archivo. Elige otro.", "Resolución", "recomendado", "Giro", "Comodidad", "Mira", "Texturas HD",
      "Suave", "Apagado", "Medio", "Fuerte", "Láser", "Punto", "Sin mira", "Activado", "configuración recomendada aplicada", "Salir", "Agregar texturas", "Carpeta de texturas agregada",
      "Permitir acceso a los archivos", "El launcher necesita permiso para ver tus archivos y así elegir el disco y las texturas. En la siguiente pantalla, activa la opción de esta app y vuelve aquí.",
      "Permitir", "Volver", "Subir", "Usar esta carpeta", "Nada aquí", "Nueva versión %s disponible", "Descargar e instalar",
      "Para instalar, permite que esta app instale actualizaciones. Activa la opción y vuelve aquí.", "Descargando...", "Confirma la instalación en la ventana del sistema", "La actualización falló", "Reintentar",
      "Elige el disco (.iso)", "Elige la carpeta de texturas", "Almacenamiento", "Documentos", "Cancelar", "No hay discos (.iso) aquí", "Abre las carpetas hasta encontrar tu archivo .iso.", "Esta carpeta está vacía. Abre otra carpeta, o usa esta.", "Toca un archivo .iso para elegirlo", "Disco", "Texturas", "Apunta con el control derecho y tira del gatillo para interactuar.", "Elige otra imagen de disco, por ejemplo en otro idioma.", "Disco listo", "Opcional. Agrega una carpeta con texturas de reemplazo.", "Dispositivo", "Listo" },
};

/* the launcher's file browser: folders first, then the .iso files (disc) - or only folders (textures) */
struct QEntry { char name[256]; bool dir; double size; };
static struct { char path[1024]; QEntry* e; int n, cap; } qb;
static SDL_EnumerationResult SDLCALL qb_cb(void* ud, const char* dirname, const char* fname)
{
    bool want_files = *(bool*)ud;
    char full[1400];
    struct stat st;   /* (the plain stat: SDL's path info fails on a disc image over 2 GB in this 32-bit build, and the image was left out of the list) */
    if (fname[0] == '.') return SDL_ENUM_CONTINUE;
    SDL_snprintf(full, sizeof full, "%s%s%s", dirname, dirname[SDL_strlen(dirname) - 1] == '/' ? "" : "/", fname);
    if (stat(full, &st) != 0) return SDL_ENUM_CONTINUE;
    bool dir = S_ISDIR(st.st_mode);
    if (!dir) {
        size_t l = SDL_strlen(fname);
        if (!want_files || l < 5 || SDL_strcasecmp(fname + l - 4, ".iso")) return SDL_ENUM_CONTINUE;
    }
    if (qb.n == qb.cap) { qb.cap = qb.cap ? qb.cap * 2 : 128; qb.e = (QEntry*)SDL_realloc(qb.e, qb.cap * sizeof(QEntry)); }
    SDL_strlcpy(qb.e[qb.n].name, fname, sizeof qb.e[qb.n].name);
    qb.e[qb.n].dir = dir;
    qb.e[qb.n].size = (double)st.st_size;
    qb.n++;
    return SDL_ENUM_CONTINUE;
}
static int qb_cmp(const void* a, const void* b)
{
    const QEntry* x = (const QEntry*)a;
    const QEntry* y = (const QEntry*)b;
    if (x->dir != y->dir) return x->dir ? -1 : 1;
    return SDL_strcasecmp(x->name, y->name);
}
static void qb_open(const char* path, bool want_files)
{
    SDL_strlcpy(qb.path, path, sizeof qb.path);
    qb.n = 0;
    SDL_EnumerateDirectory(qb.path, qb_cb, &want_files);
    if (qb.n > 1) qsort(qb.e, qb.n, sizeof(QEntry), qb_cmp);
}
static void qb_up(bool want_files)
{
    char* s = strrchr(qb.path, '/');
    if (!s || s == qb.path || SDL_strlen(qb.path) <= SDL_strlen("/storage/emulated/0")) return;
    char up[1024];
    SDL_strlcpy(up, qb.path, sizeof up);
    up[s - qb.path] = 0;
    qb_open(up, want_files);
}

extern "C" int overlay_launcher_xr(void)
{
    static const float PW = 1260.0f, PH = 760.0f, K = 1.28f, SS = 2.0f;   /* panel size; the mockup's scale; the drawing's oversampling */
    static const int turn_v[] = { 0, 30, 45, 90 };
    static const int comfort_v[] = { 0, 2, 3 };
    static const int sight_v[] = { 2, 1, 0 };
    enum { S_MAIN, S_PERM, S_BROWSE };
    int lang = 0, q3 = 0, rec, tab = 0, bad = 0, done = 0, pw = 0, ph = 0, scr = S_MAIN, browse_mode = 0, scroll = 0, tex_ok = 0, upd_perm = 0, upd_asked = 0;
    float scrollf = 0.0f;
    GLuint pic = 0, fbo = 0, fbo_tex = 0;
    uint64_t t_last = SDL_GetTicksNS();
    const char* hn = xr_headset_name();
    ImFont* font = nullptr;
    if (!ready) return 1;
    {   /* the device's language: pt, es, anything else English */
        int nl = 0;
        SDL_Locale** loc = SDL_GetPreferredLocales(&nl);
        if (loc && nl > 0 && loc[0] && loc[0]->language) {
            if (!SDL_strcasecmp(loc[0]->language, "pt")) lang = 1;
            else if (!SDL_strcasecmp(loc[0]->language, "es")) lang = 2;
        }
        SDL_free(loc);
    }
    q3 = hn && (SDL_strstr(hn, "Quest 3") != NULL || SDL_strstr(hn, "Quest3") != NULL);
    rec = q3 ? 3 : 2;   /* the Quest 2's older chip: 2x; the Quest 3: 3x */
    if (g_config.scale != rec && !(q3 && g_config.scale == rec + 1)) { g_config.scale = rec; config_save(); }   /* the recommended one (on a Quest 3 also one step above it): anything else goes back to it */
    host_log("launcher: headset '%s' language %d recommended resolution %dx\n", hn ? hn : "", lang, rec);
    {   /* the system's own font (Roboto) */
        static const char* const paths[] = { "/system/fonts/Roboto-Regular.ttf", "/system/fonts/RobotoStatic-Regular.ttf", "/system/fonts/DroidSans.ttf" };
        static const ImWchar ranges[] = { 0x20, 0x17F, 0x2000, 0x206F, 0xB0, 0xB7, 0 };
        for (const char* p : paths) {
            if (FILE* f = fopen(p, "rb")) { fclose(f); font = ImGui::GetIO().Fonts->AddFontFromFileTTF(p, 32.0f, nullptr, ranges); if (font) break; }
        }
    }
    ImFont* fontC = nullptr;   /* Chakra Petch SemiBold for the titles and buttons: a technical lettering, not too round */
    {
        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = false;
        fontC = ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)g_launcher_font, g_launcher_font_size, 32.0f, &fc, nullptr);
    }
    GLuint texUmb = 0, texBand = 0;   /* the Umbrella emblem and the bar-code band, cut from the reference card */
    int umbW = 1, umbH = 1, bandW = 1, bandH = 1;
    {
        struct { const unsigned char* d; int n; GLuint* t; int* w; int* h; } im[2] = { { g_launcher_umbrella, g_launcher_umbrella_size, &texUmb, &umbW, &umbH }, { g_launcher_band, g_launcher_band_size, &texBand, &bandW, &bandH } };
        for (int k = 0; k < 2; k++) {
            int nn = 0;
            unsigned char* px = stbi_load_from_memory(im[k].d, im[k].n, im[k].w, im[k].h, &nn, 4);
            if (!px) continue;
            for (int q = 0; q < *im[k].w * *im[k].h; q++)   /* the headset's grade, as on the game (once, here) */
                for (int ch = 0; ch < 3; ch++) px[q * 4 + ch] = (unsigned char)(255.0f * powf(px[q * 4 + ch] / 255.0f, gs_vr_grade_ig()) + 0.5f);
            glGenTextures(1, im[k].t);
            glBindTexture(GL_TEXTURE_2D, *im[k].t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *im[k].w, *im[k].h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
            stbi_image_free(px);
        }
    }
    {   /* the panel is drawn into its own picture at twice the size */
        glGenTextures(1, &fbo_tex);
        glBindTexture(GL_TEXTURE_2D, fbo_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)(PW * SS), (int)(PH * SS), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { host_log("launcher: the panel's picture cannot be made\n"); fbo = 0; }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetStyle().FontScaleDpi = 1.0f;
    io.DisplayFramebufferScale = ImVec2(SS, SS);
    xr_screen_lock(1, 42);   /* fixed in front, like the headset's own panels */
    and_update_check(g_vr_version ? g_vr_version : "0.0.0");
    while (!done) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) if (ev.type == SDL_EVENT_QUIT) exit(0);   /* (not passed on to ImGui: the controller is the pointer) */
        int fr = xr_frame_begin();
        if (fr != 1) { if (fr == 0) xr_frame_end_none(); continue; }
        uint64_t now = SDL_GetTicksNS();
        float dt = (float)(now - t_last) * 1e-9f;
        t_last = now;
        if (dt < 0.001f) dt = 0.001f;
        if (dt > 0.1f) dt = 0.1f;
        float mu = 0, mv = 0;
        int down = 0, point = xr_quad_pointer(&mu, &mv, &down);
        VrInput vin;
        float stick = xr_input(&vin) ? (fabsf(vin.ry) > fabsf(vin.ly) ? vin.ry : vin.ly) : 0.0f;
        io.DisplaySize = ImVec2(PW, PH);
        io.DeltaTime = dt;
        if (point) io.AddMousePosEvent(mu * PW, mv * PH); else io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        io.AddMouseButtonEvent(0, point && down);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(PW, PH));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(8, 7, 9, 255));
        ImGui::Begin("##qlauncher", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImFont* F = font ? font : ImGui::GetFont();
        const char* const* T = q_text[lang];
        const float topH = 112.0f;
        const bool disc = host_disc_ok() != 0;
        const ImU32 cWhite = IM_COL32(245, 240, 235, 255), cGray = IM_COL32(154, 146, 139, 255), cDim = IM_COL32(122, 114, 107, 255), cRed = IM_COL32(224, 48, 56, 255);
        const ImU32 cBtn = IM_COL32(19, 17, 15, 255), cBtnLine = IM_COL32(106, 90, 82, 255), cPlay = IM_COL32(168, 35, 42, 255);
        const float FS = 1.15f;               /* every text size is scaled by this: the headset needs bigger letters */
        ImFont* FR = F;                       /* Roboto: paragraphs, paths, file names */
        ImFont* FC = fontC ? fontC : F;       /* Chakra Petch SemiBold: titles, tabs, buttons */
        auto text = [&](float x, float y, float sz, ImU32 c, const char* s) { dl->AddText(FC, sz * FS, ImVec2(x, y), c, s); };
        auto tw = [&](float sz, const char* s) { return FC->CalcTextSizeA(sz * FS, FLT_MAX, 0.0f, s).x; };
        auto textR = [&](float x, float y, float sz, ImU32 c, const char* s) { dl->AddText(FR, sz * FS, ImVec2(x, y), c, s); };
        auto twR = [&](float sz, const char* s) { return FR->CalcTextSizeA(sz * FS, FLT_MAX, 0.0f, s).x; };
        /* a button: (x, y, w, h) in panel pixels, fs the font size, bg the fill, line the border */
        auto button = [&](const char* id, const char* label, float x, float y, float w, float h, float fs, ImU32 bg, ImU32 line, float lw, float rad) -> bool {
            ImGui::SetCursorPos(ImVec2(x, y));
            ImGui::InvisibleButton(id, ImVec2(w, h));
            bool hov = ImGui::IsItemHovered(), clicked = ImGui::IsItemDeactivated() && hov;
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), bg, rad);
            if (hov) dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), IM_COL32(255, 80, 80, 40), rad);
            dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), hov ? IM_COL32(255, 96, 104, 255) : line, rad, 0, lw);
            if (label[0]) text(x + (w - tw(fs, label)) * 0.5f, y + (h - fs) * 0.5f - 1.0f, fs, cWhite, label);
            return clicked;
        };
        /* where the file browser starts */
        auto browse_start = [&](int mode) {
            if (!and_all_files()) { scr = S_PERM; browse_mode = mode; return; }
            static const char* const starts[] = { "/storage/emulated/0/Download", "/storage/emulated/0" };
            browse_mode = mode; scroll = 0; scrollf = 0.0f; scr = S_BROWSE;
            qb_open(starts[0], mode == 1);
            if (qb.n == 0 && !SDL_GetPathInfo(starts[0], nullptr)) qb_open(starts[1], mode == 1);
        };
        {   /* test aid (env.txt RECVX_LAUNCHER_SCR = perm / browse / browse_dir / settings): starts on that screen, for the pictures */
            static int once;
            const char* ts = SDL_getenv("RECVX_LAUNCHER_SCR");
            if (ts && !once++) {
                if (!SDL_strcmp(ts, "settings")) tab = 3;
                else if (!SDL_strcmp(ts, "tab1")) tab = 1;
                else if (!SDL_strcmp(ts, "tab2")) tab = 2;
                else if (!SDL_strcmp(ts, "perm")) { scr = S_PERM; browse_mode = 1; }
                else if (!SDL_strcmp(ts, "browse")) browse_start(1);
                else if (!SDL_strcmp(ts, "browse_iso")) { browse_start(1); qb_open("/storage/emulated/0/Download/MoonlightXR", true); }
                else if (!SDL_strcmp(ts, "browse_dir")) browse_start(2);
            }
        }
        /* the background: a dark red glow behind the panel's middle */
        for (int i = 0; i < 7; i++) dl->AddCircleFilled(ImVec2(PW * 0.62f, PH * 0.5f), 560.0f - i * 70.0f, IM_COL32(110, 8, 14, 6 + i * 3), 64);
        /* the top bar: the title (two colours) on the left, the four tabs on the right, a hairline under it */
        {   /* the game's name, large and heavy */
            float x = 40.0f;
            text(x, 24.0f, 46.0f, cRed, "RE");
            x += tw(46.0f, "RE") + 20.0f;
            text(x, 24.0f, 46.0f, cWhite, "CODE: VERONICA X");
            x += tw(46.0f, "CODE: VERONICA X") + 20.0f;
            text(x, 24.0f, 46.0f, cRed, "VR");
            x += tw(46.0f, "VR") + 14.0f;
            text(x, 56.0f, 14.0f, cWhite, "STANDALONE");
        }
        if (disc && scr == S_MAIN) {
            const char* tabs[4] = { T[Q_TAB_PLAY], T[Q_TAB_DISC], T[Q_TAB_TEX], T[Q_TAB_SET] };
            float tx = PW - 40.0f;
            for (int k = 3; k >= 0; k--) {
                float w = tw(20.0f, tabs[k]) + 40.0f;
                tx -= w;
                bool on = tab == k;
                char tid[16];
                SDL_snprintf(tid, sizeof tid, "##tab%d", k);
                if (button(tid, tabs[k], tx, 32.0f, w, 46.0f, 20.0f, on ? IM_COL32(58, 26, 24, 255) : IM_COL32(24, 21, 21, 255), on ? cRed : IM_COL32(52, 46, 46, 255), 1.5f, 8.0f)) tab = k;
                tx -= 8.0f;
            }
        }
        dl->AddLine(ImVec2(40.0f, topH), ImVec2(PW - 40.0f, topH), IM_COL32(43, 35, 38, 255), 1.0f);
        const float cx = scr == S_MAIN ? 600.0f : 60.0f, right = PW - 50.0f;
        const float by = 150.0f;   /* the body's top: level with the top of the emblem */
        if (scr == S_PERM) {   /* the "all files" permission, then on to the browser */
            text(cx, by + 20.0f, 36.0f, cWhite, T[Q_PERM_TITLE]);
            dl->AddText(FR, FS * 20.0f, ImVec2(cx, by + 80.0f), cGray, T[Q_PERM_TEXT], nullptr, right - cx);
            if (and_all_files()) browse_start(browse_mode);
            else if (button("##perm", T[Q_PERM_BTN], cx, by + 200.0f, 260.0f, 56.0f, 22.0f, cPlay, cRed, 1.5f, 8.0f)) and_all_files_request();
            if (button("##permback", T[Q_BACK], cx + 280.0f, by + 200.0f, 200.0f, 56.0f, 22.0f, cBtn, cBtnLine, 1.5f, 8.0f)) scr = S_MAIN;
        } else if (scr == S_BROWSE) {   /* the file browser: back button + clickable path, shortcuts on the left, the list, a scroll bar */
            const bool files = browse_mode == 1;
            static const char* const ROOT = "/storage/emulated/0";
            const float hdrY = by + 30.0f, hdrH = 50.0f, sideW = 210.0f, listX = cx + sideW + 18.0f, listW = right - listX - 22.0f;
            const float top = hdrY + hdrH + 14.0f, bot = PH - 100.0f, rowH = 52.0f;
            const int vis = (int)((bot - top) / rowH);
            const bool atRoot = SDL_strlen(qb.path) <= SDL_strlen(ROOT);
            if (fabsf(stick) > 0.25f) scrollf -= stick * dt * 16.0f;   /* the thumbstick scrolls */
            if (scrollf > qb.n - vis) scrollf = (float)(qb.n - vis);
            if (scrollf < 0) scrollf = 0;
            scroll = (int)scrollf;
            text(cx, by - 8.0f, 30.0f, cWhite, files ? T[Q_PICK_ISO] : T[Q_PICK_DIR]);   /* what is being chosen: the disc (.iso) or a folder of textures */
            /* the header: back, then the path as clickable pieces */
            if (button("##bkb", "", cx, hdrY, 56.0f, hdrH, 20.0f, cBtn, cBtnLine, 1.5f, 10.0f)) {
                if (atRoot) scr = S_MAIN; else { qb_up(files); scrollf = 0; }
            }
            {   /* the arrow, drawn */
                float ax = cx + 28.0f, ay = hdrY + hdrH * 0.5f;
                dl->AddLine(ImVec2(ax + 9.0f, ay), ImVec2(ax - 9.0f, ay), cWhite, 3.0f);
                dl->AddLine(ImVec2(ax - 9.0f, ay), ImVec2(ax - 1.0f, ay - 8.0f), cWhite, 3.0f);
                dl->AddLine(ImVec2(ax - 9.0f, ay), ImVec2(ax - 1.0f, ay + 8.0f), cWhite, 3.0f);
            }
            {
                float px = cx + 70.0f;
                char acc[1024];
                SDL_strlcpy(acc, ROOT, sizeof acc);
                const char* rest = qb.path + (SDL_strlen(qb.path) >= SDL_strlen(ROOT) ? SDL_strlen(ROOT) : SDL_strlen(qb.path));
                std::vector<std::pair<std::string, std::string>> crumbs;   /* (label, full path) */
                crumbs.push_back({ T[Q_STORAGE], ROOT });
                while (*rest == '/') {
                    const char* e = strchr(rest + 1, '/');
                    std::string seg(rest + 1, e ? (size_t)(e - rest - 1) : strlen(rest + 1));
                    SDL_strlcat(acc, "/", sizeof acc);
                    SDL_strlcat(acc, seg.c_str(), sizeof acc);
                    crumbs.push_back({ seg, acc });
                    rest = e ? e : rest + strlen(rest);
                }
                size_t from = 0;   /* too many pieces: the first ones are left out */
                float total = 0;
                for (size_t k = 0; k < crumbs.size(); k++) total += twR(19.0f, crumbs[k].first.c_str()) + 36.0f;
                while (crumbs.size() - from > 1 && total > right - px - 170.0f) { total -= twR(19.0f, crumbs[from].first.c_str()) + 36.0f; from++; }
                if (from) { textR(px, hdrY + 14.0f, 19.0f, cDim, "…  >"); px += 56.0f; }
                for (size_t k = from; k < crumbs.size(); k++) {
                    bool last = k + 1 == crumbs.size();
                    float w = twR(19.0f, crumbs[k].first.c_str());
                    char id[24];
                    SDL_snprintf(id, sizeof id, "##crumb%d", (int)k);
                    ImGui::SetCursorPos(ImVec2(px - 6.0f, hdrY));
                    ImGui::InvisibleButton(id, ImVec2(w + 12.0f, hdrH));
                    bool hov = ImGui::IsItemHovered(), clk = ImGui::IsItemDeactivated() && hov;
                    textR(px, hdrY + (hdrH - 19.0f) * 0.5f - 1.0f, 19.0f, last ? cWhite : (hov ? cWhite : cGray), crumbs[k].first.c_str());
                    if (hov && !last) dl->AddLine(ImVec2(px, hdrY + hdrH - 10.0f), ImVec2(px + w, hdrY + hdrH - 10.0f), cRed, 2.0f);
                    if (clk && !last) { qb_open(crumbs[k].second.c_str(), files); scrollf = 0; break; }
                    px += w;
                    if (!last) { textR(px + 10.0f, hdrY + (hdrH - 19.0f) * 0.5f - 1.0f, 19.0f, cDim, ">"); px += 36.0f; }
                }
            }
            /* the shortcuts */
            {
                struct Sc { const char* label; const char* path; };
                const Sc sc[3] = { { T[Q_STORAGE], "/storage/emulated/0" }, { "Download", "/storage/emulated/0/Download" }, { T[Q_DOCUMENTS], "/storage/emulated/0/Documents" } };
                for (int k = 0; k < 3; k++) {
                    float y = top + k * 56.0f;
                    char id[24];
                    SDL_snprintf(id, sizeof id, "##sc%d", k);
                    ImGui::SetCursorPos(ImVec2(cx, y));
                    ImGui::InvisibleButton(id, ImVec2(sideW, 48.0f));
                    bool hov = ImGui::IsItemHovered(), clk = ImGui::IsItemDeactivated() && hov, cur = !SDL_strcmp(qb.path, sc[k].path);
                    dl->AddRectFilled(ImVec2(cx, y), ImVec2(cx + sideW, y + 48.0f), cur ? IM_COL32(58, 26, 24, 255) : (hov ? IM_COL32(40, 28, 28, 255) : IM_COL32(19, 17, 15, 255)), 9.0f);
                    if (cur) dl->AddRect(ImVec2(cx, y), ImVec2(cx + sideW, y + 48.0f), cRed, 9.0f, 0, 1.5f);
                    dl->AddRectFilled(ImVec2(cx + 14.0f, y + 17.0f), ImVec2(cx + 36.0f, y + 33.0f), IM_COL32(196, 54, 62, 255), 3.0f);   /* a folder */
                    dl->AddRectFilled(ImVec2(cx + 14.0f, y + 13.0f), ImVec2(cx + 24.0f, y + 19.0f), IM_COL32(196, 54, 62, 255), 2.0f);
                    textR(cx + 48.0f, y + (48.0f - 18.0f) * 0.5f - 1.0f, 18.0f, cWhite, sc[k].label);
                    if (clk) { qb_open(sc[k].path, files); scrollf = 0; }
                }
            }
            /* the list */
            dl->AddRectFilled(ImVec2(listX, top), ImVec2(listX + listW, bot), IM_COL32(14, 12, 12, 255), 10.0f);
            for (int i = 0; i < vis && scroll + i < qb.n; i++) {
                const QEntry& e = qb.e[scroll + i];
                float y = top + i * rowH;
                char id[24];
                SDL_snprintf(id, sizeof id, "##e%d", i);
                ImGui::SetCursorPos(ImVec2(listX, y));
                ImGui::InvisibleButton(id, ImVec2(listW, rowH));
                bool hov = ImGui::IsItemHovered(), clicked = ImGui::IsItemDeactivated() && hov;
                if (hov) dl->AddRectFilled(ImVec2(listX + 4.0f, y + 2.0f), ImVec2(listX + listW - 4.0f, y + rowH - 2.0f), IM_COL32(58, 26, 24, 255), 8.0f);
                else if ((scroll + i) & 1) dl->AddRectFilled(ImVec2(listX + 4.0f, y + 2.0f), ImVec2(listX + listW - 4.0f, y + rowH - 2.0f), IM_COL32(20, 18, 18, 255), 8.0f);
                float iy = y + rowH * 0.5f;
                if (e.dir) {   /* a folder icon */
                    dl->AddRectFilled(ImVec2(listX + 20.0f, iy - 11.0f), ImVec2(listX + 44.0f, iy + 11.0f), IM_COL32(196, 54, 62, 255), 4.0f);
                    dl->AddRectFilled(ImVec2(listX + 20.0f, iy - 15.0f), ImVec2(listX + 32.0f, iy - 8.0f), IM_COL32(196, 54, 62, 255), 3.0f);
                } else {   /* a disc icon */
                    dl->AddCircleFilled(ImVec2(listX + 32.0f, iy), 13.0f, IM_COL32(232, 226, 220, 255), 24);
                    dl->AddCircleFilled(ImVec2(listX + 32.0f, iy), 7.0f, IM_COL32(196, 54, 62, 255), 16);
                    dl->AddCircleFilled(ImVec2(listX + 32.0f, iy), 4.5f, IM_COL32(14, 12, 12, 255), 16);
                }
                {   /* the name, cut with "..." before it reaches the size or the chevron */
                    char nm[300];
                    float avail = listW - 62.0f - (e.dir ? 60.0f : 170.0f);
                    SDL_strlcpy(nm, e.name, sizeof nm);
                    if (twR(20.0f, nm) > avail) {
                        size_t l = SDL_strlen(nm);
                        while (l > 4 && twR(20.0f, nm) > avail) { nm[--l] = 0; if (l > 3) { nm[l - 1] = '.'; nm[l - 2] = '.'; nm[l - 3] = '.'; } }
                    }
                    textR(listX + 62.0f, iy - 10.0f, 20.0f, cWhite, nm);
                }
                if (e.dir) {   /* a chevron */
                    float xx = listX + listW - 34.0f;
                    dl->AddLine(ImVec2(xx, iy - 8.0f), ImVec2(xx + 8.0f, iy), cGray, 2.5f);
                    dl->AddLine(ImVec2(xx + 8.0f, iy), ImVec2(xx, iy + 8.0f), cGray, 2.5f);
                } else {
                    char sz[32];
                    double mb = e.size / 1048576.0;
                    if (mb >= 1024.0) SDL_snprintf(sz, sizeof sz, "%.2f GB", mb / 1024.0); else SDL_snprintf(sz, sizeof sz, "%.0f MB", mb);
                    textR(listX + listW - 30.0f - twR(17.0f, sz), iy - 9.0f, 17.0f, cGray, sz);
                }
                if (clicked) {
                    char np[1400];
                    SDL_snprintf(np, sizeof np, "%s/%s", qb.path, e.name);
                    if (e.dir) { qb_open(np, files); scrollf = 0; break; }
                    if (host_disc_set(np) > 0) { scr = S_MAIN; bad = 0; } else bad = 1;
                    break;
                }
            }
            if (qb.n == 0) {
                textR(listX + 30.0f, top + 28.0f, 22.0f, cGray, files ? T[Q_NO_ISO] : T[Q_EMPTY]);
                dl->AddText(FR, FS * 17.0f, ImVec2(listX + 30.0f, top + 64.0f), cDim, files ? T[Q_HINT_ISO] : T[Q_HINT_DIR], nullptr, listW - 60.0f);
            }
            if (qb.n > vis) {   /* the scroll bar: a track, a thumb, a page up / page down at its ends */
                float tx = listX + listW + 6.0f, tt = top + 40.0f, tb = bot - 40.0f, th = fmaxf(36.0f, (tb - tt) * vis / qb.n);
                float frac = qb.n > vis ? scrollf / (qb.n - vis) : 0.0f;
                dl->AddRectFilled(ImVec2(tx, tt), ImVec2(tx + 12.0f, tb), IM_COL32(30, 26, 26, 255), 6.0f);
                dl->AddRectFilled(ImVec2(tx, tt + (tb - tt - th) * frac), ImVec2(tx + 12.0f, tt + (tb - tt - th) * frac + th), IM_COL32(150, 60, 62, 255), 6.0f);
                if (button("##sup", "", tx - 8.0f, top, 28.0f, 36.0f, 16.0f, cBtn, cBtnLine, 1.0f, 6.0f)) scrollf -= vis - 1;
                if (button("##sdn", "", tx - 8.0f, bot - 36.0f, 28.0f, 36.0f, 16.0f, cBtn, cBtnLine, 1.0f, 6.0f)) scrollf += vis - 1;
                dl->AddTriangleFilled(ImVec2(tx + 6.0f, top + 12.0f), ImVec2(tx - 1.0f, top + 22.0f), ImVec2(tx + 13.0f, top + 22.0f), cWhite);
                dl->AddTriangleFilled(ImVec2(tx - 1.0f, bot - 22.0f), ImVec2(tx + 13.0f, bot - 22.0f), ImVec2(tx + 6.0f, bot - 12.0f), cWhite);
            }
            /* the footer: cancel, and in the textures mode the folder being looked at */
            const float fy = PH - 82.0f;
            if (button("##bcancel", T[Q_CANCEL], cx, fy, 190.0f, 54.0f, 20.0f, cBtn, cBtnLine, 1.5f, 9.0f)) scr = S_MAIN;
            if (!files) {
                if (button("##buse", T[Q_USE_FOLDER], right - 360.0f, fy, 360.0f, 54.0f, 21.0f, cPlay, cRed, 2.0f, 9.0f)) {
                    SDL_strlcpy(g_tex_dir, qb.path, sizeof g_tex_dir);
                    g_tex_hd = 1; config_save(); tex_ok = 1; scr = S_MAIN;
                }
            } else textR(right - twR(17.0f, T[Q_HINT_PICK]), fy + 18.0f, 17.0f, cDim, T[Q_HINT_PICK]);
        } else {   /* the main screen: the picture and the hint on the left, the tab's page on the right (the reference image) */
            const char* dev = hn && hn[0] ? (SDL_strstr(hn, "Quest2") ? "Quest 2" : hn) : "Quest";
            {   /* the Umbrella emblem on the left, the card's own "Security Clearance" bar-code band along the bottom */
                if (texUmb) dl->AddImage((ImTextureID)(intptr_t)texUmb, ImVec2(112.0f, 157.0f), ImVec2(112.0f + 376.0f, 157.0f + 376.0f * umbH / umbW));
                if (texBand) {
                    float bw = 640.0f, bh = bw * bandH / bandW;
                    dl->AddImage((ImTextureID)(intptr_t)texBand, ImVec2((PW - bw) * 0.5f, PH - bh - 44.0f), ImVec2((PW + bw) * 0.5f, PH - 44.0f));
                }
            }
            if (!disc) {   /* first start: only the disc */
                text(cx, by + 20.0f, 42.0f, cWhite, T[Q_DISC_TITLE]);
                dl->AddText(FR, FS * 21.0f, ImVec2(cx, by + 84.0f), cGray, T[Q_DISC_TEXT], nullptr, right - cx);
                if (bad) text(cx, by + 190.0f, 19.0f, IM_COL32(240, 120, 110, 255), T[Q_DISC_BAD]);
                if (button("##disc", T[Q_CHOOSE_DISC], cx, by + 230.0f, 330.0f, 72.0f, 26.0f, cPlay, cRed, 2.0f, 12.0f)) browse_start(1);
            } else if (tab == 0) {   /* play */
                char sub[160];
                text(cx, by, 44.0f, cWhite, T[Q_READY]);
                SDL_snprintf(sub, sizeof sub, "%s", T[Q_APPLIED]);
                text(cx, by + 58.0f, 20.0f, cGray, sub);
                {   /* the card's lines: label in grey, value in white */
                    char v1[64], v2[64];
                    SDL_snprintf(v1, sizeof v1, "%dx", g_config.scale ? g_config.scale : rec);
                    SDL_snprintf(v2, sizeof v2, "%s", T[Q_OKSHORT]);
                    const char* lab[2] = { T[Q_RES], T[Q_TAB_DISC] };
                    const char* val[2] = { v1, v2 };
                    for (int k = 0; k < 2; k++) {
                        float fy = by + 100.0f + k * 38.0f;
                        text(cx, fy, 20.0f, cGray, lab[k]);
                        text(cx + 190.0f, fy, 20.0f, cWhite, val[k]);
                        dl->AddLine(ImVec2(cx, fy + 36.0f), ImVec2(right, fy + 36.0f), IM_COL32(38, 32, 30, 255), 1.0f);
                    }
                }
                float y = by + 206.0f;
                {   /* the update */
                    int us = and_update_state();
                    char tag[64], msg[160];
                    and_update_tag(tag, sizeof tag);
                    if (us == 3 || us == 4 || us == 5 || us == 6) {
                        float bw = right - cx, bh = 62.0f;
                        dl->AddRectFilled(ImVec2(cx, y), ImVec2(cx + bw, y + bh), us == 6 ? IM_COL32(70, 22, 24, 255) : IM_COL32(18, 52, 30, 255), 12.0f);
                        dl->AddRect(ImVec2(cx, y), ImVec2(cx + bw, y + bh), us == 6 ? IM_COL32(200, 70, 70, 255) : IM_COL32(60, 150, 90, 255), 12.0f, 0, 1.5f);
                        dl->AddCircleFilled(ImVec2(cx + 26.0f, y + bh * 0.5f), 7.0f, us == 6 ? IM_COL32(240, 110, 100, 255) : IM_COL32(60, 220, 110, 255), 16);
                        const float ty = y + (bh - 20.0f) * 0.5f - 1.0f;
                        if (us == 3) {
                            SDL_snprintf(msg, sizeof msg, T[Q_UPD_NEW], tag);
                            text(cx + 46.0f, ty, 20.0f, cWhite, msg);
                            const char* bl = upd_perm ? T[Q_PERM_BTN] : T[Q_UPD_GET];
                            float w = tw(18.0f, bl) + 40.0f;
                            if (button("##upd", bl, cx + bw - w - 12.0f, y + 10.0f, w, bh - 20.0f, 18.0f, IM_COL32(20, 90, 40, 255), IM_COL32(111, 224, 140, 255), 1.5f, 8.0f)) {
                                if (!and_can_install()) { upd_perm = 1; and_request_install(); }
                                else { upd_perm = 0; and_update_download(); }
                            }
                            if (upd_perm) { if (and_can_install()) upd_perm = 0; else dl->AddText(FR, FS * 16.0f, ImVec2(cx + 46.0f, y + bh + 6.0f), cGray, T[Q_UPD_PERM], nullptr, bw); }
                        } else if (us == 4) {
                            SDL_snprintf(msg, sizeof msg, "%s %d%%", T[Q_UPD_DOWN], and_update_pct());
                            text(cx + 46.0f, ty, 20.0f, cWhite, msg);
                        } else if (us == 5) text(cx + 46.0f, ty, 20.0f, cWhite, T[Q_UPD_INSTALL]);
                        else {
                            text(cx + 46.0f, ty, 20.0f, cWhite, T[Q_UPD_FAIL]);
                            float w = tw(18.0f, T[Q_UPD_RETRY]) + 40.0f;
                            if (button("##updr", T[Q_UPD_RETRY], cx + bw - w - 12.0f, y + 10.0f, w, bh - 20.0f, 18.0f, IM_COL32(80, 20, 20, 255), cWhite, 1.5f, 8.0f)) and_update_download();
                        }
                        y += bh + (upd_perm && us == 3 ? 40.0f : 22.0f);
                    }
                    (void)upd_asked;
                }
                {   /* the PLAY button: its bottom level with the emblem's, a gradient and a bevel */
                    const float py = 530.0f - 104.0f, ph2 = 104.0f;   /* (the emblem's bottom: 150 + 380) */
                    for (int g = 0; g < 16; g++) {   /* a smooth gradient: 16 clipped strips, light at the top, dark at the bottom */
                        float t = g / 15.0f;
                        dl->PushClipRect(ImVec2(cx, py + ph2 * g / 16.0f), ImVec2(right, py + ph2 * (g + 1) / 16.0f + 0.5f), true);
                        dl->AddRectFilled(ImVec2(cx, py), ImVec2(right, py + ph2), IM_COL32((int)(196 - 70 * t), (int)(42 - 18 * t), (int)(50 - 18 * t), 255), 14.0f);
                        dl->PopClipRect();
                    }
                    dl->AddRect(ImVec2(cx + 3.0f, py + 3.0f), ImVec2(right - 3.0f, py + ph2 - 3.0f), IM_COL32(255, 140, 140, 70), 12.0f, 0, 1.5f);
                    if (button("##play", T[Q_PLAY], cx, py, right - cx, ph2, 46.0f, IM_COL32(0, 0, 0, 0), cRed, 3.0f, 14.0f)) done = 1;
                }
            } else if (tab == 1) {   /* disc */
                text(cx, by, 44.0f, cWhite, T[Q_TAB_DISC]);
                dl->AddText(FR, FS * 21.0f, ImVec2(cx, by + 62.0f), cGray, T[Q_DISC_PAGE], nullptr, right - cx);
                text(cx, by + 130.0f, 22.0f, IM_COL32(111, 224, 140, 255), T[Q_DISC_OK]);
                if (bad) text(cx, by + 170.0f, 19.0f, IM_COL32(240, 120, 110, 255), T[Q_DISC_BAD]);
                if (button("##change", T[Q_CHANGE_DISC], cx, by + 210.0f, 330.0f, 72.0f, 26.0f, cPlay, cRed, 2.0f, 12.0f)) browse_start(1);
            } else if (tab == 2) {   /* textures */
                text(cx, by, 44.0f, cWhite, T[Q_TAB_TEX]);
                dl->AddText(FR, FS * 21.0f, ImVec2(cx, by + 62.0f), cGray, T[Q_TEX_PAGE], nullptr, right - cx);
                if (tex_ok || g_tex_dir[0]) text(cx, by + 130.0f, 22.0f, IM_COL32(111, 224, 140, 255), T[Q_TEX_OK]);
                if (button("##addtex", T[Q_ADD_TEX], cx, by + 210.0f, 330.0f, 72.0f, 26.0f, cPlay, cRed, 2.0f, 12.0f)) browse_start(2);
            } else {   /* settings */
                float y = by;
                const float rowH = 76.0f;   /* five rows = the emblem's height (380) */
                char id[32];
                auto row = [&](const char* label) {
                    text(cx, y + (rowH - 21.0f) * 0.5f - 1.0f, 21.0f, cWhite, label);
                    dl->AddLine(ImVec2(cx, y + rowH), ImVec2(right, y + rowH), IM_COL32(38, 32, 30, 255), 1.0f);
                };
                auto seg = [&](const char* sid, const char* label, float x, float w, bool on, bool recd) -> bool {
                    return button(sid, label, x, y + (rowH - 44.0f) * 0.5f, w, 44.0f, 19.0f, on ? IM_COL32(58, 26, 24, 255) : IM_COL32(0, 0, 0, 0), on ? cRed : (recd ? IM_COL32(47, 180, 87, 255) : IM_COL32(90, 74, 69, 255)), 1.5f, 8.0f);
                };
                {   /* resolution: 1x, 2x (and 3x on a Quest 3), the recommended one with a green border */
                    row(T[Q_RES]);
                    float x = right;
                    for (int v = q3 ? rec + 1 : rec; v >= rec; v--) {   /* only the recommended resolution (and, on a stronger headset, the one above it) */
                        char lb[48];
                        if (v == rec) SDL_snprintf(lb, sizeof lb, "%dx  %s", v, T[Q_REC]); else SDL_snprintf(lb, sizeof lb, "%dx", v);
                        float w = tw(19.0f, lb) + 32.0f;
                        x -= w;
                        SDL_snprintf(id, sizeof id, "##res%d", v);
                        if (seg(id, lb, x, w, g_config.scale == v, v == rec)) { g_config.scale = v; config_save(); }
                        x -= 8.0f;
                    }
                    y += rowH;
                }
                {   /* turning */
                    const char* lb[4] = { T[Q_SMOOTH], "30°", "45°", "90°" };
                    row(T[Q_TURN]);
                    float x = right;
                    for (int v = 3; v >= 0; v--) {
                        float w = tw(19.0f, lb[v]) + 32.0f;
                        x -= w;
                        SDL_snprintf(id, sizeof id, "##turn%d", v);
                        if (seg(id, lb[v], x, w, g_vr_turn == turn_v[v], false)) { g_vr_turn = turn_v[v]; config_save(); }
                        x -= 8.0f;
                    }
                    y += rowH;
                }
                {   /* comfort */
                    const char* lb[3] = { T[Q_OFF], T[Q_MEDIUM], T[Q_STRONG] };
                    row(T[Q_COMFORT]);
                    float x = right;
                    for (int v = 2; v >= 0; v--) {
                        float w = tw(19.0f, lb[v]) + 32.0f;
                        x -= w;
                        SDL_snprintf(id, sizeof id, "##comf%d", v);
                        if (seg(id, lb[v], x, w, g_vr_comfort == comfort_v[v], false)) { g_vr_comfort = comfort_v[v]; config_save(); }
                        x -= 8.0f;
                    }
                    y += rowH;
                }
                {   /* aim */
                    const char* lb[3] = { T[Q_LASER], T[Q_DOT], T[Q_NOSIGHT] };
                    row(T[Q_SIGHT]);
                    float x = right;
                    for (int v = 2; v >= 0; v--) {
                        float w = tw(19.0f, lb[v]) + 32.0f;
                        x -= w;
                        SDL_snprintf(id, sizeof id, "##sight%d", v);
                        if (seg(id, lb[v], x, w, g_vr_sight == sight_v[v], false)) { g_vr_sight = sight_v[v]; config_save(); }
                        x -= 8.0f;
                    }
                    y += rowH;
                }
                {   /* HD textures */
                    const char* lb[2] = { T[Q_OFF], T[Q_ON] };
                    row(T[Q_HDTEX]);
                    float x = right;
                    for (int v = 1; v >= 0; v--) {
                        float w = tw(19.0f, lb[v]) + 32.0f;
                        x -= w;
                        SDL_snprintf(id, sizeof id, "##hd%d", v);
                        if (seg(id, lb[v], x, w, (g_tex_hd != 0) == (v != 0), false)) { g_tex_hd = v; config_save(); }
                        x -= 8.0f;
                    }
                }
            }
        }
        {   /* the window's rounded corners: the corners outside the rounding are painted black (the world around it is black) */
            const float R = 62.0f;
            for (int c = 0; c < 4; c++) {
                float sx = (c & 1) ? -1.0f : 1.0f, sy = (c & 2) ? -1.0f : 1.0f;
                ImVec2 C((c & 1) ? PW : 0.0f, (c & 2) ? PH : 0.0f), O(C.x + sx * R, C.y + sy * R), prev(C.x, C.y + sy * R);
                for (int i = 1; i <= 24; i++) {
                    float th = 1.5707963f * i / 24.0f;
                    ImVec2 q(O.x - sx * R * cosf(th), O.y - sy * R * sinf(th));
                    dl->AddTriangleFilled(C, prev, q, IM_COL32(0, 0, 0, 255));
                    prev = q;
                }
            }
        }
        if (scr == S_MAIN) {   /* bottom right: the version, level with the bottom of the bar-code band */
            char ft[96];
            SDL_snprintf(ft, sizeof ft, "v%s  ·  90 Hz", g_vr_version ? g_vr_version : "");
            text(PW - 50.0f - tw(15.0f, ft), PH - 44.0f - 24.0f, 15.0f, cDim, ft);
        }
        if (point) {   /* where the controller points */
            ImVec2 m(mu * PW, mv * PH);
            dl->AddCircleFilled(m, 8.0f, IM_COL32(111, 224, 140, down ? 255 : 200));
            dl->AddCircle(m, 11.0f, IM_COL32(20, 60, 30, 230), 24, 2.0f);
        }
        ImGui::End();
        ImGui::Render();
        {   /* the headset's grade on every colour the panel draws (the fonts are white masks tinted by these) */
            static unsigned char lut[256];
            static float lut_ig = -1.0f;
            float ig = gs_vr_grade_ig();
            if (ig != lut_ig) { lut_ig = ig; for (int q = 0; q < 256; q++) lut[q] = (unsigned char)(255.0f * powf(q / 255.0f, ig) + 0.5f); }
            ImDrawData* dd = ImGui::GetDrawData();
            for (int ln = 0; ln < dd->CmdListsCount; ln++) {
                ImDrawList* l = dd->CmdLists[ln];
                for (int vi = 0; vi < l->VtxBuffer.Size; vi++) {
                    unsigned char* c = (unsigned char*)&l->VtxBuffer[vi].col;   /* ABGR in memory: R, G, B, A */
                    c[0] = lut[c[0]]; c[1] = lut[c[1]]; c[2] = lut[c[2]];
                }
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, (int)(PW * SS), (int)(PH * SS));
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        xr_quad_source(fbo);
        xr_frame_end_quad_rect(0, 0, (int)(PW * SS), (int)(PH * SS));
        xr_quad_source(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        {   /* test aid (env.txt RECVX_LAUNCHER_SHOT=1): the panel as a picture, once - launcher_shot.bmp in the app's folder */
            static int nshot;
            if (SDL_getenv("RECVX_LAUNCHER_SHOT") && ++nshot == 90) {
                unsigned char* px = (unsigned char*)SDL_malloc(2520 * 1520 * 4);   /* (not static: the game library must fit the PS2 address window, android/loader.c) */
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glReadPixels(0, 0, 2520, 1520, GL_RGBA, GL_UNSIGNED_BYTE, px);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                if (FILE* f = fopen("launcher_shot.bmp", "wb")) {
                    unsigned char hd[54] = { 'B', 'M' };
                    unsigned int sz = 54 + 2520 * 1520 * 3, off = 54, hs = 40, w = 2520, h = 1520;
                    unsigned short pl = 1, bp = 24;
                    memcpy(hd + 2, &sz, 4); memcpy(hd + 10, &off, 4); memcpy(hd + 14, &hs, 4); memcpy(hd + 18, &w, 4); memcpy(hd + 22, &h, 4); memcpy(hd + 26, &pl, 2); memcpy(hd + 28, &bp, 2);
                    fwrite(hd, 1, 54, f);
                    for (int y = 0; y < 1520; y++)
                        for (int x = 0; x < 2520; x++) { const unsigned char* q = px + (y * 2520 + x) * 4; unsigned char bgr[3] = { q[2], q[1], q[0] }; fwrite(bgr, 1, 3, f); }
                    fclose(f);
                    host_log("launcher: picture written (launcher_shot.bmp)\n");
                }
            }
        }
        SDL_GL_SwapWindow(win);
    }
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    if (texUmb) glDeleteTextures(1, &texUmb);
    if (texBand) glDeleteTextures(1, &texBand);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (fbo_tex) glDeleteTextures(1, &fbo_tex);
    xr_screen_lock(0, 0);
    g_vr_request = 1;
    config_save();
    return 1;
}
#endif
