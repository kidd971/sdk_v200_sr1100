target_include_directories(sr_phy PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(sr_phy
    PRIVATE
        sr_nvm.c
        sr_nvm_private.c
        sr_calib.c
        sr_spectral.c
)
