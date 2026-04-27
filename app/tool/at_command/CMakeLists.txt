# at_command_test: integration test using profiler_backend for hardware access
add_executable(at_command_test.elf at_command_test.c)
target_link_libraries(at_command_test.elf
    PRIVATE
        at_module
        profiler_backend
        swc
)
add_custom_command(TARGET at_command_test.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary at_command_test.elf at_command_test.bin
)
