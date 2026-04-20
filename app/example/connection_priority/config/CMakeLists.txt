target_include_directories(connection_priority_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(TRANSCEIVER STREQUAL "SR1000")
    target_include_directories(connection_priority_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sr1000)
else()
    target_include_directories(connection_priority_backend PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sr1100)
endif()
