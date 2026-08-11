add_library(swc_facade)
target_sources(swc_facade
    PRIVATE
        swc_hal_facade.c
)
target_include_directories(swc_facade
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)
