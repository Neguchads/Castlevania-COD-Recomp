#include "dev_runtime.h"
#include "core/log.h"
#include "core/diag.h"
#undef LOGI
#define LOGI(...) do { if (diag::Perf()) LOGR(__VA_ARGS__); } while (0)
#include "d3d8_runtime.inc"
#include "indexed_up.h"

HLE_STDCALL(D3DDevice_DrawIndexedVerticesUP, 5) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    if (!g_backend || !a[1]) return 0;
    if (!a[2] || !a[3] || a[1] > 1048576) {
        LOGE("d3d: invalid indexed UP input");
        return 0x8876086Cu;
    }
    const auto* indices = static_cast<const uint8_t*>(HostPtr(a[2]));
    auto bytes = cod_graphics::IndexedVertexBytes({indices, size_t(a[1]) * 2}, a[4], kUpHalf);
    if (!bytes) {
        LOGE("d3d: indexed UP exceeds vertex upload capacity");
        return 0x8876086Cu;
    }
    Walk(Dev(kDevPush));
    SyncDeferred();
    const auto stream = g_state.streams[0];
    const auto base_vertex = g_state.base_vertex;
    g_state.streams[0] = {UpVertices(HostPtr(a[3]), *bytes), a[4]};
    g_state.base_vertex = 0;
    g_backend->Draw(g_state, a[0], 0, a[1], indices);
    g_state.base_vertex = base_vertex;
    g_state.streams[0] = stream;
    return 0;
}
