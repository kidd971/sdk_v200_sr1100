target_sources(datalink_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node.h
)

target_include_directories(datalink_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
