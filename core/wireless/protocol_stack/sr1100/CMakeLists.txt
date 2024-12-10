target_sources(swc
    PRIVATE
        wps_phy_common.c
        wps_debug.c
    PUBLIC
        wps_debug.h
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})

