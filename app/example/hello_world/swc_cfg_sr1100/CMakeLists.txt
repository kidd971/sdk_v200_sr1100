target_sources(hello_world_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node.h
)

target_include_directories(hello_world_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
