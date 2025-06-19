add_library(fragmentation_backend "")
target_include_directories(fragmentation_backend PUBLIC facade)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
else()
    add_subdirectory(swc_cfg_sr1100)
endif()

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(fragmentation_coordinator.elf "")
    target_sources(fragmentation_coordinator.elf PRIVATE frag_coord.c)
    target_link_libraries(fragmentation_coordinator.elf
        PRIVATE
            swc
            fragmentation_backend
            pseudo_data
    )
    add_custom_command(TARGET fragmentation_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary fragmentation_coordinator.elf fragmentation_coordinator.bin
    )
endif()

add_executable(fragmentation_node.elf "")
target_sources(fragmentation_node.elf PRIVATE frag_node.c)
target_link_libraries(fragmentation_node.elf
    PRIVATE
        swc
        fragmentation_backend
        pseudo_data
)
add_custom_command(TARGET fragmentation_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary fragmentation_node.elf fragmentation_node.bin
)
