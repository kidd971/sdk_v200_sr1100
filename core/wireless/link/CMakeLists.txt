target_sources(swc
    PRIVATE
        link_channel_hopping.c
        link_connect_status.c
        link_credit_flow_ctrl.c
        link_fallback.c
        link_lqi.c
        link_multi_radio.c
        link_phase.c
        link_protocol.c
        link_random_datarate_offset.c
        link_saw_arq.c
        link_scheduler.c
    PUBLIC
        link_channel_hopping.h
        link_connect_status.h
        link_credit_flow_ctrl.h
        link_ddcm.h
        link_error.h
        link_fallback.h
        link_lqi.h
        link_multi_radio.h
        link_phase.h
        link_protocol.h
        link_random_datarate_offset.h
        link_saw_arq.h
        link_scheduler.h
)

if(TRANSCEIVER STREQUAL "SR1000")
    add_subdirectory(sr1000)
else()
    add_subdirectory(sr1100)
endif()

target_include_directories(swc PUBLIC ${CMAKE_CURRENT_LIST_DIR})
