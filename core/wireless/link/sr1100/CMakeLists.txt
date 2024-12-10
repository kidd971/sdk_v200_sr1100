target_sources(swc
    PRIVATE
        link_cca.c
        link_gain_loop.c
        link_tdma_sync.c
    PUBLIC
        link_cca.h
        link_gain_loop.h
        link_tdma_sync.h
        link_utils.h
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
