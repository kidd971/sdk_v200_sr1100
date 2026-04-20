target_sources(middleware_tinyusb PRIVATE backend.c)
# QUASAR board only has one USB port
target_compile_definitions(middleware_tinyusb PUBLIC BOARD_TUD_RHPORT=0)
# QUASAR board supports USB HIGH-SPEED
target_compile_definitions(middleware_tinyusb PUBLIC BOARD_TUD_MAX_SPEED=OPT_MODE_HIGH_SPEED)
