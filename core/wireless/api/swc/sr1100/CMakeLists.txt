#Include Extension API base on CMake build option.
if (${SWC_EXT_API_ENABLE})
    add_subdirectory(extension)
endif()

target_sources(swc
    PRIVATE
        swc_api.c
)
target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
