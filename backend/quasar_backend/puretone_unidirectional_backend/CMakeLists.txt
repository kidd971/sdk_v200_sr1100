if (NOT TRANSCEIVER STREQUAL "SR1100")
    return()
endif()

# at_cmd_core supplies the facade prototypes this backend implements (and only those --
# the core itself is linked by the applications).
target_link_libraries(puretone_unidirectional_backend PRIVATE hardware module_tinyusb_baremetal at_cmd_core)
target_sources(puretone_unidirectional_backend PRIVATE puretone_unidirectional_backend.c)

if (USB_AUDIO_ENABLED)
    target_link_libraries(puretone_unidirectional_backend PRIVATE middleware_tinyusb module_tinyusb_audio)
    target_sources(puretone_unidirectional_backend PRIVATE puretone_unidirectional_tinyusb_backend.c)
else()
    target_link_libraries(puretone_unidirectional_backend PRIVATE max98091_driver)
    target_sources(puretone_unidirectional_backend PRIVATE puretone_unidirectional_i2s_backend.c)
endif()
