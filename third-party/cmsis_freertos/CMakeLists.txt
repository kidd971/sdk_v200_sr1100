if(${MCU_DRIVER} STREQUAL STMG4XX)
    set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
elseif(${MCU_DRIVER} STREQUAL STMU5XX)
    set(FREERTOS_PORT GCC_ARM_CM33_NTZ_NONSECURE CACHE STRING "")
endif()

set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/freertos_app_config/default CACHE STRING "")
set(CMSIS_FREE_RTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS/RTOS2/FreeRTOS CACHE STRING "")
set(CMSIS_FREERTOS_SOURCE_DIRECTORY ${CMSIS_FREE_RTOS_PATH}/Source CACHE STRING "")
set(CMSIS_FREERTOS_INCLUDE_DIRECTORY ${CMSIS_FREE_RTOS_PATH}/Include CACHE STRING "")
set(FREERTOS_TUSB_INC_DIR_PATH ${CMAKE_CURRENT_LIST_DIR}/src/Source/include/ CACHE STRING "")

add_subdirectory(src/Source)

# Suppress the unused variable warning for this target
target_compile_options(freertos_kernel PRIVATE -Wno-unused-variable)
target_compile_options(freertos_kernel_port PRIVATE -Wno-unused-but-set-variable)
