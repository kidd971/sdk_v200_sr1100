add_library(hardware "")

set(BSP_PATH ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "Path to evk source files for use by this CMakeLists.")
set(STM32G4_LINKER_SCRIPT_PATH ${BSP_PATH}/GCC/STM32G473RETX_FLASH.ld CACHE INTERNAL "Path to CMAKE_EXE_LINKER_FLAG use by gcc-arm-flags.cmake")
set(STM32G4_USB_DEVICE_APP_PATH ${BSP_PATH}/usb_device/app CACHE INTERNAL "Path to usb_device/app/*.h for use by stm32_mw_usb_device lib")
set(STM32G4_USB_DEVICE_TARGET_PATH ${BSP_PATH}/usb_device/target CACHE INTERNAL "Path to usb_device/target/*.h for use by stm32_mw_usb_device lib")
set(STM32G4_MCU_MODEL STM32G473xx CACHE INTERNAL "STM32G4 MCU model definition use by stm32g4xx_hal_driver lib")

if (RTOS_ENABLED)
    target_compile_definitions(hardware PRIVATE RTOS_ENABLED)
endif()

# This must be added to the linker's search path for it to find extra "ld" control scripts (such as ccmram_section.ld)
set(STM32_MCU_CCMRAM_LINKER_SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR}/src/GCC/ CACHE INTERNAL "CCMRAM Linker script argument path")

message("ST HAL Driver using MCU model: ${STM32G4_MCU_MODEL}")

message(${PROJECT_SOURCE_DIR})
message(${CMAKE_CURRENT_LIST_DIR})

target_sources(hardware
    PRIVATE
        ${BSP_PATH}/GCC/startup_stm32g473retx.s
        ${BSP_PATH}/evk.c
        ${BSP_PATH}/evk_audio.c
        ${BSP_PATH}/evk_button.c
        ${BSP_PATH}/evk_clock.c
        ${BSP_PATH}/evk_dbg.c
        ${BSP_PATH}/evk_flash.c
        ${BSP_PATH}/evk_it.c
        ${BSP_PATH}/evk_led.c
        ${BSP_PATH}/evk_mpu.c
        ${BSP_PATH}/evk_power.c
        ${BSP_PATH}/evk_radio.c
        ${BSP_PATH}/evk_timer.c
        ${BSP_PATH}/evk_timer_ext.c
        ${BSP_PATH}/evk_uart.c
        ${BSP_PATH}/evk_usb.c
        ${BSP_PATH}/syscalls.c
        ${BSP_PATH}/system_stm32g4xx.c
        ${BSP_PATH}/usb_device/evk_usb_device.c
        ${BSP_PATH}/usb_device/app/usb_device.c
        ${BSP_PATH}/usb_device/app/usbd_cdc_if.c
        ${BSP_PATH}/usb_device/app/usbd_desc.c
        ${BSP_PATH}/usb_device/target/usbd_conf.c
    PUBLIC
        ${BSP_PATH}/evk.h
        ${BSP_PATH}/evk_audio.h
        ${BSP_PATH}/evk_button.h
        ${BSP_PATH}/evk_clock.h
        ${BSP_PATH}/evk_dbg.h
        ${BSP_PATH}/evk_flash.h
        ${BSP_PATH}/evk_it.h
        ${BSP_PATH}/evk_led.h
        ${BSP_PATH}/evk_mpu.h
        ${BSP_PATH}/evk_power.h
        ${BSP_PATH}/evk_radio.h
        ${BSP_PATH}/evk_timer.h
        ${BSP_PATH}/evk_timer_ext.h
        ${BSP_PATH}/evk_uart.h
        ${BSP_PATH}/evk_usb.h
        ${BSP_PATH}/stm32g4xx_hal_conf.h
        ${BSP_PATH}/usb_device/evk_usb_device.h
        ${BSP_PATH}/usb_device/app/usb_device.h
        ${BSP_PATH}/usb_device/app/usbd_cdc_if.h
        ${BSP_PATH}/usb_device/app/usbd_desc.h
        ${BSP_PATH}/usb_device/target/usbd_conf.h
)

target_link_libraries(hardware PUBLIC stm32g4xx_hal_driver stm32_mw_usb_device)
target_include_directories(hardware PUBLIC ${BSP_PATH} ${BSP_PATH}/usb_device ${BSP_PATH}/usb_device/app ${BSP_PATH}/usb_device/target)
