if (MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    set(PHY_DIRECTORY multi_radio)
else()
    set(PHY_DIRECTORY single_radio)
endif()

target_sources(swc
    PRIVATE
        wps.c
        wps_callback.c
        wps_mac.c
        wps_mac_certification.c
        wps_mac_protocols.c
        wps_mac_statistics.c
        wps_mac_timeslots.c
        wps_mac_xlayer.c
        wps_conn_priority.c
        wps_stats.c
        wps_utils.c
        wps_connection_list.c
        ${PHY_DIRECTORY}/wps_phy.c
    PUBLIC
        wps.h
        wps_callback.h
        wps_def.h
        wps_error.h
        wps_mac.h
        wps_mac_certification.h
        wps_mac_protocols.h
        wps_mac_statistics.h
        wps_mac_timeslots.h
        wps_mac_xlayer.h
        wps_conn_priority.h
        wps_stats.h
        wps_utils.h
        wps_connection_list.h
        ${PHY_DIRECTORY}/wps_phy.h
)

if(NOT WPS_DISABLE_FRAGMENTATION)
    target_sources(swc
        PRIVATE
            wps_frag.c
        PUBLIC
            wps_frag.h
    )
endif()

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(sr1000)
else()
    add_subdirectory(sr1100)
endif()

if (RADIO_INTERFACE_QSPI)
    target_compile_definitions(swc PUBLIC RADIO_QSPI_ENABLED=1)
else()
    target_compile_definitions(swc PUBLIC RADIO_QSPI_ENABLED=0)
endif()

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${PHY_DIRECTORY})
