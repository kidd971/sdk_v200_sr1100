target_include_directories(stm32_mw_usb_device PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(stm32_mw_usb_device PRIVATE usbd_conf.c)
