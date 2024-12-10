add_library(filtering_functions "")

target_sources(filtering_functions
    PRIVATE
        fir_decimate.c
        fir_interpolate.c
    PUBLIC
        filtering_functions.h
)

target_include_directories(filtering_functions PUBLIC ${CMAKE_CURRENT_LIST_DIR})
