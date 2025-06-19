add_library(audio_unidirectional_backend "")
target_include_directories(audio_unidirectional_backend PUBLIC facade)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
else()
    add_subdirectory(swc_cfg_sr1100)
endif()

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
