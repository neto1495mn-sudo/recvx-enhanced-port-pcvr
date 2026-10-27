# RECVX_VERSION: the project version plus `git describe` in parentheses, e.g. "0.1.0 (v0.1.0-3-gabc1234)".
# Taken at configure time: re-run cmake to refresh the git part. Included by the top-level
# CMakeLists.txt and by a standalone pc/server build.
execute_process(COMMAND git describe --tags --always --dirty WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
                OUTPUT_VARIABLE RECVX_GIT_DESC OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
set(RECVX_VERSION ${PROJECT_VERSION})
if(RECVX_GIT_DESC)
    string(APPEND RECVX_VERSION " (${RECVX_GIT_DESC})")
endif()
