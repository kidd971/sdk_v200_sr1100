add_library(middleware_tinyusb "")

# Set Descriptors product name based on target configuration.
add_compile_definitions(TUSB_PRODUCT_NAME="${HARDWARE}")


add_subdirectory(tiny_usb_app_config) # Set path to applicative configurations.
add_subdirectory(backend)   #Set path to hardware specific implementations.

target_link_libraries(middleware_tinyusb PUBLIC hardware)
target_include_directories(middleware_tinyusb
    PUBLIC
        src
        src/src
        src/hw/bsp
        src/src/class/cdc
        src/src/common
        src/src/device
        src/src/portable/synopsys/dwc2
        ${TINY_USB_CONFIG_PATH}
        ${FREERTOS_TUSB_INC_DIR_PATH}
)

target_sources(middleware_tinyusb
    PRIVATE
        src/src/tusb.c
        src/src/class/cdc/cdc_device.c
        src/src/common/tusb_fifo.c
        src/src/device/usbd_control.c
        src/src/device/usbd.c
        src/src/portable/synopsys/dwc2/dcd_dwc2.c
        src/hw/bsp/board.c
        src/hw/bsp/stm32u5
        src/hw/bsp/stm32g4
        src/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
        ${TINY_USB_BACKEND_PATH}/backend.c
        ${TINY_USB_CONFIG_PATH}/tusb_descriptors.c
)


