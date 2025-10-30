# Tests for Dinari Blockchain

# Find Google Test
find_package(GTest)

if(NOT GTest_FOUND)
    message(STATUS "Google Test not found, tests will not be built")
    message(STATUS "Install Google Test or use: cmake -DBUILD_TESTS=OFF")
    return()
endif()

include_directories(${GTEST_INCLUDE_DIRS})

# Helper function to add tests
function(add_dinari_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            dinari_core
            GTest::GTest
            GTest::Main
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Unit tests
add_dinari_test(test_hash unit/test_hash.cpp)
add_dinari_test(test_base58 unit/test_base58.cpp)
add_dinari_test(test_ecdsa unit/test_ecdsa.cpp)
add_dinari_test(test_serialize unit/test_serialize.cpp)
add_dinari_test(test_config unit/test_config.cpp)

# Integration tests (to be implemented)
# add_dinari_test(test_blockchain integration/test_blockchain.cpp)
# add_dinari_test(test_network integration/test_network.cpp)
# add_dinari_test(test_mining integration/test_mining.cpp)

message(STATUS "Tests configured")
