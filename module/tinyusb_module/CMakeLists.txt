add_library(module_tinyusb "")

set_target_properties(module_tinyusb PROPERTIES LINKER_LANGUAGE C)

add_subdirectory(backend)

target_include_directories(module_tinyusb PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(module_tinyusb PRIVATE tinyusb_module.c)

target_link_libraries(module_tinyusb
    PUBLIC
        hardware
        middleware_tinyusb
    PRIVATE
        module_tinyusb_backend
        cmsis_rtos_v2
    )
