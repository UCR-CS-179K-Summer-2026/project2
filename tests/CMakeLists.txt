# Pull in GoogleTest via FetchContent — CMake downloads and builds it automatically on first configure. No submodule to init/update manually
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

# Keeps GTest's runtime library settings consistent with the rest of the build rather than using its own defaults.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# placeholder only test executable, just to prove the wiring works end to end (fetch -> compile -> link against json_engine -> run via ctest). No real test cases yet, those come once this is golden.

add_executable(engine_tests
    TestSmoke.cpp
)
 
target_link_libraries(engine_tests PRIVATE
    json_engine
    GTest::gtest_main
)
 
target_compile_options(engine_tests PRIVATE -Wall -Wextra)
include(GoogleTest)
gtest_discover_tests(engine_tests)