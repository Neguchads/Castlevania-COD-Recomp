#include "dev_runtime.h"
#include "dev_rules.h"

namespace dev {
void ApplyAudio(float* frames, uint32_t count) {
    const bool silent = muted.load() || (mute_unfocused.load() && !GameFocused());
    dev_rules::MixAudio({frames, size_t(count) * 2}, volume.load(), balance.load(), silent);
}
}
