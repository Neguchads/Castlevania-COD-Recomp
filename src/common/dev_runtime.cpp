#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <cctype>
#include <toml++/toml.hpp>
#include "app/app.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "dev_runtime.h"
#include "dev_rules.h"
#include "menu_input.h"
#include "guest_memory_guard.h"

namespace kernel { bool VerifyReadOnlyOverlay(const std::string& path); }

namespace dev {
namespace fs = std::filesystem;
namespace {
std::array<std::atomic<bool>, 256> keys{};
std::atomic<int> binding_capture{-1}, mouse_x{0}, mouse_y{0};
std::atomic<bool> focus{false};
HWND window = nullptr;
std::mutex report_lock, mods_lock, toast_lock;
std::mutex input_lock;
dev_rules::MenuKeyPulse native_menu_key;
fs::path report_dir;
bool screenshot_pending = false;
std::string report_status;
std::string toast_message;
std::chrono::steady_clock::time_point toast_expiry{};
std::vector<Mod> mods;
std::vector<Cheat> cheats;
std::vector<std::string> enabled_mods;
struct UserProfile {std::string name,data;};
std::array<UserProfile,3> user_profiles{};
int selected_profile=0;
std::vector<Binding> bindings{
    {"Confirmar / pular (A)", VK_SPACE, 0}, {"Voltar / esquivar (B)", 'E', 1},
    {"Ataque (X)", VK_LBUTTON, 2}, {"Ataque especial (Y)", VK_RBUTTON, 3},
    {"Botao preto", 'Q', 4}, {"Botao branco", 'R', 5},
    {"Gatilho esquerdo", VK_SHIFT, 6}, {"Gatilho direito", VK_CONTROL, 7},
    {"Start / menu", VK_RETURN, 8}, {"Back / mapa", VK_TAB, 9},
    {"Movimento: frente",'W',20},{"Movimento: tras",'S',21},
    {"Movimento: esquerda",'A',22},{"Movimento: direita",'D',23},
    {"Camera: cima",VK_UP,24},{"Camera: baixo",VK_DOWN,25},
    {"Camera: esquerda",VK_LEFT,26},{"Camera: direita",VK_RIGHT,27},
};
bool Pressed(int key) { return key > 0 && key < 256 && keys[size_t(key)].load(); }
void ClearInput() {
    for (auto& key : keys) key.store(false);
    mouse_x.store(0); mouse_y.store(0);
}
bool SafeChild(const fs::path& root, const fs::path& path) {
    std::error_code ec;
    auto canonical_root = fs::weakly_canonical(root, ec);
    if (ec) return false;
    auto canonical_path = fs::weakly_canonical(path, ec);
    if (ec) return false;
    auto a = canonical_root.begin(), b = canonical_path.begin();
    for (; a != canonical_root.end(); ++a, ++b)
        if (b == canonical_path.end() || *a != *b) return false;
    return true;
}
bool ReadValue(const Cheat& cheat, uint32_t& value) {
    if (!dev_rules::CheatAddress(cheat.address, cheat.width) || !kernel::g_base) return false;
    SIZE_T count = 0;
    value = 0;
    return ReadProcessMemory(GetCurrentProcess(), kernel::g_base + cheat.address, &value,
                             size_t(cheat.width), &count) && count == size_t(cheat.width);
}
bool WriteValue(const Cheat& cheat, uint32_t value) {
    MEMORY_BASIC_INFORMATION info{};
    void* address = kernel::g_base + cheat.address;
    if (!VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
        !(info.Protect & (PAGE_READWRITE | PAGE_WRITECOPY))) return false;
    SIZE_T count = 0;
    return WriteProcessMemory(GetCurrentProcess(), address, &value, size_t(cheat.width), &count) &&
           count == size_t(cheat.width);
}
}

fs::path Root() { return app::ExeDir(); }
bool GameFocused() { return window && GetForegroundWindow() == window; }
std::vector<Binding> Bindings() { std::lock_guard lock(input_lock); return bindings; }
void BeginBinding(int index) { binding_capture.store(index); }
void SetBinding(int index, int key) {
    std::lock_guard lock(input_lock);
    if (index >= 0 && size_t(index) < bindings.size() && key > 0 && key < 256 &&
        key != VK_F1 && key != VK_F8 && key != VK_F9 && key != VK_ESCAPE) bindings[size_t(index)].key = key;
}
void ResetBindings() {
    constexpr int defaults[]{VK_SPACE,'E',VK_LBUTTON,VK_RBUTTON,'Q','R',VK_SHIFT,VK_CONTROL,VK_RETURN,VK_TAB,
        'W','S','A','D',VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT};
    for(size_t i=0;i<std::size(defaults);++i) SetBinding(int(i),defaults[i]);
    invert_mouse_x.store(true);invert_mouse_y.store(false);sensitivity.store(.012f);
}
std::vector<Mod>& Mods() { return mods; }
std::vector<Cheat>& Cheats() { return cheats; }
fs::path CurrentReport() { std::lock_guard lock(report_lock); return report_dir; }
std::string ReportStatus() { std::lock_guard lock(report_lock); return report_status; }
void NotifyToast(const std::string& message) {
    std::lock_guard lock(toast_lock);
    toast_message=message;
    toast_expiry=std::chrono::steady_clock::now()+std::chrono::seconds(5);
}
std::string ToastMessage() {
    std::lock_guard lock(toast_lock);
    if(std::chrono::steady_clock::now()>=toast_expiry){toast_message.clear();return {};}
    return toast_message;
}

void Load(const void* raw) {
    const auto& table = *static_cast<const toml::table*>(raw);
    auto& appearance=menu_preferences;
    appearance.x=table["menu_x"].value_or(40.0f);appearance.y=table["menu_y"].value_or(40.0f);
    appearance.width=table["menu_width"].value_or(1120.0f);appearance.height=table["menu_height"].value_or(760.0f);
    appearance.opacity=table["menu_opacity"].value_or(.88f);
    appearance.positioned=table["menu_positioned"].value_or(false);
    for(size_t i=0;i<3;++i) {
        appearance.background[i]=table["menu_background_"+std::to_string(i)].value_or(appearance.background[i]);
        appearance.accent[i]=table["menu_accent_"+std::to_string(i)].value_or(appearance.accent[i]);
        appearance.highlight[i]=table["menu_highlight_"+std::to_string(i)].value_or(appearance.highlight[i]);
    }
    appearance.Sanitize();
    volume.store(std::clamp(table["master_volume"].value_or(.8f), 0.0f, 1.0f));
    balance.store(std::clamp(table["audio_balance"].value_or(0.0f), -1.0f, 1.0f));
    muted.store(table["muted"].value_or(false));
    keyboard.store(table["keyboard_enabled"].value_or(true));
    mouse_camera.store(table["mouse_camera"].value_or(true));
    invert_mouse_x.store(table["invert_mouse_x"].value_or(true));
    invert_mouse_y.store(table["invert_mouse_y"].value_or(false));
    controller_shortcut.store(table["controller_menu_shortcut"].value_or(true));
    minimap_enabled.store(table["minimap_enabled"].value_or(true));
    minimap_rotating.store(table["minimap_rotating"].value_or(true));
    ptbr_enabled.store(table["ptbr_enabled"].value_or(false));
    const float map_scale=table["minimap_scale"].value_or(1.0f);
    const float map_opacity=table["minimap_opacity"].value_or(.8f);
    minimap_scale.store(std::isfinite(map_scale) ? std::clamp(map_scale,.6f,1.6f) : 1.0f);
    minimap_opacity.store(std::isfinite(map_opacity) ? std::clamp(map_opacity,.2f,1.0f) : .8f);
    hud_enabled.store(table["hud_enabled"].value_or(true));
    hud_quick_hide.store(table["hud_quick_hide"].value_or(false));
    const float interface_scale=table["hud_scale"].value_or(.5f);
    const float hide_delay=table["hud_hide_delay"].value_or(1.0f);
    hud_scale.store(std::isfinite(interface_scale) ? std::clamp(interface_scale,.35f,1.0f) : .5f);
    // Experimento de escala adiado por pedido do usuario: menu original intacto.
    game_menu_scale.store(1.0f);
    game_menu_live.store(false);
    hud_hide_delay.store(std::isfinite(hide_delay) ? std::clamp(hide_delay,.2f,5.0f) : 1.0f);
    camera_mode.store(std::clamp(table["camera_mode"].value_or(0),0,5));
    auto setting=[&](const char* key,float fallback,float minimum,float maximum) {
        const float value=table[key].value_or(fallback);
        return std::isfinite(value) ? std::clamp(value,minimum,maximum) : fallback;
    };
    camera_distance.store(setting("camera_distance",1,.5f,2.5f));
    camera_height.store(setting("camera_height",0,-200,300));
    camera_field.store(setting("camera_field",1,.65f,1.5f));
    camera_fly_speed.store(setting("camera_fly_speed",600,100,2000));
    orbit_smoothing.store(setting("orbit_smoothing",8,1,20));
    sensitivity.store(std::clamp(table["mouse_sensitivity"].value_or(.012f), .001f, .1f));
    shader.store(std::clamp(table["post_shader"].value_or(0), 0, 2));
    godmode.store(table["godmode"].value_or(false));
    const float saved_speed=table["game_speed"].value_or(1.0f);
    speed.store(std::isfinite(saved_speed)?std::clamp(saved_speed,.5f,2.0f):1.0f);
    selected_profile=std::clamp(table["selected_profile"].value_or(0),0,4);
    for(size_t i=0;i<user_profiles.size();++i) {
        if(auto name=table["profile_name_"+std::to_string(i)].value<std::string>())user_profiles[i].name=*name;
        if(auto data=table["profile_data_"+std::to_string(i)].value<std::string>())user_profiles[i].data=*data;
    }
    enabled_mods.clear();
    if (const auto* array = table["enabled_mods"].as_array())
        for (const auto& node : *array) if (auto name = node.value<std::string>()) enabled_mods.push_back(*name);
    for (size_t i = 0; i < bindings.size(); ++i)
        SetBinding(int(i), table["bind_" + std::to_string(i)].value_or(bindings[i].key));
}
void Save(void* raw) {
    auto& table = *static_cast<toml::table*>(raw);
    menu_preferences.Sanitize();
    const auto& appearance=menu_preferences;
    table.insert_or_assign("menu_x",appearance.x);table.insert_or_assign("menu_y",appearance.y);
    table.insert_or_assign("menu_width",appearance.width);table.insert_or_assign("menu_height",appearance.height);
    table.insert_or_assign("menu_opacity",appearance.opacity);table.insert_or_assign("menu_positioned",appearance.positioned);
    for(size_t i=0;i<3;++i) {
        table.insert_or_assign("menu_background_"+std::to_string(i),appearance.background[i]);
        table.insert_or_assign("menu_accent_"+std::to_string(i),appearance.accent[i]);
        table.insert_or_assign("menu_highlight_"+std::to_string(i),appearance.highlight[i]);
    }
    table.insert_or_assign("master_volume", volume.load());
    table.insert_or_assign("audio_balance", balance.load());
    table.insert_or_assign("muted", muted.load());
    table.insert_or_assign("keyboard_enabled", keyboard.load());
    table.insert_or_assign("mouse_camera", mouse_camera.load());
    table.insert_or_assign("invert_mouse_x",invert_mouse_x.load());
    table.insert_or_assign("invert_mouse_y",invert_mouse_y.load());
    table.insert_or_assign("controller_menu_shortcut", controller_shortcut.load());
    table.insert_or_assign("minimap_enabled",minimap_enabled.load());
    table.insert_or_assign("minimap_rotating",minimap_rotating.load());
    table.insert_or_assign("ptbr_enabled",ptbr_enabled.load());
    table.insert_or_assign("game_menu_scale",game_menu_scale.load());
    table.insert_or_assign("game_menu_live",game_menu_live.load());
    table.insert_or_assign("minimap_scale",minimap_scale.load());
    table.insert_or_assign("minimap_opacity",minimap_opacity.load());
    table.insert_or_assign("hud_enabled",hud_enabled.load());
    table.insert_or_assign("hud_scale",hud_scale.load());
    table.insert_or_assign("hud_quick_hide",hud_quick_hide.load());
    table.insert_or_assign("hud_hide_delay",hud_hide_delay.load());
    table.insert_or_assign("camera_mode",camera_mode.load());
    table.insert_or_assign("camera_distance",camera_distance.load());
    table.insert_or_assign("camera_height",camera_height.load());
    table.insert_or_assign("camera_field",camera_field.load());
    table.insert_or_assign("camera_fly_speed",camera_fly_speed.load());
    table.insert_or_assign("orbit_smoothing",orbit_smoothing.load());
    table.insert_or_assign("mouse_sensitivity", sensitivity.load());
    table.insert_or_assign("post_shader", shader.load());
    table.insert_or_assign("godmode",godmode.load());
    table.insert_or_assign("game_speed",speed.load());
    table.insert_or_assign("selected_profile",selected_profile);
    for(size_t i=0;i<user_profiles.size();++i) {
        table.insert_or_assign("profile_name_"+std::to_string(i),user_profiles[i].name);
        table.insert_or_assign("profile_data_"+std::to_string(i),user_profiles[i].data);
    }
    toml::array enabled;
    { std::lock_guard lock(mods_lock); for (const auto& mod : mods) if (mod.enabled) enabled.push_back(mod.name); }
    table.insert_or_assign("enabled_mods", enabled);
    const auto current_bindings = Bindings();
    for (size_t i = 0; i < current_bindings.size(); ++i) table.insert_or_assign("bind_" + std::to_string(i), current_bindings[i].key);
}
std::array<std::string,3> UserProfileNames() {
    return {user_profiles[0].name,user_profiles[1].name,user_profiles[2].name};
}
int SelectedProfile() {return selected_profile;}
void SetSelectedProfile(int selection) {selected_profile=std::clamp(selection,0,4);}
bool SaveUserProfile(int slot,const std::string& requested_name) {
    if(slot<0 || slot>=int(user_profiles.size()))return false;
    std::string name=requested_name;
    while(!name.empty() && std::isspace(static_cast<unsigned char>(name.back())))name.pop_back();
    const auto first=name.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)return false;
    name=name.substr(first,64);
    toml::table snapshot;Save(&snapshot);
    toml::array active_cheats;
    for(const auto& cheat:Cheats())if(cheat.enabled)active_cheats.push_back(cheat.name);
    snapshot.insert_or_assign("active_cheats",active_cheats);
    auto& settings=app::Get();
    snapshot.insert_or_assign("display_mode",settings.display_mode==app::DisplayMode::Fullscreen?"fullscreen":
        settings.display_mode==app::DisplayMode::Windowed?"windowed":"borderless");
    snapshot.insert_or_assign("width",settings.width);snapshot.insert_or_assign("height",settings.height);
    snapshot.insert_or_assign("render_percent",settings.render_percent);snapshot.insert_or_assign("fps_cap",settings.fps_cap);
    snapshot.insert_or_assign("vsync",settings.vsync);snapshot.insert_or_assign("anisotropic_filtering",settings.aniso);
    for(size_t i=0;i<user_profiles.size();++i) {
        snapshot.erase("profile_name_"+std::to_string(i));snapshot.erase("profile_data_"+std::to_string(i));
    }
    snapshot.erase("selected_profile");
    std::ostringstream serialized;serialized<<snapshot;
    user_profiles[size_t(slot)]={name,serialized.str()};selected_profile=slot+2;
    return !user_profiles[size_t(slot)].data.empty();
}
namespace {
bool ApplySavedProfile(const UserProfile& profile) {
    if(profile.name.empty() || profile.data.empty())return false;
    try {
        auto snapshot=toml::parse(profile.data);
        Load(&snapshot);
        if(!godmode_ready.load())godmode.store(false);
        auto& settings=app::Get();
        const std::string mode=snapshot["display_mode"].value_or(std::string{"borderless"});
        settings.display_mode=mode=="fullscreen"?app::DisplayMode::Fullscreen:
            mode=="windowed"?app::DisplayMode::Windowed:app::DisplayMode::Borderless;
        settings.width=snapshot["width"].value_or(0u);settings.height=snapshot["height"].value_or(0u);
        settings.render_percent=snapshot["render_percent"].value_or(100u);
        settings.fps_cap=snapshot["fps_cap"].value_or(60u);settings.vsync=snapshot["vsync"].value_or(true);
        settings.aniso=snapshot["anisotropic_filtering"].value_or(0u);
        app::ApplyDisplay();ReloadMods();SetSpeed(speed.load());
        const auto* active=snapshot["active_cheats"].as_array();
        for(auto& cheat:Cheats()) {
            ToggleCheat(cheat,false);
            if(active && std::any_of(active->begin(),active->end(),[&](const toml::node& node) {
                return node.value<std::string>().value_or("")==cheat.name;
            }))ToggleCheat(cheat,true);
        }
        SelectTextLanguage(ptbr_enabled.load()?2:1);
        return true;
    } catch(const toml::parse_error&) {return false;}
}
void RestoreVanilla() {
    menu_preferences=MenuPreferences{};menu_geometry_reset=true;
    app::Get()=app::Settings{};app::ApplyDisplay();
    toml::table defaults;
    defaults.insert_or_assign("master_volume",.8f);defaults.insert_or_assign("audio_balance",0.0f);
    defaults.insert_or_assign("muted",false);defaults.insert_or_assign("keyboard_enabled",true);
    defaults.insert_or_assign("mouse_camera",true);defaults.insert_or_assign("invert_mouse_x",true);
    defaults.insert_or_assign("invert_mouse_y",false);defaults.insert_or_assign("controller_menu_shortcut",true);
    defaults.insert_or_assign("minimap_enabled",false);defaults.insert_or_assign("minimap_rotating",false);
    defaults.insert_or_assign("minimap_scale",1.0f);defaults.insert_or_assign("minimap_opacity",.8f);
    defaults.insert_or_assign("ptbr_enabled",false);defaults.insert_or_assign("hud_enabled",true);
    defaults.insert_or_assign("hud_scale",1.0f);defaults.insert_or_assign("hud_quick_hide",false);
    defaults.insert_or_assign("hud_hide_delay",1.0f);defaults.insert_or_assign("camera_mode",0);
    defaults.insert_or_assign("camera_distance",1.0f);defaults.insert_or_assign("camera_height",0.0f);
    defaults.insert_or_assign("camera_field",1.0f);defaults.insert_or_assign("camera_fly_speed",600.0f);
    defaults.insert_or_assign("orbit_smoothing",8.0f);defaults.insert_or_assign("mouse_sensitivity",.012f);
    defaults.insert_or_assign("post_shader",0);defaults.insert_or_assign("game_speed",1.0f);
    defaults.insert_or_assign("godmode",false);defaults.insert_or_assign("enabled_mods",toml::array{});
    Load(&defaults);ResetBindings();
    for(auto& cheat:Cheats())ToggleCheat(cheat,false);
    ReloadMods();SetSpeed(1.0f);SelectTextLanguage(1);selected_profile=0;
}
void ActivateDevProfile() {
    minimap_enabled=true;minimap_rotating=true;minimap_scale=1.0f;minimap_opacity=.8f;
    hud_enabled=true;hud_scale=.5f;hud_quick_hide=true;hud_hide_delay=1.0f;
    camera_mode=0;shader=0;ptbr_enabled=true;
    godmode.store(godmode_ready.load());SetSpeed(1.0f);
    for(auto& cheat:Cheats())ToggleCheat(cheat,false);
    SelectTextLanguage(2);selected_profile=1;
}
}
bool ApplyProfile(int selection) {
    selection=std::clamp(selection,0,4);
    if(selection==0)RestoreVanilla();
    else if(selection==1)ActivateDevProfile();
    else if(!ApplySavedProfile(user_profiles[size_t(selection-2)]))return false;
    selected_profile=selection;app::Save();return true;
}
bool VerifyProfileStorage() {
    const auto saved=user_profiles[0];const int old_selection=selected_profile;
    auto saved_cheats=Cheats();Cheats().push_back(Cheat{"Profile roundtrip",0,0,0,0,4,true,false,"Ativo"});
    const bool stored=SaveUserProfile(0,"  Roundtrip profile  ");
    bool ok=stored&&user_profiles[0].name=="Roundtrip profile";
    try {
        const auto parsed=toml::parse(user_profiles[0].data);
        const auto* active=parsed["active_cheats"].as_array();
        const bool retained_test_cheat=active&&std::any_of(active->begin(),active->end(),[](const toml::node& node) {
            return node.value<std::string>().value_or("")=="Profile roundtrip";
        });
        ok=ok&&std::abs(parsed["master_volume"].value_or(-1.0f)-volume.load())<.001f&&
            parsed["camera_mode"].value_or(-1)==camera_mode.load()&&
            retained_test_cheat&&
            !parsed.contains("profile_data_0")&&!parsed.contains("selected_profile");
    } catch(const toml::parse_error&) {ok=false;}
    const bool rejected=!SaveUserProfile(1,"   ");
    user_profiles[0]=saved;selected_profile=old_selection;Cheats()=std::move(saved_cheats);
    LOGR("dev selftest: user profile save/serialization/no recursion=%d",ok&&rejected);
    return ok&&rejected;
}
void ReloadMods() {
    std::lock_guard lock(mods_lock);
    mods.clear();
    std::error_code ec;
    fs::create_directories(Root()/"mods", ec);
    for (const auto& entry : fs::directory_iterator(Root()/"mods", ec)) {
        if (!entry.is_directory(ec) || !SafeChild(Root()/"mods", entry.path())) continue;
        std::string name = entry.path().filename().string();
        mods.push_back({name, entry.path(), std::find(enabled_mods.begin(), enabled_mods.end(), name) != enabled_mods.end()});
    }
    std::sort(mods.begin(), mods.end(), [](const Mod& a, const Mod& b) { return a.name < b.name; });
    texture_packs.store(std::any_of(mods.begin(),mods.end(),[](const Mod& mod){return mod.enabled;}));
    texture_generation.fetch_add(1);
}
void EnableMod(size_t index, bool enabled) {
    std::lock_guard lock(mods_lock);
    if (index >= mods.size()) return;
    mods[index].enabled = enabled;
    enabled_mods.clear();
    for (const auto& mod : mods) if (mod.enabled) enabled_mods.push_back(mod.name);
    texture_packs.store(!enabled_mods.empty()); texture_generation.fetch_add(1);
}
std::optional<fs::path> ModTexture(uint64_t hash) {
    char name[32];std::snprintf(name,sizeof(name),"%016llX.bmp",static_cast<unsigned long long>(hash));
    std::lock_guard lock(mods_lock);
    std::error_code ec;
    for(auto it=mods.rbegin();it!=mods.rend();++it) if(it->enabled) {
        fs::path path=it->root/"textures"/name;
        if(SafeChild(it->root/"textures",path) && fs::is_regular_file(path,ec)) return path;
    }
    return std::nullopt;
}
std::optional<fs::path> ModFile(const fs::path& relative) {
    if (!dev_rules::RelativeAsset(relative)) return std::nullopt;
    std::lock_guard lock(mods_lock);
    std::error_code ec;
    for (auto it = mods.rbegin(); it != mods.rend(); ++it) {
        if (!it->enabled) continue;
        fs::path candidate = it->root / "files" / relative;
        if (SafeChild(it->root/"files", candidate) && fs::is_regular_file(candidate, ec)) return candidate;
    }
    return std::nullopt;
}
void ReloadCheats() {
    for (auto& cheat : cheats) if (cheat.enabled) ToggleCheat(cheat, false);
    cheats.clear();
    try {
        auto table = toml::parse_file((Root()/"cheats.toml").string());
        if (table["xbe_sha256"].value_or(std::string()) !=
            "4bc21b87ce7bb39ec02beebf78fd2f5b59c234187fb9a62ac0a5ce4f8b0f9b7a") return;
        if (const auto* rows = table["cheats"].as_array()) for (const auto& node : *rows) {
            const auto* item = node.as_table();
            if (!item) continue;
            const auto& row = *item;
            Cheat cheat;
            cheat.name = row["name"].value_or(std::string("Cheat"));
            auto address = row["address"].value<int64_t>(), value = row["value"].value<int64_t>();
            auto expected = row["expected"].value<int64_t>();
            cheat.width = row["width"].value_or(4);
            if (!address || !value || !expected || *address < 0 || *address > UINT32_MAX ||
                *value < 0 || *value > UINT32_MAX || *expected < 0 || *expected > UINT32_MAX) continue;
            cheat.address = uint32_t(*address); cheat.value = uint32_t(*value); cheat.expected = uint32_t(*expected);
            if (!dev_rules::CheatAddress(cheat.address, cheat.width)) continue;
            if ((cheat.value | cheat.expected) & ~dev_rules::WidthMask(cheat.width)) continue;
            cheat.status = "Desativado";
            cheats.push_back(cheat);
        }
    } catch (const toml::parse_error&) { }
}
bool ToggleCheat(Cheat& cheat, bool enabled) {
    uint32_t current = 0;
    if (!enabled) {
        if (cheat.applied && ReadValue(cheat, current) && current == cheat.value) WriteValue(cheat, cheat.original);
        cheat.enabled = cheat.applied = false; cheat.status = "Desativado";
        return true;
    }
    if (!ReadValue(cheat, current) || current != cheat.expected || !WriteValue(cheat, cheat.value)) {
        cheat.status = "Bloqueado: valor/endereco nao corresponde ao perfil";
        return false;
    }
    cheat.original = current; cheat.applied = cheat.enabled = true; cheat.status = "Ativo";
    return true;
}
void TickCheats() {
    for (auto& cheat : cheats) if (cheat.enabled) {
        uint32_t current = 0;
        if (!ReadValue(cheat, current) || !WriteValue(cheat, cheat.value)) {
            cheat.enabled = false;
            cheat.status = "Desativado: memoria indisponivel";
        }
    }
}
void CaptureBug() {
    ShutdownInput();
    std::lock_guard lock(report_lock);
    SYSTEMTIME now; GetLocalTime(&now);
    char name[96];
    std::snprintf(name, sizeof(name), "%04u%02u%02u-%02u%02u%02u-%llu", now.wYear, now.wMonth, now.wDay,
                  now.wHour, now.wMinute, now.wSecond, static_cast<unsigned long long>(GetTickCount64()));
    report_dir = Root()/"reports"/name;
    std::error_code ec;
    fs::create_directories(report_dir, ec);
    screenshot_pending = !ec;
    report_status = ec ? "Falha ao criar a pasta do relato" : "Capturando o proximo quadro...";
    menu_open.store(true);
}
std::optional<fs::path> TakeScreenshotRequest() {
    std::lock_guard lock(report_lock);
    if (!screenshot_pending) return std::nullopt;
    screenshot_pending = false;
    return report_dir;
}
void ScreenshotDone(bool ok) {
    {
        std::lock_guard lock(report_lock);
        report_status = ok ? "Imagem salva. Descreva o que aconteceu." : "A imagem falhou; o texto ainda pode ser salvo.";
    }
    if (ConsumeTestRequest()) {
        bool saved=SaveReport("Autoteste do relato", "Teste local: captura da GPU, texto e log. Nenhum envio externo.");
        LOGR("dev selftest: screenshot=%d report=%d",ok,saved);
    }
}
bool SaveReport(const std::string& title, const std::string& text) {
    std::lock_guard lock(report_lock);
    if (report_dir.empty()) return false;
    std::ofstream out(report_dir/"report.txt", std::ios::binary);
    out << "Castlevania: Curse of Darkness | Xbox USA\nXBE SHA-256: "
           "4bc21b87ce7bb39ec02beebf78fd2f5b59c234187fb9a62ac0a5ce4f8b0f9b7a\n\n"
        << "Titulo: " << title << "\n\nRelato:\n" << text << "\n\n"
        << "Render: " << app::Get().render_percent << "% | FPS cap: " << app::Get().fps_cap
        << " | shader: " << shader.load() << " | volume: " << volume.load()
        << " | speed: " << speed.load() << "x | godmode: " << godmode.load()
        << " | blocked-damage: " << prevented_damage.load() << "\n"
        << "Output: " << app::Get().width << "x" << app::Get().height
        << " | aniso: " << app::Get().aniso << " | vsync: " << app::Get().vsync << "\n"
        << "Menu: " << menu_preferences.width << "x" << menu_preferences.height
        << " at " << menu_preferences.x << "," << menu_preferences.y
        << " | opacity: " << menu_preferences.opacity << "\n";
    out.close();
    if (!out) { report_status = "Falha ao salvar o relato"; return false; }
    logx::Flush();
    std::error_code ec;
    fs::copy_file(logx::Path(), report_dir/"runtime.log", fs::copy_options::overwrite_existing, ec);
    report_status = ec ? "Texto salvo; nao foi possivel copiar o log." : "Relato, imagem e log salvos localmente.";
    return true;
}
std::optional<fs::path> SaveF3Report(const fs::path& capture_dir) {
    std::error_code ec;
    const auto screenshot=capture_dir/"final.bmp";
    if(!fs::is_regular_file(screenshot,ec) || ec) {
        LOGE("F3 report: missing captured frame at %s",screenshot.string().c_str());
        return std::nullopt;
    }
    SYSTEMTIME now;GetLocalTime(&now);
    char name[96];
    std::snprintf(name,sizeof(name),"%04u%02u%02u-%02u%02u%02u-%llu",now.wYear,now.wMonth,now.wDay,
        now.wHour,now.wMinute,now.wSecond,static_cast<unsigned long long>(GetTickCount64()));
    const auto directory=Root()/"reports"/name;
    fs::create_directories(directory,ec);
    if(ec) {
        LOGE("F3 report: cannot create %s: %s",directory.string().c_str(),ec.message().c_str());
        return std::nullopt;
    }
    fs::copy_file(screenshot,directory/"screenshot.bmp",fs::copy_options::overwrite_existing,ec);
    if(ec) {
        LOGE("F3 report: cannot copy screenshot: %s",ec.message().c_str());
        return std::nullopt;
    }
    std::ofstream out(directory/"report.txt",std::ios::binary);
    if(!out) {
        LOGE("F3 report: cannot write report.txt under %s",directory.string().c_str());
        return std::nullopt;
    }
    out << "Castlevania: Curse of Darkness | Xbox USA\nXBE SHA-256: "
           "4bc21b87ce7bb39ec02beebf78fd2f5b59c234187fb9a62ac0a5ce4f8b0f9b7a\n\n"
        << "Titulo: Captura automatica F3\n\nRelato:\n"
        << "Print e diagnostico grafico salvos ao pressionar F3.\n"
        << "Pasta da captura: " << capture_dir.lexically_relative(Root()).string() << "\n"
        << "Render: " << app::Get().render_percent << "% | FPS cap: " << app::Get().fps_cap
        << " | shader: " << shader.load() << " | speed: " << speed.load() << "x"
        << " | godmode: " << godmode.load() << "\n"
        << "Output: " << app::Get().width << "x" << app::Get().height
        << " | aniso: " << app::Get().aniso << " | vsync: " << app::Get().vsync << "\n";
    out.close();
    if(!out) {
        LOGE("F3 report: failed writing report.txt under %s",directory.string().c_str());
        return std::nullopt;
    }
    LOGR("F3 report saved: %s",directory.string().c_str());
    logx::Flush();
    fs::copy_file(logx::Path(),directory/"runtime.log",fs::copy_options::overwrite_existing,ec);
    if(ec)LOGE("F3 report: report saved but runtime log copy failed: %s",ec.message().c_str());
    return directory;
}
void Window(void* hwnd) {
    window = static_cast<HWND>(hwnd);
    RAWINPUTDEVICE device{1, 2, 0, window};
    if (!RegisterRawInputDevices(&device, 1, sizeof(device))) LOGE("input: mouse raw registration failed");
}
void ShutdownInput() { ClipCursor(nullptr); ClearInput(); binding_capture.store(-1);std::lock_guard guard(input_lock);native_menu_key.Clear(); }
void RequestTitleRestart() { if(window) PostMessageW(window,WM_APP+3,0,0); }
bool Message(void*, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result) {
    if (msg == WM_ACTIVATE) { focus.store(LOWORD(wp) != WA_INACTIVE); if (!focus) ShutdownInput(); }
    if (msg == WM_KILLFOCUS) { focus.store(false); ShutdownInput(); }
    if (msg == WM_KEYDOWN && !(lp & (intptr_t(1)<<30))) {
        if (wp == VK_F1) { menu_open.store(!menu_open.load()); ShutdownInput(); return true; }
        if (wp == VK_ESCAPE && !UiCapturing()) {
            uint32_t flags_pointer=0,flags=0;
            if(ReadableGuestRange(kernel::g_base,0x548C10,4)) std::memcpy(&flags_pointer,kernel::g_base+0x548C10,4);
            if(flags_pointer && ReadableGuestRange(kernel::g_base,flags_pointer,4)) std::memcpy(&flags,kernel::g_base+flags_pointer,4);
            std::lock_guard guard(input_lock);
            native_menu_key.Press(GetTickCount64(),(flags&(1u<<16))!=0);
            return true;
        }
        if (wp == VK_F8) { CaptureBug(); return true; }
        if (wp == VK_F9) { muted.store(!muted.load()); return true; }
    }
    int key = -1;
    bool down = false;
    if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYUP) { key = int(wp); down = msg == WM_KEYDOWN; }
    if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) { key = VK_LBUTTON; down = msg == WM_LBUTTONDOWN; }
    if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) { key = VK_RBUTTON; down = msg == WM_RBUTTONDOWN; }
    if (key > 0 && key < 256) {
        int capture = binding_capture.load();
        if (down && capture >= 0) { SetBinding(capture, key); binding_capture.store(-1); return true; }
        keys[size_t(key)].store(down);
    }
    if (msg == WM_INPUT && !UiCapturing() && (mouse_camera || camera_mode==4 || camera_mode==5)) {
        if (keyboard && GetForegroundWindow() == window) {
            RECT bounds{};
            if (GetClientRect(window, &bounds)) {
                POINT origin{bounds.left,bounds.top}, end{bounds.right,bounds.bottom};
                ClientToScreen(window,&origin); ClientToScreen(window,&end);
                RECT screen{origin.x,origin.y,end.x,end.y}; ClipCursor(&screen);
            }
        }
        RAWINPUT raw{}; UINT bytes = sizeof(raw);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, &raw, &bytes, sizeof(RAWINPUTHEADER)) != UINT(-1) &&
            raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
            mouse_x.fetch_add(raw.data.mouse.lLastX); mouse_y.fetch_add(raw.data.mouse.lLastY);
        }
    }
    if (result) *result = 0;
    return false;
}
void Pad(int port, input::Pad& pad) {
    if (port != 0) return;
    ProcessControllerShortcut(pad);
    if (UiCapturing()) {
        MenuGamePad(pad,CharacterChoosing() || !GameFocused() || (camera_mode==4 && camera_live));
        return;
    }
    if(camera_mode==4 && camera_live) {
        const bool use_keys=keyboard && GameFocused();
        FreeCameraInput(pad,use_keys && Pressed('W'),use_keys && Pressed('S'),use_keys && Pressed('A'),
            use_keys && Pressed('D'),use_keys && Pressed('E'),use_keys && Pressed('Q'),mouse_x.exchange(0),mouse_y.exchange(0));
        std::lock_guard guard(input_lock);
        if(native_menu_key.Active(GetTickCount64())) {
            if(native_menu_key.back) pad.analog[1]=255;else pad.buttons|=1u<<4;
        }
        return;
    }
    const bool orbital=camera_mode==5 && camera_live;
    if(orbital) OrbitCameraInput(pad,mouse_x.exchange(0),mouse_y.exchange(0));
    if (!keyboard || GetForegroundWindow() != window) return;
    std::lock_guard lock(input_lock);
    pad.connected = true;
    if(native_menu_key.Active(GetTickCount64())) {
        if(native_menu_key.back) pad.analog[1]=255;
        else pad.buttons|=1u<<4;
    }
    auto held=[&](size_t index){return Pressed(bindings[index].key);};
    int16_t x = dev_rules::MoveAxis(held(13),held(12));
    int16_t y = dev_rules::MoveAxis(held(10),held(11));
    if (x || y) { pad.lx = x; pad.ly = y; }
    for (const auto& binding : bindings) if (Pressed(binding.key)) {
        if (binding.button < 8) pad.analog[binding.button] = 255;
        else if(binding.button<10) pad.buttons |= uint16_t(1u << (binding.button == 8 ? 4 : 5));
    }
    static auto last = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    static int16_t rx = 0, ry = 0;
    if (now-last >= std::chrono::milliseconds(8)) {
        rx = dev_rules::MouseAxis(mouse_x.exchange(0), sensitivity.load(), invert_mouse_x.load());
        ry = dev_rules::MouseAxis(mouse_y.exchange(0), sensitivity.load(), invert_mouse_y.load());
        last = now;
    }
    if (!orbital && mouse_camera && (rx || ry)) { pad.rx = rx; pad.ry = ry; }
    const auto camera_x=dev_rules::MoveAxis(held(17),held(16));
    const auto camera_y=dev_rules::MoveAxis(held(14),held(15));
    if(!orbital && (camera_x || camera_y)){pad.rx=camera_x;pad.ry=camera_y;}
}
void Initialize() {
    ReloadMods(); ReloadCheats();
    SetSpeed(speed.load());
    menu_open.store(std::getenv("COD_DEV_MENU") != nullptr);
    texture_dump.store(std::getenv("COD_DEV_DUMP") != nullptr);
}
bool ConsumeTestRequest() { return std::getenv("COD_DEV_SELFTEST") != nullptr; }
void RunIntegrationChecks() {
    if (!ConsumeTestRequest() || !kernel::g_base) return;
    static bool ran=false;
    if(ran) return;
    ran=true;
    toml::table saved;Save(&saved);
    toml::table altered=saved;
    altered.insert_or_assign("master_volume",.25f);
    altered.insert_or_assign("post_shader",1);
    altered.insert_or_assign("menu_x",123.0f);
    altered.insert_or_assign("menu_opacity",.65f);
    altered.insert_or_assign("invert_mouse_x",false);
    altered.insert_or_assign("invert_mouse_y",true);
    altered.insert_or_assign("bind_10",int('I'));
    altered.insert_or_assign("minimap_enabled",false);
    altered.insert_or_assign("minimap_scale",1.25f);
    altered.insert_or_assign("minimap_opacity",.6f);
    altered.insert_or_assign("minimap_rotating",false);
    altered.insert_or_assign("game_menu_scale",.65f);
    altered.insert_or_assign("ptbr_enabled",true);
    altered.insert_or_assign("orbit_smoothing",12.0f);
    altered.insert_or_assign("hud_enabled",false);
    altered.insert_or_assign("hud_scale",.75f);
    altered.insert_or_assign("hud_quick_hide",true);
    altered.insert_or_assign("hud_hide_delay",.5f);
    altered.insert_or_assign("camera_mode",3);altered.insert_or_assign("camera_distance",1.2f);
    altered.insert_or_assign("camera_height",60.0f);altered.insert_or_assign("camera_field",1.1f);
    altered.insert_or_assign("camera_fly_speed",800.0f);
    Load(&altered);
    bool settings_ok=volume.load()==.25f && shader.load()==1 && menu_preferences.x==123 && menu_preferences.opacity==.65f &&
        !invert_mouse_x.load() && invert_mouse_y.load() && Bindings()[10].key=='I' &&
        !minimap_enabled.load() && minimap_scale.load()==1.25f && minimap_opacity.load()==.6f &&
        !hud_enabled.load() && hud_scale.load()==.75f && hud_quick_hide.load() && hud_hide_delay.load()==.5f &&
        camera_mode.load()==3 && camera_distance.load()==1.2f && camera_height.load()==60 && camera_field.load()==1.1f && camera_fly_speed.load()==800 &&
        !minimap_rotating.load() && game_menu_scale.load()==1.0f && ptbr_enabled.load() && orbit_smoothing.load()==12;
    toml::table roundtrip;Save(&roundtrip);
    settings_ok=settings_ok && roundtrip["menu_x"].value_or(0.0f)==123 && roundtrip["menu_opacity"].value_or(0.0f)==.65f;
    settings_ok=settings_ok && !roundtrip["minimap_enabled"].value_or(true) &&
        roundtrip["minimap_scale"].value_or(0.0f)==1.25f && roundtrip["minimap_opacity"].value_or(0.0f)==.6f;
    settings_ok=settings_ok && !roundtrip["hud_enabled"].value_or(true) &&
        roundtrip["hud_scale"].value_or(0.0f)==.75f && roundtrip["hud_quick_hide"].value_or(false) &&
        roundtrip["hud_hide_delay"].value_or(0.0f)==.5f;
    Load(&saved);
    LOGR("dev selftest: settings roundtrip=%d",settings_ok);
    VerifyProfileStorage();

    auto fixture=Root()/".selftest"/"overlay";
    std::error_code ec;fs::create_directories(fixture/"files",ec);
    {std::ofstream file(fixture/"files"/"dev-proof.txt");file<<"COD overlay proof";}
    std::vector<Mod> prior;
    {std::lock_guard lock(mods_lock);prior=mods;mods.push_back({"Integration test",fixture,true});}
    std::vector<uint8_t> bytes;
    bool overlay_ok=kernel::ReadGameFile("D:\\dev-proof.txt",bytes) &&
                    std::string(bytes.begin(),bytes.end())=="COD overlay proof";
    bool traversal_blocked=!ModFile("../outside");
    {std::lock_guard lock(mods_lock);mods=std::move(prior);}
    LOGR("dev selftest: real game file overlay=%d path guard=%d",overlay_ok,traversal_blocked);
    LOGR("dev selftest: file overlay write/delete guard=%d",kernel::VerifyReadOnlyOverlay((fixture/"files"/"dev-proof.txt").string()));

    uint32_t address=0,size=4096;
    bool cheat_ok=false,guard_ok=false;
    if(mem::AllocateVirtual(address,size,mem::kMemReserve|mem::kMemCommit,PAGE_READWRITE)==0) {
        kernel::W32(address,123);
        Cheat test;test.name="Isolated test allocation";test.address=address;test.expected=456;test.value=999;
        guard_ok=!ToggleCheat(test,true) && kernel::R32(address)==123;
        test.expected=123;
        cheat_ok=ToggleCheat(test,true) && kernel::R32(address)==999;
        ToggleCheat(test,false);
        cheat_ok=cheat_ok && kernel::R32(address)==123;
        mem::FreeVirtual(address,size,mem::kMemRelease);
    }
    LOGR("dev selftest: allocated-memory cheat apply/revert=%d expected-value guard=%d",cheat_ok,guard_ok);
    LOGR("dev selftest: godmode original damage/heal/other-actor/off=%d",VerifyGodmodeOriginal());
    VerifyHudHooks();
    VerifyIndicatorHooks();
    VerifyCameraHooks();
    VerifyLanguageHooks();
    VerifyMenuBackground();
    LOGR("dev selftest: native character selection passed=%d",VerifyCharacterSelection());
    VerifySpeedRuntime();
    CaptureBug();
}
}
