target_sources(swc
    PRIVATE
        swc_hal_facade.c
)

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
