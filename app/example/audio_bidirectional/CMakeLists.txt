add_library(audio_bidirectional_backend "")
target_include_directories(audio_bidirectional_backend PUBLIC facade)

add_executable(audio_bidirectional_coordinator.elf "")
target_sources(audio_bidirectional_coordinator.elf PRIVATE audio_bidirectional_coord.c)

add_executable(audio_bidirectional_node.elf "")
target_sources(audio_bidirectional_node.elf PRIVATE audio_bidirectional_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(audio_bidirectional_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(audio_bidirectional_node.elf PRIVATE swc_cfg_sr1000/swc_cfg_node.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(audio_bidirectional_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(audio_bidirectional_node.elf PRIVATE swc_cfg_sr1100/swc_cfg_node.h)
endif()

target_link_libraries(audio_bidirectional_coordinator.elf
    PRIVATE
        audio_core
        audio_bidirectional_backend
        swc)
add_custom_command(TARGET audio_bidirectional_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_bidirectional_coordinator.elf audio_bidirectional_coordinator.bin
)

target_link_libraries(audio_bidirectional_node.elf
    PRIVATE
        audio_core
        audio_bidirectional_backend
        swc)
add_custom_command(TARGET audio_bidirectional_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_bidirectional_node.elf audio_bidirectional_node.bin
)
