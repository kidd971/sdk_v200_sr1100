add_library(common_backend "")
target_include_directories(common_backend PUBLIC ${CMAKE_SOURCE_DIR}/app/example/common)

if (RTOS_ENABLED)
    target_link_libraries(common_backend PRIVATE hardware module_tinyusb_freertos)
else()
    target_link_libraries(common_backend PRIVATE hardware module_tinyusb_baremetal)
endif()

target_sources(common_backend PRIVATE common_backend.c)
