if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(sr1000)
else()
    add_subdirectory(sr1100)
endif()

target_sources(swc
    PRIVATE
        sr_phy_hal.c
        sr_access.c
        sr_utils.c
    PUBLIC
        sr_phy_error.h
        sr_phy_hal.h
        sr_utils.h
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
