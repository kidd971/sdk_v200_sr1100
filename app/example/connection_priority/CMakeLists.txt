add_library(connection_priority_backend "")

target_include_directories(connection_priority_backend PUBLIC facade)

add_executable(connection_priority_coordinator.elf "")
target_sources(connection_priority_coordinator.elf PRIVATE connection_priority_coord.c)

add_executable(connection_priority_node.elf "")
target_sources(connection_priority_node.elf PRIVATE connection_priority_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(connection_priority_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(connection_priority_node.elf PRIVATE swc_cfg_sr1000/swc_cfg_node.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(connection_priority_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(connection_priority_node.elf PRIVATE swc_cfg_sr1100/swc_cfg_node.h)
endif()

target_link_libraries(connection_priority_coordinator.elf
    PRIVATE
        connection_priority_backend
        swc)
add_custom_command(TARGET connection_priority_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary connection_priority_coordinator.elf connection_priority_coordinator.bin
)

target_link_libraries(connection_priority_node.elf
    PRIVATE
        connection_priority_backend
        swc)
target_compile_definitions(connection_priority_node.elf PRIVATE NODE)
add_custom_command(TARGET connection_priority_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary connection_priority_node.elf connection_priority_node.bin
)
