add_library(swc_backend "")
target_link_libraries(swc_backend
    PUBLIC
        swc_facade
    PRIVATE
        hardware
)
target_sources(swc_backend PRIVATE wireless_core_backend.c)
