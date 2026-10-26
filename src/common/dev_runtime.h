#pragma once
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <memory>
#include <string>
#include <vector>
#include "core/input.h"
#include "controller_map.h"
#include "texture_pack.h"
#include "menu_preferences.h"

namespace dev {
inline std::atomic<bool> menu_open{false}, muted{false}, keyboard{true}, mouse_camera{true};
inline std::atomic<bool> controller_shortcut{true};
inline std::atomic<bool> minimap_enabled{true};
inline std::atomic<bool> minimap_rotating{true};
inline std::atomic<float> camera_heading{0};
inline std::atomic<float> minimap_scale{1.0f},minimap_opacity{.8f};
inline std::atomic<bool> hud_enabled{true},hud_quick_hide{false},hud_always_visible{false};
inline std::atomic<float> hud_scale{.5f},hud_hide_delay{1.0f};
inline std::atomic<float> game_menu_scale{1.0f};
inline std::atomic<bool> game_menu_live{false};
inline std::atomic<bool> ptbr_enabled{false},language_dirty{false};
bool NativeMenuActive(uint8_t* base);
void SelectTextLanguage(int language);
bool TranslationPackAvailable();
bool VerifyLanguageHooks();
bool VerifyMenuBackground();
inline std::atomic<int> camera_mode{0};
inline std::atomic<bool> camera_live{false};
inline std::atomic<float> camera_distance{1.0f},camera_height{0.0f},camera_field{1.0f},camera_fly_speed{600};
// graus somados aos angulos nativos da view (Personalizada e Livre)
inline std::atomic<float> camera_yaw{0},camera_pitch{0},camera_roll{0};
// contador de FPS por cima do jogo; canto 0..3 = sup. esq., sup. dir., inf. esq., inf. dir.
inline std::atomic<bool> show_fps{false};
inline std::atomic<int> fps_corner{1};
// Diagnostico > Registrar desempenho: as linhas "perf:" do XBE_PERF, so nesta sessao (nao salvo)
inline std::atomic<bool> perf_log{false};
// Pesquisa: registra no log os inteiros do inimigo alvo que caem entre quadros (so nesta sessao)
inline std::atomic<bool> enemy_probe{false};
inline std::atomic<float> orbit_smoothing{8.0f};
void OrbitCameraInput(input::Pad& pad,int mouse_x,int mouse_y);
void FreeCameraInput(input::Pad& pad,bool forward,bool back,bool left,bool right,bool up,bool down,int mouse_x,int mouse_y);
bool VerifyHudHooks();
bool VerifyIndicatorHooks();
bool VerifyCameraHooks();
inline std::atomic<bool> invert_mouse_x{true}, invert_mouse_y{false};
inline std::atomic<bool> godmode{false}, godmode_ready{false};
inline std::atomic<unsigned> prevented_damage{0};
enum class NewGamePhase { Idle, Choosing, Starting };
inline std::atomic<NewGamePhase> new_game_phase{NewGamePhase::Idle};
inline std::atomic<int> new_game_character{0};
inline std::atomic<bool> character_selection_ready{false};
inline std::atomic<bool> character_preview_active{false};
inline bool CharacterChoosing() { return new_game_phase.load() == NewGamePhase::Choosing; }
inline bool UiCapturing() { return menu_open.load() || CharacterChoosing(); }
// padrao: o jogo segue rodando com o menu aberto (analogicos continuam no jogo)
inline std::atomic<bool> pause_with_menu{false};
// barra do menu sendo ajustada: o analogico esquerdo arrasta a barra e nao anda
inline std::atomic<bool> menu_adjusting{false};
// Dificuldade: multiplicadores do dano em vida (stat 6) do jogador (slot 0) e dos Innocent Devils (outros slots)
inline std::atomic<float> damage_taken{1.0f},devil_damage_taken{1.0f};
// Economia: multiplicador do ganho de ouro (stat 111) e EXP (stat 3); compras sem gastar ouro
inline std::atomic<float> gold_gain{1.0f},exp_gain{1.0f};
inline std::atomic<bool> free_shopping{false};
// MP infinito: gasto de MP do Hector (atributo 32) zerado no setter
inline std::atomic<bool> infinite_mp{false};
// Simula o save de Lament of Innocence (PS2) para liberar o bonus Moai em jogo novo ou ao carregar
inline std::atomic<bool> lament_save{false};
// Apresentacao: brilho (soma), contraste e saturacao (1 = original)
inline std::atomic<float> brightness{0.0f},contrast{1.0f},saturation{1.0f};
inline std::atomic<bool> mute_unfocused{false};
// Menu dev aberto pausa jogo, relogio, vblank e audio (fora da escolha de personagem, que precisa do jogo).
inline bool GamePaused() { return menu_open.load() && pause_with_menu.load() && !CharacterChoosing(); }
void PauseClock(bool paused);
bool ValidateCharacterSelection();
bool VerifyCharacterSelection();
inline std::atomic<float> volume{.8f}, balance{0}, sensitivity{.012f}, speed{1};
inline std::atomic<int> shader{0};
inline std::atomic<bool> texture_dump{false};
inline std::atomic<bool> texture_packs{false},translation_textures{false};
bool HdTexturesActive();
inline std::atomic<unsigned> texture_generation{0}, texture_replacements{0}, texture_exports{0};
struct Binding { const char* action; int key; int button; };
struct Mod { std::string name; std::filesystem::path root; bool enabled; };
struct Cheat {
    std::string name;
    uint32_t address = 0, expected = 0, value = 0, original = 0;
    int width = 4;
    bool enabled = false, applied = false;
    std::string status;
};
std::vector<Binding> Bindings();
void BeginBinding(int index);
void SetBinding(int index, int key);
void ResetBindings();
// Mapeamento do controle (jogador 1): botao do jogo -> botao fisico. Menu dev e atalhos usam o fisico.
ControllerMap ControllerMapping();
void SetControllerMapping(const ControllerMap& map);
void BeginControllerCapture(int target);
void CancelControllerCapture();
int ControllerCaptureTarget();
bool ControllerConnected();
void Initialize();
void ShutdownInput();
void RequestTitleRestart();
void Load(const void* table);
void Save(void* table);
std::array<std::string,3> UserProfileNames();
int SelectedProfile();
void SetSelectedProfile(int selection);
bool SaveUserProfile(int slot,const std::string& name);
bool ApplyProfile(int selection);
bool VerifyProfileStorage();
void Draw(double fps);
void DrawMinimap();
void ConfigureMenuFonts();
void UpdateUiInput();
bool GameFocused();
void ProcessControllerShortcut(input::Pad& pad);
void SetSpeed(float factor);
uint64_t VirtualNanoseconds();
int64_t RealWaitNanoseconds(int64_t virtual_ns);
bool ValidateGodmodeImage();
bool VerifyGodmodeOriginal();
// Cheats de botao: vida cheia (sem reviver) e ouro no limite do jogo; falso se nada mudou.
bool RefillPlayerHp();
bool RefillPlayerMp();
bool MaxPlayerGold();
bool VerifySpeedRuntime();
void Pad(int port, input::Pad& pad);
bool Message(void* hwnd, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result);
void Window(void* hwnd);
void ApplyAudio(float* frames, uint32_t count);
void CaptureBug();
std::optional<std::filesystem::path> TakeScreenshotRequest();
void ScreenshotDone(bool ok);
std::filesystem::path CurrentReport();
std::string ReportStatus();
bool SaveReport(const std::string& title, const std::string& text);
std::optional<std::filesystem::path> SaveF3Report(const std::filesystem::path& capture_dir);
// Previa da captura no menu: o menu pede o arquivo, o backend sobe a textura e devolve o id do ImGui.
struct ReportPreview { std::filesystem::path path; uint64_t texture = 0; uint32_t width = 0, height = 0; };
void RequestReportPreview(const std::filesystem::path& bmp);
std::optional<std::filesystem::path> TakeReportPreviewRequest();
void SetReportPreview(const std::filesystem::path& bmp, uint64_t texture, uint32_t width, uint32_t height);
ReportPreview CurrentReportPreview();
void NotifyToast(const std::string& message);
std::string ToastMessage();
std::vector<Mod>& Mods();
std::vector<Cheat>& Cheats();
void ReloadMods();
void EnableMod(size_t index, bool enabled);
void ReloadCheats();
void TickCheats();
bool ToggleCheat(Cheat& cheat, bool enabled);
std::optional<std::filesystem::path> ModFile(const std::filesystem::path& relative);
std::optional<std::filesystem::path> ModTexture(uint64_t hash);
std::shared_ptr<const TextureImage> ReplacementTexture(uint64_t hash);
void DumpTexture(uint64_t hash, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels);
std::filesystem::path Root();
bool ConsumeTestRequest();
void RunIntegrationChecks();
}
