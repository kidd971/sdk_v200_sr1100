add_library(buffer "")

target_sources(buffer
    PRIVATE
        uwb_circular_buffer.c
    PUBLIC
        uwb_circular_buffer.h
)

target_include_directories(buffer PUBLIC ${CMAKE_CURRENT_LIST_DIR})
