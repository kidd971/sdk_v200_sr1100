add_library(logger "")

target_sources(logger
    PRIVATE
        uwb_log.c
    PUBLIC
        uwb_log.h
)

target_link_libraries(logger PRIVATE buffer)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_LIST_DIR})
