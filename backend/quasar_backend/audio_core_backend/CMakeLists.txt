target_link_libraries(audio_core PRIVATE hardware)
target_sources(audio_core PRIVATE audio_core_cdc_backend.c)

if (USB_AUDIO_ENABLED)
    target_link_libraries(audio_core PRIVATE middleware_tinyusb module_tinyusb_audio)
    target_sources(audio_core PRIVATE audio_core_tinyusb_backend.c)
else()
    target_sources(audio_core PRIVATE audio_core_i2s_backend.c)
endif()
