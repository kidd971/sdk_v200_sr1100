add_library(hardware "")

set(BSP_PATH ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "Path to bsp source files for use by this CMakeLists.")
set(STM32U5_HAL_CONF_PATH ${BSP_PATH} CACHE INTERNAL "Path to stm32u5xx_hal_conf.h for use by stm32u5xx_hal_driver lib")
set(STM32U5_LINKER_SCRIPT_PATH ${BSP_PATH}/GCC/STM32U5A5AJHXQ_FLASH.ld CACHE INTERNAL "Path to CMAKE_EXE_LINKER_FLAG use by gcc-arm-flags.cmake")
set(STM32U5_MCU_MODEL STM32U5A5xx CACHE INTERNAL "STM32U5 MCU model definition use by stm32u5xx_hal_driver lib")

if (RTOS_ENABLED)
    target_compile_definitions(hardware PRIVATE RTOS_ENABLED)
endif()

message("ST HAL Driver using MCU model: ${STM32U5_MCU_MODEL}")

target_link_libraries(hardware PUBLIC stm32u5xx_hal_driver cmsis_device_u5)

target_include_directories(hardware PUBLIC ${BSP_PATH})

target_sources(hardware
    PRIVATE
        ${BSP_PATH}/GCC/startup_stm32u5a5ajhxq.s
        ${BSP_PATH}/quasar.c
        ${BSP_PATH}/quasar_adc.c
        ${BSP_PATH}/quasar_audio.c
        ${BSP_PATH}/quasar_button.c
        ${BSP_PATH}/quasar_clock.c
        ${BSP_PATH}/quasar_debug.c
        ${BSP_PATH}/quasar_dma.c
        ${BSP_PATH}/quasar_fifo.c
        ${BSP_PATH}/quasar_gpio.c
        ${BSP_PATH}/quasar_i2c.c
        ${BSP_PATH}/quasar_it.c
        ${BSP_PATH}/quasar_led.c
        ${BSP_PATH}/quasar_memory.c
        ${BSP_PATH}/quasar_power.c
        ${BSP_PATH}/quasar_pwm.c
        ${BSP_PATH}/quasar_radio.c
        ${BSP_PATH}/quasar_rgb.c
        ${BSP_PATH}/quasar_spi.c
        ${BSP_PATH}/quasar_timer.c
        ${BSP_PATH}/quasar_timer_ext.c
        ${BSP_PATH}/quasar_uart.c
        ${BSP_PATH}/quasar_usb.c
        ${BSP_PATH}/syscalls.c
        ${BSP_PATH}/system_stm32u5xx.c
)

target_compile_definitions(hardware PUBLIC RADIO_QSPI_ENABLED=0)
