# Game Title (original Xbox), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(castlevania_cod LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the SDK: sdk/ here, two folders up inside the SDK's templates, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/TGP482/XboxXBERecomp.git GIT_TAG e0de3e44d10443c1c9aad512f36baf133cdd4a60)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game/")

# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game.toml)
    file(STRINGS config/game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat, xbe = GAME_XBE
add_xbe_image(CastlevaniaCOD default.xbe game.toml ${GAME_PARTITIONS} game)

# SDL e a unica fonte de controle na UI; XInput duplicado disputa foco com mouse/teclado.
target_compile_definitions(CastlevaniaCOD PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)



# O módulo local inclui o áudio original e acrescenta SetPitch.
get_target_property(COD_HOST_SOURCES CastlevaniaCOD SOURCES)
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/xdk/dsound[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/audio/xaudio2[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/gpu/d3d12[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/xdk/d3d8[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/kernel/files[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/app/main[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/core/input[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/cpu/dispatch[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/kernel/objects[.]cpp$")
list(FILTER COD_HOST_SOURCES EXCLUDE REGEX "/runtime/kernel/xboxkrnl[.]cpp$")
set_property(TARGET CastlevaniaCOD PROPERTY SOURCES ${COD_HOST_SOURCES})
set_source_files_properties(src/common/input_extension.cpp PROPERTIES
    INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:SDL3::SDL3-static,INTERFACE_INCLUDE_DIRECTORIES>")

# Icone do exe e da janela: o SDK carrega o recurso ICON 1. A arte e do usuario e fica em assets/icon.ico
# (fora do git); sem o arquivo, o exe sai com o icone padrao do Windows.
set(COD_ICON ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${COD_ICON})
# llvm-rc direto: a linguagem RC do CMake com clang (frontend GNU) pega o rc.exe do Windows SDK e passa -MD/-MF,
# que ele recusa. O .res entra no link como arquivo; o lld-link aceita.
get_filename_component(COD_LLVM_BIN ${CMAKE_CXX_COMPILER} DIRECTORY)
find_program(COD_LLVM_RC NAMES llvm-rc HINTS ${COD_LLVM_BIN})
if(EXISTS ${COD_ICON} AND COD_LLVM_RC)
    set(COD_ICON_RC ${CMAKE_CURRENT_BINARY_DIR}/cod_icon.rc)
    set(COD_ICON_RES ${CMAKE_CURRENT_BINARY_DIR}/cod_icon.res)
    file(CONFIGURE OUTPUT ${COD_ICON_RC} CONTENT "1 ICON \"${COD_ICON}\"\n")
    add_custom_command(OUTPUT ${COD_ICON_RES}
        COMMAND ${COD_LLVM_RC} /no-preprocess /fo ${COD_ICON_RES} ${COD_ICON_RC}
        DEPENDS ${COD_ICON_RC} ${COD_ICON} VERBATIM)
    add_custom_target(cod_icon DEPENDS ${COD_ICON_RES})
    add_dependencies(CastlevaniaCOD cod_icon)
    target_link_options(CastlevaniaCOD PRIVATE ${COD_ICON_RES})
    set_property(TARGET CastlevaniaCOD APPEND PROPERTY LINK_DEPENDS ${COD_ICON_RES})
endif()


# A DLL precisa ser relinkada se o EXE mudar; evita recompilação desnecessária no primeiro início.
set_property(TARGET CastlevaniaCOD_code APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:CastlevaniaCOD>)

add_custom_target(cod_modding_docs ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/MODDING.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/cheats.example.toml"
        "$<TARGET_FILE_DIR:CastlevaniaCOD>"
    DEPENDS CastlevaniaCOD)
