target_sources(audio_unidirectional_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node.h
)

target_include_directories(audio_unidirectional_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
