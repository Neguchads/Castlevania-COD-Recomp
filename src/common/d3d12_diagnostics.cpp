#include "dev_runtime.h"
// Diagnóstico local: preserva o SDK compartilhado e registra somente quando solicitado.
#include "core/log.h"
#include "core/diag.h"
#undef LOGI
#define LOGI(...) do { if (diag::Perf()) LOGR(__VA_ARGS__); } while (0)
#include "d3d12_runtime.inc"
