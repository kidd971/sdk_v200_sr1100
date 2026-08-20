add_library(reconnect_store "")

target_sources(reconnect_store
    PRIVATE
        reconnect_store.c
    PUBLIC
        reconnect_store.h
        reconnect_store_facade.h
)

target_include_directories(reconnect_store PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# pairing_api.h only, for pairing_assigned_address_t -- the record is four bytes
# of pairing result and nothing else.
target_link_libraries(reconnect_store PUBLIC spark_pairing)
