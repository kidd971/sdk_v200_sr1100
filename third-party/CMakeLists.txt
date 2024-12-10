# Configuration these libraries exclusively for applications dependent on the nanopb and pigweed.
if(RPC_ENABLED)
    set(dir_pw_third_party_nanopb ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/src CACHE PATH "Path to the Nanopb installation." FORCE)
    add_subdirectory(nanopb)
    add_subdirectory(pigweed)
    add_subdirectory(littlefs)
endif()

if(RTOS_ENABLED)
    add_subdirectory(cmsis_freertos)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/cmsis_5")
    add_subdirectory(cmsis_5)
endif()
if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/stmicroelectronics")
    add_subdirectory(stmicroelectronics)
endif()

