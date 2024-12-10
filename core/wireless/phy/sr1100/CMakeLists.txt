if(RADIO_INTERFACE_QSPI)
    set(INTERFACE_DIRECTORY qspi)
else()
    set(INTERFACE_DIRECTORY spi)
endif()

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${INTERFACE_DIRECTORY})

target_sources(swc
    PRIVATE
        sr_nvm.c
        sr_nvm_private.c
        sr_calib.c
        sr_spectral.c
    PUBLIC
        sr1100_def.h
        sr1100_v3_reg.h
        sr_calib.h
        sr_def.h
        sr_nvm.h
        sr_nvm_private.h
        sr_reg.h
        sr_spectral.h
        sr_utils.h
)
