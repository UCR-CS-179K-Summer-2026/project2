cmake_minimum_required(VERSION 3.14)
project(JsonAnalyticsEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# [CHANGED] Engine code now builds as a library so it can be linked by both the CLI executable below and (later) a test executable, without either one dragging in the other's main().
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

# [CHANGED] test_executor is now just main.cpp, linked against json_engine. Same behavior as before, just reorganized.
add_executable(test_executor main.cpp)
target_link_libraries(test_executor PRIVATE json_engine)
target_compile_options(test_executor PRIVATE -Wall -Wextra)