target_sources(audio_bidirectional_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node.h
)

target_include_directories(audio_bidirectional_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
