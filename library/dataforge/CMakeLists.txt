add_library(library_dataforge "")

target_sources(library_dataforge
    PRIVATE
        dataforge.c
)

target_include_directories(library_dataforge PUBLIC ${CMAKE_CURRENT_LIST_DIR})

