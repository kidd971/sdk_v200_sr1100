add_library(crc4_itu "")

target_sources(crc4_itu
    PRIVATE
        crc4_itu.c
    PUBLIC
        crc4_itu.h
)

target_include_directories(crc4_itu PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(crc8 "")

target_sources(crc8
    PRIVATE
        crc8.c
    PUBLIC
        crc8.h
)

target_include_directories(crc8 PUBLIC ${CMAKE_CURRENT_LIST_DIR})
