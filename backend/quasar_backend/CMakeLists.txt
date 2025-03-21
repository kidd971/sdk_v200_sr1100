# Core
add_subdirectory(wireless_core_backend)

# SDK Example Application
if (APP STREQUAL "Audio-Unidirectional")
    add_subdirectory(audio_unidirectional_backend)
    add_subdirectory(audio_core_backend)
elseif (APP STREQUAL "BSP-validator")
    add_subdirectory(bsp_validator_backend)
elseif (APP STREQUAL "All")
    add_subdirectory(audio_unidirectional_backend)
    add_subdirectory(bsp_validator_backend)
    add_subdirectory(audio_core_backend)
endif()



