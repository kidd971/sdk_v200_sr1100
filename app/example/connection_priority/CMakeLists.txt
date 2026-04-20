add_library(connection_priority_backend "")
target_include_directories(connection_priority_backend PUBLIC facade)
target_link_libraries(connection_priority_backend PUBLIC common_backend)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(connection_priority_coordinator.elf "")
    target_sources(connection_priority_coordinator.elf PRIVATE connection_priority_coord.c)
    target_link_libraries(connection_priority_coordinator.elf
        PRIVATE
            connection_priority_backend
            swc
    )
    add_custom_command(TARGET connection_priority_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary connection_priority_coordinator.elf connection_priority_coordinator.bin
    )
endif()

add_executable(connection_priority_node.elf "")
target_sources(connection_priority_node.elf PRIVATE connection_priority_node.c)
target_link_libraries(connection_priority_node.elf
    PRIVATE
        connection_priority_backend
        swc
)
target_compile_definitions(connection_priority_node.elf PRIVATE NODE)
add_custom_command(TARGET connection_priority_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary connection_priority_node.elf connection_priority_node.bin
)
