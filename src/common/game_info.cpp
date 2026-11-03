#include "app/app.h"
#include "dev_runtime.h"
#include "core/log.h"
#include "gpu/backend.h"
#include <chrono>
#include <cstdlib>
#include <thread>

int ValidateTextureColors();
static void StartDiagnostics() {
    dev::Initialize();
    if (!std::getenv("XBE_PERF")) return;
    const int result = ValidateTextureColors();
    if (result) {
        LOGE("graphics: color conversion self-test failed: %d", result);
        std::abort();
    }
    LOGR("graphics: color conversion self-test passed (RGBA, BGRA, ARGB, P8 alpha)");
}

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Castlevania: Curse of Darkness",               // window and splash
    .short_title = L"CastlevaniaCOD",               // message boxes
    .name = L"CastlevaniaCOD",                      // window classes, crash dumps, handover event
    .log = "castlevania_cod",                        // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "CastlevaniaCOD"}},  // each recompiled xbe and its exe, for title launches
    .options = {},                        // game settings kept in the settings file
    .load = dev::Load,                      // reads more from the settings file's toml::table
    .save = dev::Save,                      // writes more into it
    .init = StartDiagnostics,                      // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = dev::Window,                    // the game window is up
    .message = dev::Message,                   // window messages first; true takes one
    .ui = dev::Draw,                        // ImGui drawing each frame
    .pad = dev::Pad,                       // changes a polled pad
    .rumble = nullptr,                    // changes a rumble before it is set
};
