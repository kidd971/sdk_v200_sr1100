cmake_minimum_required(VERSION 3.18)

# Assert CMake build variable for the extension API
if (NOT DEFINED SWC_EXT_API_ENABLE)
    set(SWC_EXT_API_ENABLE false)
endif()

set(TRANSCEIVER "SR1000" CACHE STRING "Set SR1000 transceiver model has default if the variable wasn't initialized.")

if (TRANSCEIVER STREQUAL "SR1000" AND RADIO_INTERFACE_QSPI)
    message(FATAL_ERROR "QSPI interface is not supported on SR1000 transceiver.")
endif()

option(WPS_INCLUDE_DEFAULT_CONFIG "Whether the location of the default config header should be in the include directories" ON)

# Statistics options
option(WPS_ENABLE_PHY_STATS "Whether PHY statistics should be compiled and usable." ON)
option(WPS_ENABLE_STATS_USED_TIMESLOTS "Whether used frames statistics should be compiled and usable." ON)

include(CMakeDependentOption)
cmake_dependent_option(WPS_ENABLE_PHY_STATS_PER_BANDS "Enable per channel statistics" OFF "WPS_ENABLE_PHY_STATS" OFF)

option(WPS_ENABLE_LINK_STATS "Whether link statistics should be compiled and usable." ON)
option(WPS_DISABLE_FRAGMENTATION "Whether fragmentation should be compiled and usable." OFF)
option(WPS_DISABLE_LINK_THROTTLE "Whether link throttle should be compiled and usable." OFF)

if(BUILD_TESTS)
    add_library(swc "")

    set(SWC_LINK_SCOPE PUBLIC)
    set(SWC_INCLUDE_SCOPE PUBLIC)
    set(SWC_DEFINITION_SCOPE PUBLIC)
    set(SWC_BUILD_FROM_SOURCE ON)

else()
    if(TRANSCEIVER STREQUAL "SR1000")
        set(SWC_TRANSCEIVER_SUFFIX "sr1000")
        set(SWC_API_TRANSCEIVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/api/swc/sr1000")
        if (SWC_EXT_API_ENABLE)
            if(RELEASE_MODE)
                message(FATAL_ERROR "SWC extension API is internal and must not be built in RELEASE_MODE.")
            endif()
            add_subdirectory(api/swc/sr1000/extension)
        endif()
    elseif(TRANSCEIVER STREQUAL "SR1100")
        set(SWC_TRANSCEIVER_SUFFIX "sr1100")
        set(SWC_API_TRANSCEIVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/api/swc/sr1100")
        if (SWC_EXT_API_ENABLE)
            if(RELEASE_MODE)
                message(FATAL_ERROR "SWC extension API is internal and must not be built in RELEASE_MODE.")
            endif()
            add_subdirectory(api/swc/sr1100/extension)
        endif()
    else()
        message(FATAL_ERROR "Unsupported TRANSCEIVER value: ${TRANSCEIVER}")
    endif()

    if(MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
        set(SWC_RADIO_SUFFIX "dual")
    else()
        set(SWC_RADIO_SUFFIX "single")
    endif()

    if(MCU_DRIVER STREQUAL "STMG4XX")
        set(SWC_ARCH_SUFFIX "m4")
    elseif(MCU_DRIVER STREQUAL "STMU5XX")
        set(SWC_ARCH_SUFFIX "m33")
    else()
        message(FATAL_ERROR "Unsupported MCU_DRIVER value: ${MCU_DRIVER}")
    endif()

    if(WPS_ENABLE_PHY_STATS_PER_BANDS)
        set(SWC_STATS_SUFFIX "_stats_band")
    else()
        set(SWC_STATS_SUFFIX "")
    endif()

    if(RADIO_INTERFACE_QSPI)
        set(SWC_QSPI_SUFFIX "_qspi")
    else()
        set(SWC_QSPI_SUFFIX "")
    endif()

    set(SWC_LIB_BASENAME
        "swc_api_${SWC_TRANSCEIVER_SUFFIX}_${SWC_RADIO_SUFFIX}_${SWC_ARCH_SUFFIX}${SWC_STATS_SUFFIX}${SWC_QSPI_SUFFIX}_lib"
    )

    set(SWC_LIB_FILENAME "${SWC_LIB_BASENAME}.a")

    message(STATUS "SWC library basename: ${SWC_LIB_BASENAME}")
    message(STATUS "SWC library filename: ${SWC_LIB_FILENAME}")

    set(SWC_PREBUILT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt")
    set(SWC_PREBUILT_LIB_PATH "${SWC_PREBUILT_DIR}/${SWC_LIB_FILENAME}")

    message(STATUS "SWC prebuilt directory: ${SWC_PREBUILT_DIR}")
    message(STATUS "SWC prebuilt library path: ${SWC_PREBUILT_LIB_PATH}")

    if(EXISTS "${SWC_PREBUILT_LIB_PATH}" AND RELEASE_MODE)
        message(STATUS "Using prebuilt SWC library: ${SWC_PREBUILT_LIB_PATH}")

        add_library(swc STATIC IMPORTED GLOBAL)
        set_target_properties(swc PROPERTIES
            IMPORTED_LOCATION "${SWC_PREBUILT_LIB_PATH}"
        )

        set(SWC_LINK_SCOPE INTERFACE)
        set(SWC_INCLUDE_SCOPE INTERFACE)
        set(SWC_DEFINITION_SCOPE INTERFACE)
        set(SWC_BUILD_FROM_SOURCE OFF)
    else()
        message(STATUS "Prebuilt SWC library not found. Building SWC from sources.")

        add_library(swc "")

        set_target_properties(swc PROPERTIES
            OUTPUT_NAME ${SWC_LIB_BASENAME}
            PREFIX ""
        )

        set(SWC_LINK_SCOPE PUBLIC)
        set(SWC_INCLUDE_SCOPE PUBLIC)
        set(SWC_DEFINITION_SCOPE PUBLIC)
        set(SWC_BUILD_FROM_SOURCE ON)
    endif()
endif()

target_include_directories(swc
    ${SWC_INCLUDE_SCOPE}
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/api/swc
    )

if(DEFINED SWC_API_TRANSCEIVER_INCLUDE_DIR)
    target_include_directories(swc
        ${SWC_INCLUDE_SCOPE}
            ${SWC_API_TRANSCEIVER_INCLUDE_DIR}
    )
endif()

if(WPS_INCLUDE_DEFAULT_CONFIG)
    target_include_directories(swc
        ${SWC_INCLUDE_SCOPE}
            ${CMAKE_CURRENT_LIST_DIR}/cfg
    )
endif()

add_subdirectory(facade)

target_link_libraries(swc
    ${SWC_LINK_SCOPE}
        swc_facade
        sr_phy
        buffer
        memory
        queue
        critical_section
        crc8
)

if(WPS_ENABLE_PHY_STATS)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_PHY_STATS=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_PHY_STATS=0)
endif()

if(WPS_ENABLE_STATS_USED_TIMESLOTS)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_STATS_USED_TIMESLOTS=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_STATS_USED_TIMESLOTS=0)
endif()

if(WPS_ENABLE_PHY_STATS_PER_BANDS)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_PHY_STATS_PER_BANDS=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_PHY_STATS_PER_BANDS=0)
endif()

if(WPS_ENABLE_LINK_STATS)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_LINK_STATS=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_ENABLE_LINK_STATS=0)
endif()

if(WPS_DISABLE_FRAGMENTATION)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_DISABLE_FRAGMENTATION=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_DISABLE_FRAGMENTATION=0)
endif()

if(WPS_DISABLE_LINK_THROTTLE)
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_DISABLE_LINK_THROTTLE=1)
else()
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_DISABLE_LINK_THROTTLE=0)
endif()

if(MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    message(STATUS "WPS_RADIO_COUNT = 2")
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_RADIO_COUNT=2)
else()
    message(STATUS "WPS_RADIO_COUNT = 1")
    target_compile_definitions(swc ${SWC_DEFINITION_SCOPE} WPS_RADIO_COUNT=1)
endif()

target_compile_definitions(swc
    ${SWC_DEFINITION_SCOPE}
        ${ACCESS_ADV_ERR_CHECK_EN}
        ${TRANSCEIVER}
)

if(SWC_BUILD_FROM_SOURCE)
    add_subdirectory(api/swc)
    if(WPS_INCLUDE_DEFAULT_CONFIG)
        add_subdirectory(cfg)
    endif()
    add_subdirectory(link)
    add_subdirectory(protocol_stack)
    add_subdirectory(xlayer)
    if(NOT BUILD_TESTS)
        file(MAKE_DIRECTORY "${SWC_PREBUILT_DIR}")

        add_custom_command(TARGET swc POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:swc>
                "${SWC_PREBUILT_LIB_PATH}"
            COMMENT "Copying SWC library to prebuilt directory: ${SWC_PREBUILT_LIB_PATH}"
        )
    endif()
endif()
