add_library(audio_unidirectional_backend "")
target_include_directories(audio_unidirectional_backend PUBLIC facade)
target_link_libraries(audio_unidirectional_backend PUBLIC common_backend)

# Link config files to backend
target_include_directories(audio_unidirectional_backend PUBLIC config)
target_link_libraries(audio_unidirectional_backend PRIVATE audio_core)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(audio_unidirectional_coordinator.elf "")
    target_sources(audio_unidirectional_coordinator.elf PRIVATE audio_unidirectional_coord.c)
    target_link_libraries(audio_unidirectional_coordinator.elf PRIVATE
        PRIVATE
            audio_core
            audio_unidirectional_backend
            swc
    )
    add_custom_command(TARGET audio_unidirectional_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary audio_unidirectional_coordinator.elf audio_unidirectional_coordinator.bin
    )
endif()

add_executable(audio_unidirectional_node.elf "")
target_sources(audio_unidirectional_node.elf PRIVATE audio_unidirectional_node.c)
target_link_libraries(audio_unidirectional_node.elf
    PRIVATE
        audio_core
        audio_unidirectional_backend
        swc
)
add_custom_command(TARGET audio_unidirectional_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_unidirectional_node.elf audio_unidirectional_node.bin
)
