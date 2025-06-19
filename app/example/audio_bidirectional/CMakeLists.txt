add_library(audio_bidirectional_backend "")
target_include_directories(audio_bidirectional_backend PUBLIC facade)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
else()
    add_subdirectory(swc_cfg_sr1100)
endif()

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(audio_bidirectional_coordinator.elf "")
    target_sources(audio_bidirectional_coordinator.elf PRIVATE audio_bidirectional_coord.c)
    target_link_libraries(audio_bidirectional_coordinator.elf
        PRIVATE
            audio_core
            audio_bidirectional_backend
            swc
    )
    add_custom_command(TARGET audio_bidirectional_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary audio_bidirectional_coordinator.elf audio_bidirectional_coordinator.bin
    )
endif()

add_executable(audio_bidirectional_node.elf "")
target_sources(audio_bidirectional_node.elf PRIVATE audio_bidirectional_node.c)
target_link_libraries(audio_bidirectional_node.elf
    PRIVATE
        audio_core
        audio_bidirectional_backend
        swc
)
add_custom_command(TARGET audio_bidirectional_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_bidirectional_node.elf audio_bidirectional_node.bin
)
