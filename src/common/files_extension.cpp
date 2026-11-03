#include "dev_runtime.h"
#include "dev_rules.h"
#include "files_runtime.inc"

namespace kernel {
bool VerifyReadOnlyOverlay(const std::string& path) {
    uint32_t memory = 0, size = 4096;
    constexpr uint32_t kGuestReadWrite = 4;
    if (mem::AllocateVirtual(memory, size, mem::kMemReserve | mem::kMemCommit, kGuestReadWrite) != 0) return false;
    auto file = std::make_shared<File>();
    file->host = path;
    file->fp = std::fopen(path.c_str(), "rb");
    file->overlay = true;
    if (!file->fp) { mem::FreeVirtual(memory, size, mem::kMemRelease); return false; }
    const uint32_t handle = NewHandle(file);
    X86Context context{};
    context.esp = memory;
    W32(memory + 4, handle);
    W32(memory + 20, memory + 512);
    W32(memory + 24, memory + 256);
    W32(memory + 28, 1);
    Args args{context};
    bool ok = Transfer(args, true, 0) == kAccessDenied;
    W32(memory + 8, memory + 512);
    W32(memory + 12, memory + 256);
    for (const uint32_t information : {10u, 13u, 19u, 20u}) {
        W32(memory + 20, information);
        ok = (kimpl_NtSetInformationFile(context, args) == kAccessDenied) && ok;
    }
    ok = ok && !file->delete_on_close;
    CloseHandle(handle);
    mem::FreeVirtual(memory, size, mem::kMemRelease);
    return ok;
}
}
