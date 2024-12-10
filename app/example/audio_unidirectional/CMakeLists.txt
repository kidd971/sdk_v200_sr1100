add_library(audio_unidirectional_backend "")
target_include_directories(audio_unidirectional_backend PUBLIC facade)

add_executable(audio_unidirectional_coordinator.elf "")
target_sources(audio_unidirectional_coordinator.elf PRIVATE audio_unidirectional_coord.c)

add_executable(audio_unidirectional_node.elf "")
target_sources(audio_unidirectional_node.elf PRIVATE audio_unidirectional_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(audio_unidirectional_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(audio_unidirectional_node.elf PRIVATE swc_cfg_sr1000/swc_cfg_node.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(audio_unidirectional_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(audio_unidirectional_node.elf PRIVATE swc_cfg_sr1100/swc_cfg_node.h)
endif()

target_link_libraries(audio_unidirectional_coordinator.elf PRIVATE
    PRIVATE
        audio_core
        audio_unidirectional_backend
        swc)

add_custom_command(TARGET audio_unidirectional_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_unidirectional_coordinator.elf audio_unidirectional_coordinator.bin
)

target_link_libraries(audio_unidirectional_node.elf
    PRIVATE
        audio_core
        audio_unidirectional_backend
        swc)

add_custom_command(TARGET audio_unidirectional_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary audio_unidirectional_node.elf audio_unidirectional_node.bin
)
