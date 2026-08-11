if(BUILD_TESTS)
    message(STATUS "Building SR PHY from sources for unit tests.")

    add_library(sr_phy "")

    target_include_directories(sr_phy
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
    )

    if(RADIO_INTERFACE_QSPI)
        target_compile_definitions(sr_phy PUBLIC RADIO_QSPI_ENABLED=1)
    else()
        target_compile_definitions(sr_phy PUBLIC RADIO_QSPI_ENABLED=0)
    endif()

    target_compile_definitions(sr_phy PUBLIC SR_PHY_RADIO_COUNT=1)

    if(TRANSCEIVER STREQUAL "SR1100")
        add_subdirectory(sr1100)
    else()
        add_subdirectory(sr1000)
    endif()

    target_sources(sr_phy
        PRIVATE
            sr_phy_hal.c
            sr_access.c
            sr_utils.c
            sr_pwr_adjust.c
    )

else()
    if(TRANSCEIVER STREQUAL "SR1000")
        set(SR_PHY_TRANSCEIVER_SUFFIX "sr1000")
        set(SR_PHY_TRANSCEIVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/sr1000")
    elseif(TRANSCEIVER STREQUAL "SR1100")
        set(SR_PHY_TRANSCEIVER_SUFFIX "sr1100")
        set(SR_PHY_TRANSCEIVER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/sr1100")
    else()
        message(FATAL_ERROR "Unsupported TRANSCEIVER value: ${TRANSCEIVER}")
    endif()

    if(MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
        set(SR_PHY_RADIO_SUFFIX "dual")
    else()
        set(SR_PHY_RADIO_SUFFIX "single")
    endif()

    if(MCU_DRIVER STREQUAL "STMG4XX")
        set(SR_PHY_ARCH_SUFFIX "m4")
    elseif(MCU_DRIVER STREQUAL "STMU5XX")
        set(SR_PHY_ARCH_SUFFIX "m33")
    else()
        message(FATAL_ERROR "Unsupported MCU_DRIVER value: ${MCU_DRIVER}")
    endif()

    if(RADIO_INTERFACE_QSPI)
        set(SR_PHY_QSPI_SUFFIX "_qspi")
    else()
        set(SR_PHY_QSPI_SUFFIX "")
    endif()

    set(SR_PHY_LIB_BASENAME
        "sr_phy_${SR_PHY_TRANSCEIVER_SUFFIX}_${SR_PHY_RADIO_SUFFIX}_${SR_PHY_ARCH_SUFFIX}${SR_PHY_QSPI_SUFFIX}_lib"
    )

    set(SR_PHY_LIB_FILENAME "${SR_PHY_LIB_BASENAME}.a")

    message(STATUS "SR PHY library basename: ${SR_PHY_LIB_BASENAME}")
    message(STATUS "SR PHY library filename: ${SR_PHY_LIB_FILENAME}")

    set(SR_PHY_PREBUILT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt")
    set(SR_PHY_PREBUILT_LIB_PATH "${SR_PHY_PREBUILT_DIR}/${SR_PHY_LIB_FILENAME}")

    message(STATUS "SR PHY prebuilt directory: ${SR_PHY_PREBUILT_DIR}")
    message(STATUS "SR PHY prebuilt library path: ${SR_PHY_PREBUILT_LIB_PATH}")

    if(EXISTS "${SR_PHY_PREBUILT_LIB_PATH}" AND RELEASE_MODE)
        message(STATUS "Using prebuilt SR PHY library: ${SR_PHY_PREBUILT_LIB_PATH}")

        add_library(sr_phy STATIC IMPORTED GLOBAL)

        set_target_properties(sr_phy PROPERTIES
            IMPORTED_LOCATION "${SR_PHY_PREBUILT_LIB_PATH}"
        )

        set(SR_PHY_INCLUDE_SCOPE INTERFACE)
        set(SR_PHY_DEFINITION_SCOPE INTERFACE)
        set(SR_PHY_BUILD_FROM_SOURCE OFF)

    else()
        message(STATUS "Prebuilt SR PHY library not found. Building SR PHY from sources.")

        add_library(sr_phy "")

        set_target_properties(sr_phy PROPERTIES
            OUTPUT_NAME ${SR_PHY_LIB_BASENAME}
            PREFIX ""
        )

        set(SR_PHY_INCLUDE_SCOPE PUBLIC)
        set(SR_PHY_DEFINITION_SCOPE PUBLIC)
        set(SR_PHY_BUILD_FROM_SOURCE ON)
    endif()

    target_include_directories(sr_phy
        ${SR_PHY_INCLUDE_SCOPE}
            ${CMAKE_CURRENT_LIST_DIR}
            ${SR_PHY_TRANSCEIVER_INCLUDE_DIR}
    )

    if(RADIO_INTERFACE_QSPI)
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} RADIO_QSPI_ENABLED=1)
    else()
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} RADIO_QSPI_ENABLED=0)
    endif()

    if(MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
        message(STATUS "SR_PHY_RADIO_COUNT = 2")
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} SR_PHY_RADIO_COUNT=2)
    else()
        message(STATUS "SR_PHY_RADIO_COUNT = 1")
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} SR_PHY_RADIO_COUNT=1)
    endif()

    if(TRANSCEIVER STREQUAL "SR1100")
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} SR1100=1 SR1000=0)
    else()
        target_compile_definitions(sr_phy ${SR_PHY_DEFINITION_SCOPE} SR1000=1 SR1100=0)
    endif()

    if(SR_PHY_BUILD_FROM_SOURCE)
        if(TRANSCEIVER STREQUAL "SR1100")
            add_subdirectory(sr1100)
        else()
            add_subdirectory(sr1000)
        endif()

        target_sources(sr_phy
            PRIVATE
                sr_phy_hal.c
                sr_access.c
                sr_utils.c
                sr_pwr_adjust.c
        )

        file(MAKE_DIRECTORY "${SR_PHY_PREBUILT_DIR}")

        add_custom_command(TARGET sr_phy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:sr_phy>
                "${SR_PHY_PREBUILT_LIB_PATH}"
            COMMENT "Copying SR PHY library to prebuilt directory: ${SR_PHY_PREBUILT_LIB_PATH}"
        )
    endif()
endif()
