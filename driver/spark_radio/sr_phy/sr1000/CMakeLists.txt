if (RADIO_INTERFACE_QSPI)
    message(FATAL_ERROR "QSPI interface is not supported on SR1000 transceiver.")
endif()

target_include_directories(sr_phy PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(sr_phy
    PRIVATE
        sr_calib.c
        sr_nvm.c
        sr_nvm_private.c
        sr_spectral.c
)

