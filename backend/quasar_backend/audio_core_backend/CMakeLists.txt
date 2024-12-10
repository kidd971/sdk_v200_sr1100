target_link_libraries(audio_core PRIVATE hardware)
target_sources(audio_core PRIVATE audio_core_backend.c)
