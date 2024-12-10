set(TINY_USB_CONFIG_PATH
    ${CMAKE_CURRENT_LIST_DIR}
    CACHE STRING "Path to TinyUSB descriptor configuration files"
)

target_link_libraries(middleware_tinyusb PUBLIC cmsis_rtos_v2)
