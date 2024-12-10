add_library(star_network_backend "")
target_include_directories(star_network_backend PUBLIC facade)

add_executable(star_network_coordinator.elf "")
target_sources(star_network_coordinator.elf PRIVATE star_network_coord.c)

add_executable(star_network_node1.elf "")
target_sources(star_network_node1.elf PRIVATE star_network_node.c)

add_executable(star_network_node2.elf "")
target_sources(star_network_node2.elf PRIVATE star_network_node.c)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(swc_cfg_sr1000)
    target_sources(star_network_coordinator.elf PRIVATE swc_cfg_sr1000/swc_cfg_coord.h)
    target_sources(star_network_node1.elf PRIVATE swc_cfg_sr1000/swc_cfg_node1.h)
    target_sources(star_network_node2.elf PRIVATE swc_cfg_sr1000/swc_cfg_node2.h)
else()
    add_subdirectory(swc_cfg_sr1100)
    target_sources(star_network_coordinator.elf PRIVATE swc_cfg_sr1100/swc_cfg_coord.h)
    target_sources(star_network_node1.elf PRIVATE swc_cfg_sr1100/swc_cfg_node1.h)
    target_sources(star_network_node2.elf PRIVATE swc_cfg_sr1100/swc_cfg_node2.h)
endif()

target_link_libraries(star_network_coordinator.elf
    PRIVATE
        star_network_backend
        swc)
add_custom_command(TARGET star_network_coordinator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary star_network_coordinator.elf star_network_coordinator.bin
)

target_link_libraries(star_network_node1.elf
    PRIVATE
        star_network_backend
        swc)
target_compile_definitions(star_network_node1.elf PRIVATE NODE1)
add_custom_command(TARGET star_network_node1.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary star_network_node1.elf star_network_node1.bin
)

target_link_libraries(star_network_node2.elf
    PRIVATE
        star_network_backend
        swc)
target_compile_definitions(star_network_node2.elf PRIVATE NODE2)
add_custom_command(TARGET star_network_node2.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary star_network_node2.elf star_network_node2.bin
)
