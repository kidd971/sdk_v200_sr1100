add_library(pseudo_data "")

target_sources(pseudo_data
    PRIVATE
        pseudo_data.c
    PUBLIC
        pseudo_data.h
)

target_include_directories(pseudo_data PUBLIC ${CMAKE_CURRENT_LIST_DIR})
