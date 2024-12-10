cmake_minimum_required(VERSION 3.18)

# Assert CMake build variable for the extension API
if (NOT DEFINED SWC_EXT_API_ENABLE)
    set(SWC_EXT_API_ENABLE false)
endif()

add_library(swc "")
target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(swc
    PUBLIC
        buffer
        memory
        queue
        critical_section
)

set(TRANSCEIVER "SR1000" CACHE STRING "Set SR1000 transceiver model has default if the variable wasn't initialized.")

option(WPS_INCLUDE_DEFAULT_CONFIG "Whether the location of the default config header should be in the include directories" ON)

# Statistics options
option(WPS_ENABLE_PHY_STATS "Whether PHY statistics should be compiled and usable." ON)
if(WPS_ENABLE_PHY_STATS)
    target_compile_definitions(swc PUBLIC WPS_ENABLE_PHY_STATS=1)
else()
    target_compile_definitions(swc PUBLIC WPS_ENABLE_PHY_STATS=0)
endif()

option(WPS_ENABLE_STATS_USED_TIMESLOTS "Whether used frames statistics should be compiled and usable." ON)
if(WPS_ENABLE_STATS_USED_TIMESLOTS)
    target_compile_definitions(swc PUBLIC WPS_ENABLE_STATS_USED_TIMESLOTS=1)
else()
    target_compile_definitions(swc PUBLIC WPS_ENABLE_STATS_USED_TIMESLOTS=0)
endif()

include(CMakeDependentOption)
cmake_dependent_option(WPS_ENABLE_PHY_STATS_PER_BANDS "Enable per channel statistics" ON "WPS_ENABLE_PHY_STATS" OFF)
if(WPS_ENABLE_PHY_STATS_PER_BANDS)
    target_compile_definitions(swc PUBLIC WPS_ENABLE_PHY_STATS_PER_BANDS=1)
else()
    target_compile_definitions(swc PUBLIC WPS_ENABLE_PHY_STATS_PER_BANDS=0)
endif()

option(WPS_ENABLE_LINK_STATS "Whether link statistics should be compiled and usable." ON)
if(WPS_ENABLE_LINK_STATS)
    target_compile_definitions(swc PUBLIC WPS_ENABLE_LINK_STATS=1)
else()
    target_compile_definitions(swc PUBLIC WPS_ENABLE_LINK_STATS=0)
endif()

option(WPS_DISABLE_FRAGMENTATION "Whether fragmentation should be compiled and usable." OFF)
if(WPS_DISABLE_FRAGMENTATION)
    target_compile_definitions(swc PUBLIC WPS_DISABLE_FRAGMENTATION=1)
else()
    target_compile_definitions(swc PUBLIC WPS_DISABLE_FRAGMENTATION=0)
endif()

option(WPS_DISABLE_LINK_THROTTLE "Whether link throttle should be compiled and usable." OFF)
if(WPS_DISABLE_LINK_THROTTLE)
    target_compile_definitions(swc PUBLIC WPS_DISABLE_LINK_THROTTLE=1)
else()
    target_compile_definitions(swc PUBLIC WPS_DISABLE_LINK_THROTTLE=0)
endif()

if (MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    target_compile_definitions(swc PUBLIC WPS_RADIO_COUNT=2)
else()
    target_compile_definitions(swc PUBLIC WPS_RADIO_COUNT=1)
endif()

message("WPS Core being compiled for a number of " ${WPS_RADIO_COUNT} " " ${TRANSCEIVER} " transceiver")

target_compile_definitions(swc PUBLIC ${ACCESS_ADV_ERR_CHECK_EN} ${TRANSCEIVER})

add_subdirectory(api/swc)
if(WPS_INCLUDE_DEFAULT_CONFIG)
    add_subdirectory(cfg)
endif()
add_subdirectory(facade)
add_subdirectory(link)
add_subdirectory(pairing)
add_subdirectory(phy)
add_subdirectory(protocol_stack)
add_subdirectory(xlayer)
