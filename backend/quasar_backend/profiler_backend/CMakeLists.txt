if (MULTI_TRANSCEIVER STREQUAL "DUAL_TRANSCEIVER")
    # Profiler does not support dual radio.
    return()
endif()
target_link_libraries(profiler_backend PRIVATE hardware)
target_sources(profiler_backend PRIVATE profiler_backend.c)
