add_library(profiler_backend "")
target_include_directories(profiler_backend PUBLIC facade)
target_link_libraries(profiler_backend PUBLIC common_backend)

add_subdirectory(config)

# DUT executable
add_executable(profiler_dut.elf "")
target_sources(profiler_dut.elf PRIVATE profiler_dut.c)
target_link_libraries(profiler_dut.elf
    PRIVATE
        profiler_backend
        swc
)
add_custom_command(TARGET profiler_dut.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary profiler_dut.elf profiler_dut.bin
)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # Helper executable
    add_executable(profiler_helper.elf "")
    target_sources(profiler_helper.elf PRIVATE profiler_helper.c)

    target_link_libraries(profiler_helper.elf
        PRIVATE
            profiler_backend
            swc
    )
    add_custom_command(TARGET profiler_helper.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary profiler_helper.elf profiler_helper.bin
    )
endif()
