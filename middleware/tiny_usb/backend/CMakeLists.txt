if (${HARDWARE} STREQUAL "PULSAR")
    set(TINY_USB_BACKEND_PATH  ${CMAKE_CURRENT_LIST_DIR}/pulsar CACHE STRING "Path to hardware specific implementations")
elseif(${HARDWARE} STREQUAL "QUASAR")
set(TINY_USB_BACKEND_PATH  ${CMAKE_CURRENT_LIST_DIR}/quasar CACHE STRING "Path to hardware specific implementations")
elseif(${HARDWARE} STREQUAL "EVK")
set(TINY_USB_BACKEND_PATH  ${CMAKE_CURRENT_LIST_DIR}/evk CACHE STRING "Path to hardware specific implementations")
endif()
