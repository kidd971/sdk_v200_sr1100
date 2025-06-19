add_library(hello_world_rtos_backend "")
target_include_directories(hello_world_rtos_backend PUBLIC facade)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
else()
    add_subdirectory(swc_cfg_sr1100)
endif()

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(hello_world_rtos_coordinator.elf "")
    target_sources(hello_world_rtos_coordinator.elf PRIVATE hello_world_rtos_coord.c)
    target_link_libraries(hello_world_rtos_coordinator.elf
        PUBLIC
            hello_world_rtos_backend
            swc
            cmsis_rtos_v2
            module_tinyusb
    )
    add_custom_command(TARGET hello_world_rtos_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary hello_world_rtos_coordinator.elf hello_world_rtos_coordinator.bin
    )
endif()

add_executable(hello_world_rtos_node.elf "")
target_sources(hello_world_rtos_node.elf PRIVATE hello_world_rtos_node.c)
target_link_libraries(hello_world_rtos_node.elf
    PUBLIC
        hello_world_rtos_backend
        swc
        cmsis_rtos_v2
        module_tinyusb
)
add_custom_command(TARGET hello_world_rtos_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary hello_world_rtos_node.elf hello_world_rtos_node.bin
)
