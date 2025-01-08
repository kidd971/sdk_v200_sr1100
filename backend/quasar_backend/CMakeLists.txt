# Core
add_subdirectory(wireless_core_backend)

# SDK Example Application
if (APP STREQUAL "Datalink")
    add_subdirectory(datalink_backend)
elseif (APP STREQUAL "BSP-validator")
    add_subdirectory(bsp_validator_backend)
elseif (APP STREQUAL "All")
    add_subdirectory(datalink_backend)
    add_subdirectory(bsp_validator_backend)
endif()



