add_library(module_tinyusb_backend "")

set_target_properties(module_tinyusb_backend PROPERTIES LINKER_LANGUAGE C)

target_link_libraries(module_tinyusb_backend PRIVATE hardware)

target_include_directories(module_tinyusb_backend PUBLIC ${PROJECT_SOURCE_DIR}/module/tinyusb_module)

if (HARDWARE STREQUAL "EVK")
    target_sources(module_tinyusb_backend PRIVATE evk_backend.c)
elseif(HARDWARE STREQUAL "PULSAR")
    target_sources(module_tinyusb_backend PRIVATE pulsar_backend.c)
elseif(HARDWARE STREQUAL "QUASAR")
    target_sources(module_tinyusb_backend PRIVATE quasar_backend.c)
endif()
