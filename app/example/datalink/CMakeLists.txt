add_library(datalink_backend "")
target_include_directories(datalink_backend PUBLIC facade)
target_link_libraries(datalink_backend PUBLIC common_backend)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(datalink_coordinator.elf "")
    target_sources(datalink_coordinator.elf PRIVATE datalink_coord.c)
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
endif()

add_executable(datalink_node.elf "")
target_sources(datalink_node.elf PRIVATE datalink_node.c)
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
