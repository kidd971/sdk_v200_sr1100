add_library(cmsis_5 "")

set_target_properties(cmsis_5 PROPERTIES LINKER_LANGUAGE C)

set(CMSIS_5_PATH ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS)

target_sources(cmsis_5
    PRIVATE
        ${CMSIS_5_PATH}/DSP/Source/FilteringFunctions/arm_fir_decimate_q15.c
        ${CMSIS_5_PATH}/DSP/Source/FilteringFunctions/arm_fir_decimate_init_q15.c
        ${CMSIS_5_PATH}/DSP/Source/FilteringFunctions/arm_fir_interpolate_q15.c
        ${CMSIS_5_PATH}/DSP/Source/FilteringFunctions/arm_fir_interpolate_init_q15.c
    PUBLIC
        ${CMSIS_5_PATH}/Core/Include/core_cm4.h
        ${CMSIS_5_PATH}/Core/Include/cmsis_version.h
        ${CMSIS_5_PATH}/Core/Include/cmsis_gcc.h
        ${CMSIS_5_PATH}/DSP/Include/arm_math.h
)

target_include_directories(cmsis_5 PUBLIC ${CMSIS_5_PATH}/Core/Include ${CMSIS_5_PATH}/DSP/Include)

if (RTOS_ENABLED)
    add_library(cmsis_rtos_v2 "")
    set_target_properties(cmsis_rtos_v2 PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(cmsis_rtos_v2 hardware cmsis_5 freertos_kernel)
    if(${MCU_DRIVER} STREQUAL STMG4XX)
        target_link_libraries(cmsis_rtos_v2 cmsis_device_g4)
        target_compile_definitions(cmsis_rtos_v2 PUBLIC ${STM32G4_MCU_MODEL})
    elseif(${MCU_DRIVER} STREQUAL STMU5XX)
        target_link_libraries(cmsis_rtos_v2 cmsis_device_u5)
        target_compile_definitions(cmsis_rtos_v2 PUBLIC ${STM32U5_MCU_MODEL})
    endif()

    set(CMSIS_RTOS2_PATH ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS/RTOS2/Include)
    target_include_directories(cmsis_rtos_v2
        PUBLIC
            ${CMSIS_RTOS2_PATH}
            ${CMSIS_FREERTOS_INCLUDE_DIRECTORY}
        INTERFACE
            ${FREERTOS_CONFIG_FILE_DIRECTORY}

    )
    target_sources(cmsis_rtos_v2
        PRIVATE
            ${CMSIS_FREERTOS_SOURCE_DIRECTORY}/cmsis_os2.c
    )
endif()

