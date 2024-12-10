add_library(adpcm "")

target_sources(adpcm
    PRIVATE
        adpcm.c
    PUBLIC
        adpcm.h
)

target_include_directories(adpcm PUBLIC ${CMAKE_CURRENT_LIST_DIR})
