add_executable(module_tests "")

target_sources(module_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core/wireless/tests/support/unit_test_backend.c
)

target_link_libraries(module_tests
    PRIVATE
        Catch2::Catch2WithMain
        test_utils
        swc
        fff
        spark_pairing
        test_support
)

add_subdirectory(pairing)

include(CTest)
include(Catch)
catch_discover_tests(module_tests TEST_PREFIX "Module/")
