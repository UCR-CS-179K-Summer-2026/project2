include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# renamed source file from the smoke-test placeholder to the real ported test cases (test_smoke.cpp -> test_query_executor.cpp).
add_executable(engine_tests
    test_query_executor.cpp
)

target_link_libraries(engine_tests PRIVATE
    json_engine
    GTest::gtest_main
)

target_compile_options(engine_tests PRIVATE -Wall -Wextra)

include(GoogleTest)

# WORKING_DIRECTORY set to the repo root (not the default build/tests/ , directory ctest would otherwise use).
gtest_discover_tests(engine_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)