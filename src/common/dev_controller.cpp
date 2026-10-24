#include "dev_runtime.h"
#include "dev_rules.h"
#include "menu_input.h"
#include "core/log.h"
#include <imgui.h>
#include <mutex>

namespace input { Pad RawPoll(int port); }

namespace dev {
void ProcessControllerShortcut(input::Pad& pad) {
    static std::mutex lock;
    static dev_rules::MenuChord chord;
    bool toggle = false;
    {
        std::lock_guard guard(lock);
        if (!controller_shortcut || !GameFocused()) { chord.Update(0); return; }
        toggle = chord.Update(pad.buttons);
        if (chord.latched) pad.buttons &= uint16_t(~dev_rules::MenuChord::mask);
    }
    if (toggle) {
        menu_open.store(!menu_open.load());
        ShutdownInput();
        LOGR("dev: menu toggled by L3 + R3");
    }
}

void UpdateUiInput() {
    input::Pad pad = input::RawPoll(0);
    ProcessControllerShortcut(pad);
    if (!UiCapturing() || !GameFocused()) pad = {};
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    // A janela do painel, e nao o monitor, determina o tamanho do conteudo.
    io.FontGlobalScale = menu_open ? MenuContentScale(menu_preferences.width,menu_preferences.height) :
        std::clamp(std::min(io.DisplaySize.x / 1280.0f,io.DisplaySize.y / 800.0f),.75f,1.5f);
    static dev_rules::ReleaseGate back;
    QueueMenuPad(pad,UiCapturing() && GameFocused(),back);
}
}
