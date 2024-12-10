target_sources(swc
    PUBLIC
        xlayer.h
        xlayer_queue.h
        xlayer_queue.c
        xlayer_circular_data.h
        xlayer_circular_data.c
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
