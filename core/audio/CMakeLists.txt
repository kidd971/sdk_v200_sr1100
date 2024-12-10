cmake_minimum_required(VERSION 3.18)

add_library(audio_core "")

target_sources(audio_core
    PRIVATE
        api/sac_api.c
        api/sac_stats.c
        api/sac_utils.c
        endpoint/sac_dummy_endpoint.c
        endpoint/sac_sinus_endpoint.c
        endpoint/sac_endpoint_swc.c
        gate/sac_fallback_gate.c
        module/sac_mixer_module.c
        processing/sac_compression.c
        processing/sac_fallback.c
        processing/sac_mute_on_underflow.c
        processing/sac_mute_packet.c
        processing/sac_packing.c
        processing/sac_volume.c
        processing/sac_src_cmsis.c
        processing/sac_cdc.c
        processing/sac_cdc_pll.c
    PUBLIC
        endpoint/sac_dummy_endpoint.h
        endpoint/sac_sinus_endpoint.h
        endpoint/sac_endpoint_swc.h
        gate/sac_fallback_gate.h
        module/sac_mixer_module.h
        processing/sac_compression.h
        processing/sac_fallback.h
        processing/sac_mute_on_underflow.h
        processing/sac_mute_packet.h
        processing/sac_packing.h
        processing/sac_volume.h
        processing/sac_src_cmsis.h
        processing/sac_cdc.h
        processing/sac_cdc_pll.h
        sac_api.h
        sac_error.h
        sac_stats.h
        sac_utils.h
)

target_link_libraries(audio_core PUBLIC adpcm cmsis_5 crc4_itu filtering_functions memory queue resampling swc)
target_include_directories(audio_core
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/endpoint
        ${CMAKE_CURRENT_LIST_DIR}/gate
        ${CMAKE_CURRENT_LIST_DIR}/module
        ${CMAKE_CURRENT_LIST_DIR}/processing
)
