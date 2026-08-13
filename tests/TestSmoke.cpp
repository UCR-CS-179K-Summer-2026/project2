// Purpose: prove the full chain works before we port any actual test cases:
//   CMake configure -> fetch GoogleTest -> compile engine_tests -> link against json_engine -> discover via ctest -> run and pass.
#include <gtest/gtest.h>
 
TEST(EngineSmokeTest, HarnessBuilds) {
    SUCCEED();
}
