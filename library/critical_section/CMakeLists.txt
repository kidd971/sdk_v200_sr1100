add_library(critical_section "")

set_target_properties(critical_section PROPERTIES LINKER_LANGUAGE C)

target_include_directories(critical_section PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (HARDWARE STREQUAL "EVK" OR HARDWARE STREQUAL "QUASAR")
    add_subdirectory(arm_cortex_m)
endif()
