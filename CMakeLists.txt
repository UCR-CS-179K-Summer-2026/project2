cmake_minimum_required(VERSION 3.14)
project(JsonAnalyticsEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#now this is for test coverage check. Compiles with --coverage -O0 -g, which instruments the binary to emit .gcda/.gcno files when tests run the raw data gcovr reads to compute line/branch coverage percentages.
# Usage: cmake -B build-coverage -DCMAKE_BUILD_TYPE=Coverage

set(CMAKE_CXX_FLAGS_COVERAGE "-g -O0 --coverage")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "--coverage")

# Engine code now builds as a library so it can be linked by both the CLI executable below and (later) a test executable, without either one dragging in the other's main().
add_library(json_engine
    AaronJsonParser/parser.cpp
    TasnimQueryParser/QueryParser.cpp
)

# query_executor.h and QueryStruct.h are header-only; exposing these as #include dirs keeps existing #include "..." paths working unchanged.
target_include_directories(json_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/AaronJsonParser
    ${CMAKE_CURRENT_SOURCE_DIR}/TasnimQueryParser
)

target_compile_options(json_engine PRIVATE -Wall -Wextra)

# test_executor is now just main.cpp, linked against json_engine. Same behavior as before, just reorganized.
add_executable(test_executor main.cpp)
target_link_libraries(test_executor PRIVATE json_engine)
target_compile_options(test_executor PRIVATE -Wall -Wextra)

#test scaffolding.
# enable_testing() turns on CTest support for this whole project, letting `ctest` discover and run any tests registered below (via add_subdirectory).
enable_testing()
add_subdirectory(tests)

# coverage report target. Only defined if gcovr is found, so the build doesn't break for anyone who doesn't have gcovr installed.
# Usage:
#   cmake -B build-coverage -DCMAKE_BUILD_TYPE=Coverage
#   cmake --build build-coverage
#   ctest --test-dir build-coverage
#   cmake --build build-coverage --target coverage

find_program(GCOVR_PATH gcovr)
if(GCOVR_PATH)
    # The actual gcovr invocation lives in scripts/coverage.sh instead of inline here.
    add_custom_target(coverage
        COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/coverage.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating coverage report (see build-coverage/coverage-report/index.html)"
    )
else()
    message(STATUS "gcovr not found. 'coverage' target will be unavailable. Install with: pip install gcovr --break-system-packages")
endif()