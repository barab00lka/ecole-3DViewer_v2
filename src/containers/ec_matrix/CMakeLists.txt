cmake_minimum_required(VERSION 3.14)
project(ec_matrix CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- Библиотека ec_matrix ----------
file(GLOB EC_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
)

# исключаем тесты, если они случайно попадут в glob
list(FILTER EC_SOURCES EXCLUDE REGEX "/tests?/")

add_library(ec_matrix STATIC ${EC_SOURCES})

target_include_directories(ec_matrix PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(ec_matrix PRIVATE
	-Wall -Wextra -Werror -pedantic -std=c++20
)

# ---------- Тесты (опционально) ----------
option(EC_BUILD_TESTS "Build ec_matrix tests" OFF)

if(EC_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc"
    )

    add_executable(ec_matrix_tests ${TEST_SOURCES})
    target_link_libraries(ec_matrix_tests PRIVATE
        ec_matrix
        GTest::gtest_main
    )
    target_compile_options(ec_matrix_tests PRIVATE
        -Wall -Wextra -Werror
    )

    gtest_discover_tests(ec_matrix_tests)
endif()
