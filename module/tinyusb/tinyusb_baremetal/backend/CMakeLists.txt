set_target_properties(module_tinyusb_baremetal_backend PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(module_tinyusb_baremetal_backend PRIVATE hardware)

if (HARDWARE STREQUAL "EVK")
    target_sources(module_tinyusb_baremetal_backend PRIVATE evk_backend.c)
elseif(HARDWARE STREQUAL "QUASAR")
    target_sources(module_tinyusb_baremetal_backend PRIVATE quasar_backend.c)
endif()
