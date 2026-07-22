if (TRANSCEIVER STREQUAL "SR1000")
    message("PureTone application is SR1100 only.")
    return()
endif()

add_library(puretone_headset_backend "")
target_include_directories(puretone_headset_backend PUBLIC facade common_backend)
target_link_libraries(puretone_headset_backend PUBLIC common_backend)

# Link config files to backend
target_include_directories(puretone_headset_backend PUBLIC config)
target_link_libraries(puretone_headset_backend PRIVATE audio_core)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(puretone_dongle.elf "")
    target_sources(puretone_dongle.elf PRIVATE puretone_dongle.c reconnect_store.c)
    target_link_libraries(puretone_dongle.elf
        PRIVATE
            audio_core
            puretone_headset_backend
            swc
            at_cmd_core
    )
    add_custom_command(TARGET puretone_dongle.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary puretone_dongle.elf puretone_dongle.bin
    )
endif()
add_executable(puretone_headset.elf "")
target_sources(puretone_headset.elf PRIVATE puretone_headset.c reconnect_store.c)
target_link_libraries(puretone_headset.elf
    PRIVATE
        audio_core
        puretone_headset_backend
        swc
        at_cmd_core
)
add_custom_command(TARGET puretone_headset.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary puretone_headset.elf puretone_headset.bin
)
