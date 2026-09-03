cmake_minimum_required(VERSION 3.31)
message(STATUS "Configuring the IDP Master CMakeLists.txt")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to RELEASE_MODE ON so that public release can work without this flag. SPARK devs should use `RELEASE_MODE OFF` flag to run from source.
option(RELEASE_MODE "Use prebuilt SWC/SR_PHY libraries when available" ON)

if (TRANSCEIVER STREQUAL "SR1100")
    add_compile_definitions(SR1100=1)
    add_compile_definitions(SR1000=0)
else()
    add_compile_definitions(SR1000=1)
    add_compile_definitions(SR1100=0)
endif()

if (NOT BUILD_TESTS)
    if(NOT DEFINED RPC_ENABLED)
        set(RPC_ENABLED false)
    endif()

    include(toolchain/gcc-arm-compiler.cmake)

    # Supply the compiler with system-wide processor definitions for the count of radios associated with a given CMAKE configuration.
    if (MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
        message(STATUS "SWC_RADIO_COUNT = 2")
        add_compile_definitions(SWC_RADIO_COUNT=2)
    else()
        message(STATUS "SWC_RADIO_COUNT = 1")
        add_compile_definitions(SWC_RADIO_COUNT=1)
    endif()

    # Physical transceiver bound to the single-radio slot (1 or 2). Default 1.
    # Only meaningful for a single-radio build; ignored when SWC_RADIO_COUNT == 2.
    # Set to 2 to run single-radio firmware electrically on radio 2 (board bring-up /
    # isolating a radio-2-line x supply interaction). Requires RADIO_INTERFACE_QSPI=OFF.
    if (NOT DEFINED SWC_SINGLE_RADIO_ID)
        set(SWC_SINGLE_RADIO_ID 1)
    endif()
    message("SWC_SINGLE_RADIO_ID = ${SWC_SINGLE_RADIO_ID}")
    add_compile_definitions(SWC_SINGLE_RADIO_ID=${SWC_SINGLE_RADIO_ID})

    # Force the app into a fixed certification mode at boot, without the USER_2 button
    # (for ST-Link-only cert benches). When defined, facade_*_get_certification_mode()
    # returns this enum value directly, bypassing the button selection:
    #   1=96k24  2=48k24  3=48k16  4=ADPCM  5=DATA  (0=NONE i.e. normal, never forced).
    # Leave undefined for normal/shipping builds -> button selection is unchanged.
    if (DEFINED CERTIF_FORCE_MODE)
        message("CERTIF_FORCE_MODE = ${CERTIF_FORCE_MODE}")
        add_compile_definitions(CERTIF_FORCE_MODE=${CERTIF_FORCE_MODE})
    endif()

    if (RTOS_ENABLED STREQUAL "ON")
        add_compile_definitions(RTOS_ENABLED=1)
    else()
        add_compile_definitions(RTOS_ENABLED=0)
    endif()

    if (USB_AUDIO_ENABLED)
        add_compile_definitions(USB_AUDIO_ENABLED=1)
    else()
        add_compile_definitions(USB_AUDIO_ENABLED=0)
    endif()

    # Board name as a string, for anything that identifies itself to a human -- boot banners,
    # log prefixes. QUASAR_U535 exists but is a PUBLIC define on the hardware target, and the
    # chain from there to the application targets is broken by a PRIVATE link somewhere, so an
    # application asking #if defined(QUASAR_U535) silently gets the wrong answer. That is worse
    # than having nothing: a banner naming the wrong board is read as proof of what was
    # flashed, and the u5a5-coordinator / u535-node pairing is a normal bench setup.
    if (MCU_VARIANT STREQUAL U535)
        add_compile_definitions(BOARD_NAME="u535")
    else()
        add_compile_definitions(BOARD_NAME="u5a5")
    endif()

    # Which radio(s) this binary drives, for the boot banner. A board can be built three ways
    # -- radio 1 only, radio 2 only, or both -- and the three are indistinguishable once
    # flashed. That matters when they are being compared against each other to find out which
    # RF chain is healthy: flashing the wrong one does not fail, it quietly answers a different
    # question than the one being asked.
    if (MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
        if (MULTI_RADIO_FORCE)
            add_compile_definitions(RADIO_TAG="dual-f${MULTI_RADIO_FORCE}")
        else()
            add_compile_definitions(RADIO_TAG="dual")
        endif()
    else()
        add_compile_definitions(RADIO_TAG="r${SWC_SINGLE_RADIO_ID}")
    endif()

    if (DEFINED I2S_MASTER_MODE)
        add_compile_definitions(I2S_MASTER_MODE=${I2S_MASTER_MODE})
    endif()

    if (DEFINED I2S_FMT_DEFAULT)
        add_compile_definitions(I2S_FMT_DEFAULT=${I2S_FMT_DEFAULT})
    endif()

    # Board pin/behavior overrides (orthogonal, self-documenting).
    if (DEFINED LED_USER1_PD7)
        add_compile_definitions(LED_USER1_PD7=1)
    endif()

    if (DEFINED LED_RGB_BLUE_PA1)
        add_compile_definitions(LED_RGB_BLUE_PA1=1)
    endif()

    if (DEFINED BYPASS_BOARD_REV_CHECK)
        add_compile_definitions(BYPASS_BOARD_REV_CHECK=1)
    endif()

    if (DEFINED SINE_DEBUG_CAPTURE)
        add_compile_definitions(SINE_DEBUG_CAPTURE=1)
    endif()

    if (DEFINED NO_CODEC)
        add_compile_definitions(NO_CODEC=1)
    endif()

    # Send the console to the USB CDC port instead of the expansion UART. Not a default in
    # either direction: the backend picks one per board -- ON for u5a5, OFF for u535 -- and
    # this overrides that choice. Asking for it on the u535 is refused there, not honoured.
    #
    # For boards where the expansion pads are not reachable and the ST-Link VCP is not
    # populated -- u5a5 in particular, where the console otherwise has nowhere to go and the
    # board can only be judged by ear and by LED.
    #
    # The USB device stack is already up on every baremetal build: common_backend.c calls
    # tinyusb_baremetal_setup() with no USB_AUDIO_ENABLED guard, which is how the
    # puretone_headset line already prints on u5a5. So this routes output rather than
    # bringing anything new up.
    #
    # Only the unidirectional app implements it. A build that asks for it elsewhere is
    # refused rather than quietly ignored -- a binary that accepted the flag and printed
    # nowhere would look exactly like a board that never started.
    if (DEFINED CONSOLE_ON_CDC)
        if (NOT APP STREQUAL "Puretone-Unidirectional")
            message(FATAL_ERROR "CONSOLE_ON_CDC is only implemented for APP=Puretone-Unidirectional (got '${APP}')")
        endif()
        message("CONSOLE_ON_CDC = ${CONSOLE_ON_CDC}")
        add_compile_definitions(CONSOLE_ON_CDC=${CONSOLE_ON_CDC})
    endif()

    # Radio-tuning knobs whose headers guard them with #ifndef, so that a comparison arm is a
    # -D on the configure line instead of an edit that has to be remembered and undone. Both
    # take their value through rather than being turned into a flag: NODE_ISI_MITIG is an
    # enumerator, and the CCA count is a number.
    if (DEFINED NODE_ISI_MITIG)
        add_compile_definitions(NODE_ISI_MITIG=${NODE_ISI_MITIG})
    endif()

    if (DEFINED MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT)
        add_compile_definitions(MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT=${MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT})
    endif()

    if (DEFINED MULTI_RADIO_FORCE)
        add_compile_definitions(MULTI_RADIO_FORCE=${MULTI_RADIO_FORCE})
    endif()

    # Both of these have #ifndef defaults in headers, so the shipped value works whether or
    # not they are forwarded -- which is exactly why the omission was invisible. Only the -D
    # override was broken, and both are documented as the way to answer a specific question:
    # ALLOW_96K=1 to retest the 96 kHz drop, PULSE_WIDTH_MAX=0 to back out the level-4 width
    # if near-field regresses. An escape hatch that silently does nothing is worse than none.
    if (DEFINED MAIN_CHANNEL_ALLOW_96K)
        add_compile_definitions(MAIN_CHANNEL_ALLOW_96K=${MAIN_CHANNEL_ALLOW_96K})
    endif()

    if (DEFINED TX_PULSE_WIDTH_MAX)
        add_compile_definitions(TX_PULSE_WIDTH_MAX=${TX_PULSE_WIDTH_MAX})
    endif()

    if (DEFINED FALLBACK_PIN_AT_BOTTOM)
        add_compile_definitions(FALLBACK_PIN_AT_BOTTOM=${FALLBACK_PIN_AT_BOTTOM})
    endif()

    # Release suffix stamped into every version the firmware reports -- the boot banner,
    # AT+VER, AT+FW_VERSION and +MODULE_INFO all come from it. fw_version.h has documented
    # this as overridable per build since it was written, but nothing ever forwarded it, so
    # the -D was accepted and silently ignored: the same omission recorded above for
    # MAIN_CHANNEL_ALLOW_96K, and invisible for the same reason -- the header's #ifndef
    # default keeps producing a correct-looking build.
    #
    # Pass it WITHOUT the quotes; they are added here so the value reaches the compiler as a
    # C string literal. -DFW_VERSION_RELEASE=_rc2_r1 gives "v2.4.1_rc2_r1".
    if (DEFINED FW_VERSION_RELEASE)
        message("FW_VERSION_RELEASE = ${FW_VERSION_RELEASE}")
        add_compile_definitions(FW_VERSION_RELEASE="${FW_VERSION_RELEASE}")
    endif()

    if (DEFINED STATS_VERBOSE)
        add_compile_definitions(STATS_VERBOSE=${STATS_VERBOSE})
    endif()

    if (DEFINED FBK4_TX_POWER_REF)
        add_compile_definitions(FBK4_TX_POWER_REF=${FBK4_TX_POWER_REF})
    endif()

    if (DEFINED RADIO_USE_SAVED_CALIB)
        add_compile_definitions(RADIO_USE_SAVED_CALIB=${RADIO_USE_SAVED_CALIB})
    endif()

    if (DEFINED ACK_TX_POWER_MAX)
        add_compile_definitions(ACK_TX_POWER_MAX=${ACK_TX_POWER_MAX})
    endif()

    if (DEFINED SINE_INJECT_DG)
        add_compile_definitions(SINE_INJECT_DG=1)
    endif()

    if (DEFINED SINE_INJECT_HS)
        add_compile_definitions(SINE_INJECT_HS=1)
    endif()

    if (U535_PWR_LDO)
        add_compile_definitions(U535_PWR_LDO=1)
    endif()

    project(IDP CXX C ASM)
    add_subdirectory(library)
    add_subdirectory(third-party)
    add_subdirectory(bsp)
    add_subdirectory(driver)
    add_subdirectory(core)
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/middleware")
        add_subdirectory(middleware)
    endif()
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/module")
        add_subdirectory(module)
    endif()
    add_subdirectory(app)
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/dev_tool")
        add_subdirectory(dev_tool)
    endif()
    add_subdirectory(backend)
    if(${MCU_DRIVER} STREQUAL STMG4XX)
        include(toolchain/gcc-arm-flags-stm32g4.cmake)
    elseif(${MCU_DRIVER} STREQUAL STMU5XX)
        include(toolchain/gcc-arm-flags-stm32u5.cmake)
    endif()
else ()
    add_compile_definitions(SWC_RADIO_COUNT=1)
    project(unit_tests)
    include(toolchain/gcc-x86-compiler.cmake)
    enable_testing()
    set(RTOS_ENABLED false)
    add_subdirectory(tests)
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/module")
        add_subdirectory(module/spark_pairing)
    endif()
    add_subdirectory(backend)
    add_subdirectory(dev_tool/sdk_validator_helper)
endif()
