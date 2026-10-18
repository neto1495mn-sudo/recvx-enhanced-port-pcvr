# PC port build.
#
#   recvx_game     : the original game + platform code from the decomp, compiled
#                    unmodified except for '#ifdef TARGET_PC' asm replacements.
#   recvx_platform : the new PC abstraction layer (PS2 hardware/IOP emulation at
#                    library boundaries, renderer, audio, input, overlay).
#   recvx          : the executable.

set(DECOMP ${RECVX_DECOMP_DIR})
add_compile_definitions(_FILE_OFFSET_BITS=64)   # disc images over 2 GB (cdvd.c fseeko); mingw-w64 honours it too
if(WIN32)
    # the crash log walks the EBP frame chain (host/log.c); measured on Linux: +1..5% game logic
    add_compile_options(-fno-omit-frame-pointer)
endif()
set(PROG ${DECOMP}/src/ps2/veronica/prog)
set(KATANA_INC ${DECOMP}/include/recvx-decomp-katana/KATANA/Include)

# SDL3 is built from source and linked statically by default, so the binary does not
# depend on a (32-bit) SDL3 package being installed. SDL still dlopen()s the system's
# X11/Wayland/audio libraries at runtime.
option(RECVX_VENDORED_SDL "Build SDL3 from source and link it statically" ON)
if(RECVX_VENDORED_SDL)
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.4.16 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(SDL3)
else()
    find_package(SDL3 REQUIRED CONFIG)
endif()

# OpenXR loader (Apache-2.0), linked statically: VR output (src/host/xr.c, --vr). Windows only for now.
option(RECVX_VR "Build the OpenXR VR output (Windows)" ON)
if(WIN32 AND RECVX_VR)
    include(FetchContent)
    set(DYNAMIC_LOADER OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(openxr GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git GIT_TAG release-1.1.63 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(openxr)
    # VR (vr117): an implicit API layer that fails to load or negotiate is skipped (the loader logs it) and VR starts
    # without it - as the loader does when another layer loads after it. With only that layer (OBS's while OBS runs) the
    # loader returned its error (-32, file access) and failed the whole instance: the game fell back to the desktop.
    set(_recvx_xr_api "${openxr_SOURCE_DIR}/src/loader/api_layer_interface.cpp")
    file(READ "${_recvx_xr_api}" _recvx_xr_src)
    string(FIND "${_recvx_xr_src}" "recvx: skipped layers" _recvx_xr_done)
    if(_recvx_xr_done EQUAL -1)
        string(REPLACE "    // If we failed catastrophically for some reason, clean up everything.\n"
                       "    // recvx: skipped layers (failed to load / negotiate, logged) do not fail the instance\n    if (found_all_layers) {\n        last_error = XR_SUCCESS;\n    }\n\n    // If we failed catastrophically for some reason, clean up everything.\n"
                       _recvx_xr_new "${_recvx_xr_src}")
        if(_recvx_xr_new STREQUAL _recvx_xr_src)
            message(FATAL_ERROR "OpenXR loader: the skipped-layer change no longer applies (loader source changed)")
        endif()
        file(WRITE "${_recvx_xr_api}" "${_recvx_xr_new}")
    endif()
    # mingw-w64's winapifamily.h lacks WINAPI_PARTITION_SYSTEM, which the loader tests under -Werror=undef
    target_compile_options(openxr_loader PRIVATE -Wno-undef -DWINAPI_PARTITION_SYSTEM=0)
endif()

# Dear ImGui (MIT) for the F1 overlay, with its SDL3 + OpenGL3 backends.
include(FetchContent)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.92.9b GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# Co-op host port mapping (src/net/portmap.c): miniupnpc (UPnP IGD) and libnatpmp (NAT-PMP),
# both BSD, static, Winsock on Windows.
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
set(UPNPC_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(miniupnpc GIT_REPOSITORY https://github.com/miniupnp/miniupnp.git GIT_TAG miniupnpc_2_3_3
                     GIT_SHALLOW TRUE SOURCE_SUBDIR miniupnpc EXCLUDE_FROM_ALL)
FetchContent_Declare(libnatpmp GIT_REPOSITORY https://github.com/miniupnp/libnatpmp.git
                     GIT_TAG 134fc89e2781e154e40042641f4d8bcbe42579f1 EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(miniupnpc libnatpmp)

# Co-op server (pc/server: recvx-server) and Monocypher, which the game shares for the
# end-to-end encryption of server sessions (src/net/rdv_proto.c)
add_subdirectory(server)

# ---------------------------------------------------------------- headers
# The Katana SDK headers are included with inconsistent case (SG_MWPLY.H vs
# sg_mwply.h). Mirror them under both spellings for case-sensitive filesystems.
set(CASEFOLD_DIR ${CMAKE_BINARY_DIR}/katana_casefold)
file(GLOB KATANA_HEADERS RELATIVE ${KATANA_INC} ${KATANA_INC}/*)
foreach(h ${KATANA_HEADERS})
    string(TOLOWER ${h} lo)
    string(TOUPPER ${h} up)
    foreach(n ${lo} ${up})
        if(NOT n STREQUAL h AND NOT EXISTS ${KATANA_INC}/${n} AND NOT EXISTS ${CASEFOLD_DIR}/${n})
            file(MAKE_DIRECTORY ${CASEFOLD_DIR})
            file(CREATE_LINK ${KATANA_INC}/${h} ${CASEFOLD_DIR}/${n} COPY_ON_ERROR SYMBOLIC)
        endif()
    endforeach()
endforeach()

# ---------------------------------------------------------------- game (decomp)
file(GLOB GAME_SOURCES ${PROG}/*.c)
set(GAME_EXCLUDE
    gcc_wrapper.c        # ee-gcc soft-float/int64 runtime helpers
    ps2_MovieFunc.c      # libmpeg/IPU movie player -> pc/src/movie
    ps2_MovieWork.c
    ps2_Vu1Scissor2.c    # #included by ps2_Vu1Strip.c
)
foreach(x ${GAME_EXCLUDE})
    list(REMOVE_ITEM GAME_SOURCES ${PROG}/${x})
endforeach()

# PC implementations of decomp-declared APIs that need the decomp's types
file(GLOB GLUE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/glue/*.c)

add_library(recvx_game OBJECT ${GAME_SOURCES} ${GLUE_SOURCES})
target_compile_definitions(recvx_game PRIVATE main=recvx_game_main)
target_include_directories(recvx_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ps2
    ${DECOMP}/src/cri/mwlib/ee/lib/libadxe
    ${PROG}
)
# Katana/CRI dirs ship libc-named headers (math.h, stddef.h, ...); search them
# after the host's system headers.
target_compile_options(recvx_game PRIVATE
    "SHELL:-idirafter ${KATANA_INC}"
    "SHELL:-idirafter ${CASEFOLD_DIR}"
    "SHELL:-idirafter ${DECOMP}/include/recvx-decomp-cri/cri/mwlib/include"
    "SHELL:-idirafter ${DECOMP}/include/recvx-decomp-cri/cri/mwlib/ee/include"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/pc_prelude.h"
    -std=gnu11
    -fpermissive              # decomp relies on implicit declarations/int conversions
    -fno-strict-aliasing
    -fwrapv
    -fsigned-char             # EE char is signed
    -ffp-contract=off         # no FMA fusion: keep the PS2's operation order
    -msse2 -mfpmath=sse       # IEEE single precision, no x87 extended intermediates
    -fno-tree-vectorize
    -ftrivial-auto-var-init=zero  # deterministic stand-in for MWCC's stale registers (docs/dev/uninitialized-locals.txt)
)
# the decomp is compiled as is, without warnings except calls to undeclared functions (an implicit
# int return breaks float and 64-bit results); missing returns and uninitialized locals are listed by
# pc/tools/decomp_warnings.py. The port's own glue gets warnings.
set_source_files_properties(${GAME_SOURCES} PROPERTIES COMPILE_OPTIONS "-w;-Werror=implicit-function-declaration")
set_source_files_properties(${GLUE_SOURCES} PROPERTIES COMPILE_OPTIONS "-Wall;-Wno-unused-function")
if(WIN32)
    # GCC (not MSVC) bitfield layout, as on Linux and the EE
    target_compile_options(recvx_game PRIVATE -mno-ms-bitfields)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # the executable is non-PIE (fixed address window); position-independent code only costs
    # get_pc_thunk calls on x86-32
    target_compile_options(recvx_game PRIVATE -fno-pie)
endif()

# ---------------------------------------------------------------- platform layer
file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(recvx_platform OBJECT ${PLATFORM_SOURCES})
set_property(SOURCE src/host/host.c src/host/overlay.cpp APPEND PROPERTY COMPILE_DEFINITIONS "RECVX_VERSION=\"${RECVX_VERSION}\"")
target_include_directories(recvx_platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ps2
)
target_compile_options(recvx_platform PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
    -fno-strict-aliasing -fwrapv -ffp-contract=off -msse2 -mfpmath=sse
    -Wall -Wno-unused-function
)
target_link_libraries(recvx_platform PRIVATE SDL3::SDL3 imgui miniupnpc::miniupnpc natpmp monocypher)
if(TARGET openxr_loader)
    target_link_libraries(recvx_platform PRIVATE OpenXR::headers)
    target_compile_definitions(recvx_platform PRIVATE RECVX_HAVE_OPENXR=1)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(recvx_platform PRIVATE -fno-pie)
endif()
if(WIN32)
    target_compile_options(recvx_platform PRIVATE $<$<COMPILE_LANGUAGE:C>:-mno-ms-bitfields>)
endif()

# ---------------------------------------------------------------- executable
set(RECVX_LAYOUT_PAD 0 CACHE STRING "test aid: bytes of padding before all code, data and bss (pc/tools/layout_check.sh)")
if(RECVX_LAYOUT_PAD)
    set(LAYOUT_PAD_SRC tests/layout_pad.c)
    set_source_files_properties(tests/layout_pad.c PROPERTIES COMPILE_DEFINITIONS RECVX_LAYOUT_PAD=${RECVX_LAYOUT_PAD})
endif()
add_executable(recvx ${LAYOUT_PAD_SRC} $<TARGET_OBJECTS:recvx_game> $<TARGET_OBJECTS:recvx_platform>
               $<$<TARGET_EXISTS:glibc_compat>:$<TARGET_OBJECTS:glibc_compat>>)
target_link_libraries(recvx PRIVATE imgui SDL3::SDL3 miniupnpc::miniupnpc natpmp monocypher m)
if(WIN32)
    target_link_libraries(recvx PRIVATE ws2_32 pthread shell32 winmm)   # co-op UDP (src/net); clock_gettime (winpthreads)
    if(TARGET openxr_loader)
        target_link_libraries(recvx PRIVATE openxr_loader advapi32 pathcch winhttp)   # winhttp: the VR update check (src/host/update.c)
    endif()
    # Same address window as Linux: fixed image base, no ASLR relocation; large-address-aware
    # gives the 32-bit exe 4 GB on 64-bit Windows (Linux already uses addresses above 2 GB). -static: libstdc++,
    # libgcc and winpthread go into the exe, only system DLLs (UCRT, opengl32, ...) remain.
    target_link_options(recvx PRIVATE -static -Wl,--image-base,0x30000000 -Wl,--disable-dynamicbase -Wl,--large-address-aware
                        -Wl,--stack,8388608)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Place the image inside the PS2 address window (see docs/ARCHITECTURE.md, "Data model and memory").
    target_link_options(recvx PRIVATE -no-pie -Wl,-Ttext-segment=0x30000000 -static-libgcc -static-libstdc++)
    set_target_properties(recvx PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endif()

# ---------------------------------------------------------------- tests
add_executable(test_vu tests/test_vu.c src/ps2/vu.c)
target_compile_options(test_vu PRIVATE -std=gnu11 -msse2 -mfpmath=sse -ffp-contract=off)
target_link_libraries(test_vu PRIVATE m)
add_test(NAME vu COMMAND test_vu)
# the same against the plain C (non-SSE) native paths, as an arm64 build would compile them
add_executable(test_vu_portable tests/test_vu.c src/ps2/vu.c)
target_compile_definitions(test_vu_portable PRIVATE RECVX_VU_PORTABLE)
target_compile_options(test_vu_portable PRIVATE -std=gnu11 -msse2 -mfpmath=sse -ffp-contract=off)
target_link_libraries(test_vu_portable PRIVATE m)
add_test(NAME vu_portable COMMAND test_vu_portable)

# decodes every movie on the disc (needs RECVX_ISO; skipped otherwise)
add_executable(test_movies tests/test_movies.c src/movie/mpeg2.c src/movie/pss.c)
target_compile_options(test_movies PRIVATE -std=gnu11 -O2)
target_link_libraries(test_movies PRIVATE m)
add_test(NAME movies COMMAND test_movies)
set_tests_properties(movies PROPERTIES SKIP_RETURN_CODE 77)

# control bindings (pc/src/input/bindings.c): defaults, rebinding, ini, a virtual SDL gamepad
add_executable(test_bindings tests/test_bindings.c src/input/bindings.c)
target_compile_options(test_bindings PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=gnu11>)
target_link_libraries(test_bindings PRIVATE SDL3::SDL3)
add_test(NAME bindings COMMAND test_bindings)

# modern controls' direction math (pc/src/input/modern.c): camera + stick -> angle, the cut hold
add_executable(test_modern tests/test_modern.c src/input/modern.c)
target_compile_options(test_modern PRIVATE -std=gnu11)
target_link_libraries(test_modern PRIVATE m)
add_test(NAME modern COMMAND test_modern)

# every recvx.ini key has a row in the settings table (F1 and OPTION > ENHANCED SETTINGS) or is listed
add_test(NAME settings COMMAND ${CMAKE_COMMAND} -DSRC=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_settings.cmake)

# "You appear as" list (pc/src/host/actors.c): event doubles hidden, indices kept
add_executable(test_appear tests/test_appear.c src/host/actors.c)
add_test(NAME appear COMMAND test_appear)

# co-op networking (pc/src/net): a busy port, the port mapping started and stopped again
add_executable(test_net tests/test_net.c src/net/net.c src/net/portmap.c)
target_compile_options(test_net PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=gnu11>)
target_link_libraries(test_net PRIVATE SDL3::SDL3 miniupnpc::miniupnpc natpmp $<$<BOOL:${WIN32}>:ws2_32>)
add_test(NAME net COMMAND test_net)

# co-op packets (pc/src/net/coop_proto.c): malformed events of every type, transfers, the event
# ring, the join code, world event actions, and a fuzz loop
add_executable(test_coop tests/test_coop.c src/net/coop_proto.c src/net/rdv_proto.c)
target_compile_options(test_coop PRIVATE -std=gnu11)
target_link_libraries(test_coop PRIVATE monocypher m)
add_test(NAME coop COMMAND test_coop)

# UTF-8 file paths and replace-on-rename saving (pc/src/host/fileio.c)
add_executable(test_fileio tests/test_fileio.c src/host/fileio.c)
target_compile_options(test_fileio PRIVATE -std=gnu11)
add_test(NAME fileio COMMAND test_fileio)

# native save files (pc/src/host/savestore.c)
add_executable(test_savestore tests/test_savestore.c src/host/savestore.c src/host/fileio.c)
target_compile_options(test_savestore PRIVATE -std=gnu11)
target_link_libraries(test_savestore PRIVATE SDL3::SDL3)
add_test(NAME savestore COMMAND test_savestore)

# co-op server protocol and server core (src/net/rdv_proto.c, server/server.c): messages, malformed
# input and fuzz, the end-to-end encryption, room codes (unique, collisions, grace period), hijacks,
# CREATE retries, passwords, relay limits
add_executable(test_rdv tests/test_rdv.c)
target_compile_options(test_rdv PRIVATE -std=gnu11)
target_link_libraries(test_rdv PRIVATE recvx_server_core)
target_compile_definitions(test_rdv PRIVATE RECVX_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests")
add_test(NAME rdv COMMAND test_rdv)
# the rendezvous client state machine against the server core on a fake network (deterministic)
add_executable(test_rdv_client tests/test_rdv_client.c src/net/rendezvous.c)
target_compile_options(test_rdv_client PRIVATE -std=gnu11)
target_link_libraries(test_rdv_client PRIVATE recvx_server_core)
if(WIN32)
    target_link_libraries(test_rdv_client PRIVATE ws2_32)
endif()
add_test(NAME rdv_client COMMAND test_rdv_client)
# fuzzing under AddressSanitizer + UBSan: parser, server and client state machines (Linux, GCC/Clang)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RDV_SAN -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_executable(fuzz_rdv tests/fuzz_rdv.c src/net/rendezvous.c ${CMAKE_CURRENT_SOURCE_DIR}/server/server.c
                   ${CMAKE_CURRENT_SOURCE_DIR}/server/config.c src/net/rdv_proto.c)
    target_compile_options(fuzz_rdv PRIVATE -std=gnu11 -O1 -g ${RDV_SAN})
    target_link_options(fuzz_rdv PRIVATE ${RDV_SAN})
    target_link_libraries(fuzz_rdv PRIVATE monocypher)
    add_test(NAME rdv_fuzz COMMAND fuzz_rdv 300000)
endif()
