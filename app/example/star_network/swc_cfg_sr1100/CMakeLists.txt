target_sources(star_network_backend
    PUBLIC
        swc_cfg_coord.h
        swc_cfg_node1.h
        swc_cfg_node2.h
)

target_include_directories(star_network_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})
