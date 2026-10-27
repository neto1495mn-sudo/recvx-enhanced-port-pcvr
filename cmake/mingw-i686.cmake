# Cross-build the 32-bit Windows executable with mingw-w64 GCC (see docs/BUILDING.md, "Windows").
#   cmake -B build-win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake -DMINGW_PREFIX=/path/to/usr
# GCC, not clang: the decomp relies on GCC's data layout (arrays of 32+ bytes are 32-byte aligned,
# which the EE packet buffers need); llvm-mingw only aligns them naturally and misrenders.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(MINGW_PREFIX "$ENV{MINGW_PREFIX}" CACHE PATH "directory holding bin/i686-w64-mingw32-gcc (empty: PATH)")
if(MINGW_PREFIX)
    set(_p "${MINGW_PREFIX}/bin/")
endif()
set(CMAKE_C_COMPILER   ${_p}i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER ${_p}i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  ${_p}i686-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
