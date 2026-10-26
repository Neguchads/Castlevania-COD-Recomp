#include "dev_runtime.h"
#include "controller_map.h"
#include "dev_rules.h"
#include "menu_input.h"
#include "core/log.h"
#include <imgui.h>
#include <chrono>
#include <mutex>

namespace input { Pad RawPoll(int port); }

namespace dev {
namespace {
std::mutex map_lock;
ControllerMap controller_map = IdentityControllerMap();
ButtonCapture button_capture;
std::atomic<bool> controller_connected{false};
double Now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
}
ControllerMap ControllerMapping() { std::lock_guard guard(map_lock); return controller_map; }
void SetControllerMapping(const ControllerMap& map) { std::lock_guard guard(map_lock); controller_map = map; }
void BeginControllerCapture(int target) { std::lock_guard guard(map_lock); button_capture.Begin(target, Now()); }
void CancelControllerCapture() { std::lock_guard guard(map_lock); button_capture.Cancel(); }
int ControllerCaptureTarget() { std::lock_guard guard(map_lock); return button_capture.target; }
bool ControllerConnected() { return controller_connected.load(); }
void ProcessControllerShortcut(input::Pad& pad) {
    static std::mutex lock;
    static dev_rules::MenuChord chord;
    static dev_rules::TitleResetChord reset;
    bool toggle = false;
    auto restart = dev_rules::TitleResetChord::Idle;
    {
        std::lock_guard guard(lock);
        const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
        restart = reset.Update(GameFocused() ? pad.buttons : 0, pad.analog[6], pad.analog[7], now);
        // Com os dois gatilhos, Start/Back nao chegam ao jogo (nao abrem pausa/mapa).
        if (dev_rules::TitleResetChord::Held(pad.buttons, pad.analog[6], pad.analog[7]))
            pad.buttons &= uint16_t(~dev_rules::TitleResetChord::mask);
        if (!controller_shortcut || !GameFocused()) { chord.Update(0); }
        else {
            toggle = chord.Update(pad.buttons);
            if (chord.latched) pad.buttons &= uint16_t(~dev_rules::MenuChord::mask);
        }
    }
    if (restart == dev_rules::TitleResetChord::Started)
        NotifyToast("Segure Start + Back + LT + RT por 2 s para voltar ao título. O progresso não salvo será perdido.");
    if (restart == dev_rules::TitleResetChord::Fire) {
        LOGR("dev: title restart by Start + Back + LT + RT");
        RequestTitleRestart();
    }
    if (toggle) {
        menu_open.store(!menu_open.load());
        ShutdownInput();
        LOGR("dev: menu toggled by L3 + R3");
    }
}

void UpdateUiInput() {
    input::Pad pad = input::RawPoll(0);
    controller_connected.store(pad.connected);
    ProcessControllerShortcut(pad);
    {
        // Mapeamento esperando um botao: o controle so responde a captura, nao navega o menu.
        std::lock_guard guard(map_lock);
        if (!menu_open.load()) button_capture.Cancel();  // nada de remapear jogando
        if (button_capture.target >= 0) {
            const auto result = button_capture.Update(GameFocused() ? PadHeldMask(pad) : 0, Now());
            if (result.source >= 0) {
                AssignButton(controller_map, result.target, result.source);
                LOGR("dev: controller button %d now reads physical %d", result.target, result.source);
            }
            pad = {};
        }
    }
    if (!UiCapturing() || !GameFocused()) pad = {};
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    // A janela do painel, e nao o monitor, determina o tamanho do conteudo.
    io.FontGlobalScale = menu_open ? MenuContentScale(menu_preferences.width,menu_preferences.height) :
        std::clamp(std::min(io.DisplaySize.x / 1280.0f,io.DisplaySize.y / 800.0f),.75f,1.5f);
    static dev_rules::ReleaseGate back;
    menu_adjusting.store(UiCapturing() && ImGui::IsAnyItemActive());
    QueueMenuPad(pad,UiCapturing() && GameFocused(),back,ImGui::IsAnyItemActive());
}
}
