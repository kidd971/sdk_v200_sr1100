if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(sr1000)
else()
    add_subdirectory(sr1100)
endif()

target_sources(swc
    PUBLIC
        sr_phy_error.h
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
