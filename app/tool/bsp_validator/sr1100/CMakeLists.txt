add_library(bsp_validator_backend "")

target_include_directories(bsp_validator_backend PUBLIC facade)
target_link_libraries(bsp_validator_backend PUBLIC swc)

add_executable(bsp_validator.elf "")
target_sources(bsp_validator.elf
    PRIVATE
        bsp_validator.c
)
target_link_libraries(bsp_validator.elf PRIVATE critical_section bsp_validator_backend)
add_custom_command(TARGET bsp_validator.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary bsp_validator.elf bsp_validator.bin
)

target_include_directories(bsp_validator_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../facade)
