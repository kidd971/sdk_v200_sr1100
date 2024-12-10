add_library(cmsis_device_u5 "")

set_target_properties(cmsis_device_u5 PROPERTIES LINKER_LANGUAGE C)

set(CMSIS_U5_PATH ${CMAKE_CURRENT_LIST_DIR}/src)

target_sources(cmsis_device_u5
    PUBLIC
        ${CMSIS_U5_PATH}/Include/stm32u5xx.h
        ${CMSIS_U5_PATH}/Include/system_stm32u5xx.h
)

target_link_libraries(cmsis_device_u5 PUBLIC cmsis_5)
target_compile_definitions(cmsis_device_u5 PUBLIC ${STM32U5_MCU_MODEL} USE_HAL_DRIVER STM32U5A5xx)
target_include_directories(cmsis_device_u5 PUBLIC ${CMSIS_U5_PATH}/Include)
