add_library(at_cmd_core "")

target_sources(at_cmd_core
    PRIVATE
        at_cmd_core.c
    PUBLIC
        at_cmd_core.h
        at_cmd_core_facade.h
)

target_include_directories(at_cmd_core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(at_cmd_core PUBLIC at_module swc)
