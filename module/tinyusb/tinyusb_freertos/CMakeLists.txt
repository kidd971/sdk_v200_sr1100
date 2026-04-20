add_library(module_tinyusb_freertos_backend "")
target_include_directories(module_tinyusb_freertos_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(module_tinyusb_freertos "")
set_target_properties(module_tinyusb_freertos PROPERTIES LINKER_LANGUAGE C)

target_include_directories(module_tinyusb_freertos PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(module_tinyusb_freertos PRIVATE tinyusb_freertos.c)

target_link_libraries(module_tinyusb_freertos
    PUBLIC
        hardware
        middleware_tinyusb
    PRIVATE
        module_tinyusb_freertos_backend
        cmsis_rtos_v2
)

add_subdirectory(backend)
