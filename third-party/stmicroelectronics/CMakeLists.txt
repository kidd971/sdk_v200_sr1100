if(${MCU_DRIVER} STREQUAL STMG4XX)
    add_subdirectory(cmsis_device_g4)
    add_subdirectory(stm32g4xx_hal_driver)
    add_subdirectory(stm32_mw_usb_device)
elseif(${MCU_DRIVER} STREQUAL STMU5XX)
    add_subdirectory(cmsis_device_u5)
    add_subdirectory(stm32u5xx_hal_driver)
endif()



