target_sources(swc
    PRIVATE
        swc_api_extension.c
    PUBLIC
        swc_api_extension.h
)
target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
