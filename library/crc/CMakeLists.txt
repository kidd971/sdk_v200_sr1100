add_library(crc4_itu "")

target_sources(crc4_itu
    PRIVATE
        crc4_itu.c
    PUBLIC
        crc4_itu.h
)

target_include_directories(crc4_itu PUBLIC ${CMAKE_CURRENT_LIST_DIR})
