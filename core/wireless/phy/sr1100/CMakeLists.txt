target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if (NOT BUILD_TESTS)
    target_link_libraries(swc PRIVATE hardware)
endif()

target_sources(swc
    PRIVATE
        sr_nvm.c
        sr_nvm_private.c
        sr_calib.c
        sr_spectral.c
    PUBLIC
        sr1100_def.h
        sr1120_V3_reg.h
        sr_access.h
        sr_calib.h
        sr_def.h
        sr_nvm.h
        sr_nvm_private.h
        sr_spectral.h
)
