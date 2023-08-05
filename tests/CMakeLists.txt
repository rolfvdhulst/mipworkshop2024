
find_package(GTest REQUIRED)
add_executable(mipworkshop2024_tests
        test_main.cpp
        MPSReaderTest.cpp)

target_link_libraries(mipworkshop2024_tests
        PUBLIC mipworkshop2024
        PUBLIC GTest::GTest)