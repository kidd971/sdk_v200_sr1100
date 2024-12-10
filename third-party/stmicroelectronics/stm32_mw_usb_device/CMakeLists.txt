add_library(stm32_mw_usb_device "")

set(ST_USB_PATH ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(stm32_mw_usb_device
    PRIVATE
        stm32g4xx_hal_driver
    PUBLIC
        cmsis_device_g4
)

add_subdirectory(st_usb_app_config)

target_sources(stm32_mw_usb_device
    PRIVATE
        ${ST_USB_PATH}/Class/CDC/Src/usbd_cdc.c
        ${ST_USB_PATH}/Core/Src/usbd_core.c
        ${ST_USB_PATH}/Core/Src/usbd_ctlreq.c
        ${ST_USB_PATH}/Core/Src/usbd_ioreq.c
    PUBLIC
        ${ST_USB_PATH}/Class/CDC/Inc/usbd_cdc.h
        ${ST_USB_PATH}/Core/Inc/usbd_core.h
        ${ST_USB_PATH}/Core/Inc/usbd_ctlreq.h
        ${ST_USB_PATH}/Core/Inc/usbd_ioreq.h
)



target_include_directories(stm32_mw_usb_device
    PUBLIC
        ${ST_USB_PATH}/Class/CDC/Inc
        ${ST_USB_PATH}/Core/Inc
)


