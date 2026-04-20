add_library(module_tinyusb_audio "")
set_target_properties(module_tinyusb_audio PROPERTIES LINKER_LANGUAGE C)
target_include_directories(module_tinyusb_audio PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_sources(module_tinyusb_audio PRIVATE tinyusb_audio.c)
target_link_libraries(module_tinyusb_audio
    PRIVATE
        middleware_tinyusb
)
