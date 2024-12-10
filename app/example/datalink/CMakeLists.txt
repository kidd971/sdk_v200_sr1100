add_library(datalink_backend "")
target_include_directories(datalink_backend PUBLIC facade)

add_executable(datalink_coordinator.elf "")
target_sources(datalink_coordinator.elf PRIVATE datalink_coord.c)

add_executable(datalink_node.elf "")
target_sources(datalink_node.elf PRIVATE datalink_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(datalink_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(datalink_node.elf PRIVATE swc_cfg_sr1000/swc_cfg_node.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(datalink_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(datalink_node.elf PRIVATE swc_cfg_sr1100/swc_cfg_node.h)
endif()

target_link_libraries(datalink_coordinator.elf
    PRIVATE
        swc
        library_dataforge
        datalink_backend
)

add_custom_command(TARGET datalink_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary datalink_coordinator.elf datalink_coordinator.bin
)

target_link_libraries(datalink_node.elf
    PRIVATE
        swc
        library_dataforge
        datalink_backend
)
add_custom_command(TARGET datalink_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary datalink_node.elf datalink_node.bin
)
