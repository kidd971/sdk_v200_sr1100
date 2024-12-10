target_sources(fragmentation_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node.h
)

target_include_directories(fragmentation_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
