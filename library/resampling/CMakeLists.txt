add_library(resampling "")

target_sources(resampling
    PRIVATE
        resampling.c
    PUBLIC
        resampling.h
)

target_include_directories(resampling PUBLIC ${CMAKE_CURRENT_LIST_DIR})
