add_library(module_tinyusb_baremetal "")
set_target_properties(module_tinyusb_baremetal PROPERTIES LINKER_LANGUAGE C)

add_subdirectory(backend)

target_include_directories(module_tinyusb_baremetal PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(module_tinyusb_baremetal PRIVATE tinyusb_module_baremetal.c)

target_link_libraries(module_tinyusb_baremetal
    PUBLIC
        hardware
        middleware_tinyusb
    PRIVATE
        module_tinyusb_baremetal_backend
    )
