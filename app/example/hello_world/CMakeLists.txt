add_library(hello_world_backend "")
target_include_directories(hello_world_backend PUBLIC facade)

add_executable(hello_world_coordinator.elf "")
target_sources(hello_world_coordinator.elf PRIVATE hello_world_coord.c)

add_executable(hello_world_node.elf "")
target_sources(hello_world_node.elf PRIVATE hello_world_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(hello_world_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(hello_world_node.elf PRIVATE swc_cfg_sr1000/swc_cfg_node.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(hello_world_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(hello_world_node.elf PRIVATE swc_cfg_sr1100/swc_cfg_node.h)
endif()

target_link_libraries(hello_world_coordinator.elf
    PRIVATE
        hello_world_backend
	    swc)
add_custom_command(TARGET hello_world_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary hello_world_coordinator.elf hello_world_coordinator.bin
)

target_link_libraries(hello_world_node.elf
    PRIVATE
        hello_world_backend
	    swc)
add_custom_command(TARGET hello_world_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary hello_world_node.elf hello_world_node.bin
)
