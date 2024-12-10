add_library(critical_section "")

set_target_properties(critical_section PROPERTIES LINKER_LANGUAGE C)

target_include_directories(critical_section PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(critical_section PRIVATE critical_section.c)
