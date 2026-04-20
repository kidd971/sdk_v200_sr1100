if(RTOS_ENABLED)
    target_compile_definitions(middleware_tinyusb PUBLIC CFG_TUSB_OS=OPT_OS_FREERTOS)
    target_link_libraries(middleware_tinyusb PUBLIC cmsis_rtos_v2)
endif()

if(USB_AUDIO_ENABLED)
    add_subdirectory(cdc_audio_hid)
else()
    add_subdirectory(single_cdc)
endif()

#Set USB descriptor based on MCU selected.
if(${MCU_DRIVER} STREQUAL STMU5XX)
    target_compile_definitions(middleware_tinyusb
        PUBLIC
            CFG_TUSB_MCU=OPT_MCU_STM32U5
    )
elseif(${MCU_DRIVER} STREQUAL STMG4XX)
    target_compile_definitions(middleware_tinyusb
        PUBLIC
            CFG_TUSB_MCU=OPT_MCU_STM32G4
    )
endif()
