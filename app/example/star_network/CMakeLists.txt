add_library(star_network_backend "")
target_include_directories(star_network_backend PUBLIC facade)
target_link_libraries(star_network_backend PUBLIC common_backend)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(star_network_coordinator.elf "")
    target_sources(star_network_coordinator.elf PRIVATE star_network_coord.c)
    target_link_libraries(star_network_coordinator.elf
        PRIVATE
            star_network_backend
            swc
    )
    add_custom_command(TARGET star_network_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary star_network_coordinator.elf star_network_coordinator.bin
    )
endif()

add_executable(star_network_node1.elf "")
target_sources(star_network_node1.elf PRIVATE star_network_node.c)
target_link_libraries(star_network_node1.elf
    PRIVATE
        star_network_backend
        swc
)
target_compile_definitions(star_network_node1.elf PRIVATE NODE1)
add_custom_command(TARGET star_network_node1.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary star_network_node1.elf star_network_node1.bin
)

add_executable(star_network_node2.elf "")
target_sources(star_network_node2.elf PRIVATE star_network_node.c)
target_link_libraries(star_network_node2.elf
    PRIVATE
        star_network_backend
        common_backend
        swc
)
target_compile_definitions(star_network_node2.elf PRIVATE NODE2)
add_custom_command(TARGET star_network_node2.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary star_network_node2.elf star_network_node2.bin
)
