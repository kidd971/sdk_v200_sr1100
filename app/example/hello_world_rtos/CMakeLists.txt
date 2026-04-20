add_library(hello_world_rtos_backend "")
target_include_directories(hello_world_rtos_backend PUBLIC facade)
target_link_libraries(hello_world_rtos_backend PUBLIC common_backend)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(hello_world_rtos_coordinator.elf "")
    target_sources(hello_world_rtos_coordinator.elf PRIVATE hello_world_rtos_coord.c)
    target_link_libraries(hello_world_rtos_coordinator.elf
        PUBLIC
            hello_world_rtos_backend
            swc
            cmsis_rtos_v2
            module_tinyusb_freertos
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
        module_tinyusb_freertos
)
add_custom_command(TARGET hello_world_rtos_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary hello_world_rtos_node.elf hello_world_rtos_node.bin
)
