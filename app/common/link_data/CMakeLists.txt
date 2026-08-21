# Header only. It lives in app/common rather than beside one application because the wire
# format it describes is a contract with the ODM's SOC, and a contract with two definitions
# is the thing this header was created to stop.
add_library(link_data INTERFACE)

target_sources(link_data INTERFACE puretone_link_data.h)

target_include_directories(link_data INTERFACE ${CMAKE_CURRENT_LIST_DIR})

# at_cmd_core.h supplies AT_VENDOR_PAYLOAD_MAX and at_vendor_frame_t: the vendor block's
# budget and shape are AT-level facts, and the inline pack/deliver helpers call into the core.
target_link_libraries(link_data INTERFACE at_cmd_core)
