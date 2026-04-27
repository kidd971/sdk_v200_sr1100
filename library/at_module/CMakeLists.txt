add_library(at_module "")

target_sources(at_module
    PRIVATE
        at_module.c
    PUBLIC
        at_module.h
)

target_include_directories(at_module PUBLIC ${CMAKE_CURRENT_LIST_DIR})
