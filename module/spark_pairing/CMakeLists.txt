add_library(spark_pairing "")

target_sources(spark_pairing
    PRIVATE
        api/pairing_api.c
        api/pairing_error.c
        communication/pairing_address.c
        communication/pairing_event.c
        utilities/pairing_security.c
        utilities/pairing_timer.c
        state/pairing_state.c
        state/pairing_state_coordinator.c
        state/pairing_state_machine.c
        state/pairing_state_node.c
        wireless/pairing_wireless.c
)

target_include_directories(spark_pairing
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/api
        ${CMAKE_CURRENT_LIST_DIR}/communication
        ${CMAKE_CURRENT_LIST_DIR}/state
        ${CMAKE_CURRENT_LIST_DIR}/utilities
        ${CMAKE_CURRENT_LIST_DIR}/wireless
)

if (NOT BUILD_TESTS)
    target_link_libraries(spark_pairing PUBLIC swc swc_backend)
else()
    target_link_libraries(spark_pairing PUBLIC swc)
endif()

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(wireless/pairing_wireless_cfg_sr1000)
    target_sources(spark_pairing PUBLIC wireless/pairing_wireless_cfg_sr1000/pairing_wireless_cfg.h)
else()
    add_subdirectory(wireless/pairing_wireless_cfg_sr1100)
    target_sources(spark_pairing PUBLIC wireless/pairing_wireless_cfg_sr1100/pairing_wireless_cfg.h)
endif()
