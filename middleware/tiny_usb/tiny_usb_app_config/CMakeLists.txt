if (TINY_USB_CFG STREQUAL "TINY_USB_SINGLE_HS_BAREMETAL")
    add_subdirectory(single_cdc_high_speed_baremetal)
elseif(TINY_USB_CFG STREQUAL "TINY_USB_SINGLE_FS_BAREMETAL")
    add_subdirectory(single_cdc_full_speed_baremetal)
elseif(TINY_USB_CFG STREQUAL "TINY_USB_SINGLE_FS_RTOS")
    add_subdirectory(single_cdc_full_speed_rtos)
else()
    message("Cannot find the Tiny USB configuration: ${TINY_USB_CFG}")
endif()

#Set USB descriptor based on MCU selected.
if(${MCU_DRIVER} STREQUAL STMU5XX)
    target_compile_definitions(middleware_tinyusb
        PUBLIC
            CFG_TUSB_MCU=OPT_MCU_STM32U5
            TUSB_VID=1155   # STMicroelectronics
            TUSB_PID=22336  # Virtual COM Port
    )
elseif(${MCU_DRIVER} STREQUAL STMG4XX)
    target_compile_definitions(middleware_tinyusb
        PUBLIC
            CFG_TUSB_MCU=OPT_MCU_STM32G4
            TUSB_VID=1155   # STMicroelectronics
            TUSB_PID=22336  # Virtual COM Port
    )
endif()
