add_library(fixed_point "")

target_sources(fixed_point
    PRIVATE
        fixed_point.c
    PUBLIC
        fixed_point.h
)

target_include_directories(fixed_point PUBLIC ${CMAKE_CURRENT_LIST_DIR})
