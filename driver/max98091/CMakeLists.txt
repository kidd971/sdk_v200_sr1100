add_library(max98091_driver "")

target_sources(max98091_driver
    PRIVATE
        max98091.c
    PUBLIC
        max98091.h
        max98091_reg.h
    )

target_include_directories(max98091_driver PUBLIC ${CMAKE_CURRENT_LIST_DIR})
