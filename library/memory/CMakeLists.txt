add_library(memory "")

target_sources(memory
    PRIVATE
        mem_pool.c
    PUBLIC
        mem_pool.h
)

target_link_libraries(memory PRIVATE buffer)
target_include_directories(memory PUBLIC ${CMAKE_CURRENT_LIST_DIR})
