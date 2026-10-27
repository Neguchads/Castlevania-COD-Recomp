#include "../src/common/capture_path.h"
#include <cstdio>
int main() {
    namespace fs=std::filesystem;
    if(cod_capture::Directory({})!=fs::path("captures/orange-marker"))return 1;
    if(cod_capture::Directory("captures/custom")!=fs::path("captures/custom"))return 2;
    const fs::path absolute=fs::current_path()/"capture-test";
    if(cod_capture::Directory(absolute)!=absolute)return 3;
    // O destino continua utilizavel com a configuracao vazia lida antes do F3.
    const fs::path cached_empty;
    if(cod_capture::Directory(cached_empty).empty())return 4;
    std::puts("Capture path: empty boot config, relative and absolute overrides passed");
}
