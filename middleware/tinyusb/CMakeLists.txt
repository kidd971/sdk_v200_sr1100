add_library(middleware_tinyusb "")

# Set Descriptors product name based on target configuration.
add_compile_definitions(TUSB_PRODUCT_NAME="${HARDWARE}")

add_subdirectory(tinyusb_app_config) # Set path to applicative configurations.

target_link_libraries(middleware_tinyusb PRIVATE hardware)
target_include_directories(middleware_tinyusb
    PUBLIC
        src/src
        src/hw/bsp
        src/src/class/cdc
        src/src/common
        src/src/device
        ${FREERTOS_TUSB_INC_DIR_PATH}
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(middleware_tinyusb
    PRIVATE
        src/src/tusb.c
        src/src/class/cdc/cdc_device.c
        src/src/common/tusb_fifo.c
        src/src/device/usbd_control.c
        src/src/device/usbd.c
)

if(USB_AUDIO_ENABLED)
    target_include_directories(middleware_tinyusb
        PUBLIC
            src/src/class/audio
            src/src/class/hid
    )

    target_sources(middleware_tinyusb
        PRIVATE
            src/src/class/audio/audio_device.c
            src/src/class/hid/hid_device.c
    )
endif()

if(${HARDWARE} STREQUAL "QUASAR" AND STM32U5_MCU_MODEL STREQUAL "STM32U535xx")
    # STM32U535 has USB Full Speed Device (FSDEV), not OTG HS (DWC2)
    target_include_directories(middleware_tinyusb
        PUBLIC
            src/src/portable/st/stm32_fsdev
    )
    target_sources(middleware_tinyusb
        PRIVATE
            src/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
    )
    target_compile_definitions(middleware_tinyusb PUBLIC TUP_MEM_CONST_ADDR)
elseif(${HARDWARE} STREQUAL "QUASAR")
    target_include_directories(middleware_tinyusb
        PUBLIC
            src/src/portable/synopsys/dwc2
    )
    target_sources(middleware_tinyusb
        PRIVATE
            src/src/portable/synopsys/dwc2/dwc2_common.c
            src/src/portable/synopsys/dwc2/dcd_dwc2.c
    )
elseif(${HARDWARE} STREQUAL "EVK")
    target_include_directories(middleware_tinyusb
        PUBLIC
            src/src/portable/st/stm32_fsdev
    )
    target_sources(middleware_tinyusb
        PRIVATE
            src/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
    )
endif()

add_subdirectory(backend)   #Set path to hardware specific implementations.

