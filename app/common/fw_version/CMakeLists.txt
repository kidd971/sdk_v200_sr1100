# Header only: a version string has nothing to compile, but it does need to be reachable
# from both app/common (at_cmd_core) and app/example (the applications), which is what an
# INTERFACE target is for.
add_library(fw_version INTERFACE)

target_sources(fw_version INTERFACE fw_version.h)

target_include_directories(fw_version INTERFACE ${CMAKE_CURRENT_LIST_DIR})
