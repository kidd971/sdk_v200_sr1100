if (NOT TRANSCEIVER STREQUAL "SR1100")
    return()
endif()

add_library(puretone_unidirectional_backend "")
target_include_directories(puretone_unidirectional_backend PUBLIC facade common_backend)
target_link_libraries(puretone_unidirectional_backend PUBLIC common_backend)

# Link config files to backend
target_include_directories(puretone_unidirectional_backend PUBLIC config)
target_link_libraries(puretone_unidirectional_backend PRIVATE audio_core)

add_subdirectory(config)

if (NOT MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # The coordinator does not support multi radio
    add_executable(puretone_unidirectional_coordinator.elf "")
    target_sources(puretone_unidirectional_coordinator.elf PRIVATE puretone_unidirectional_coord.c)
    target_link_libraries(puretone_unidirectional_coordinator.elf
        PRIVATE
            fw_version
            link_data
            reconnect_store
            audio_core
            puretone_unidirectional_backend
            swc
            spark_pairing
    )
    add_custom_command(TARGET puretone_unidirectional_coordinator.elf
        POST_BUILD
        COMMAND arm-none-eabi-objcopy -O binary puretone_unidirectional_coordinator.elf puretone_unidirectional_coordinator.bin
    )
endif()

add_executable(puretone_unidirectional_node.elf "")
target_sources(puretone_unidirectional_node.elf PRIVATE puretone_unidirectional_node.c)
target_link_libraries(puretone_unidirectional_node.elf
    PRIVATE
        fw_version
        link_data
        reconnect_store
        audio_core
        puretone_unidirectional_backend
        swc
        spark_pairing
)
add_custom_command(TARGET puretone_unidirectional_node.elf
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary puretone_unidirectional_node.elf puretone_unidirectional_node.bin
)
