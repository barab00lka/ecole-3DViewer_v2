cmake_minimum_required(VERSION 3.14)
project(my_matrix CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- Библиотека my_matrix ----------
file(GLOB MY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cc"
)

# исключаем тесты, если они случайно попадут в glob
list(FILTER MY_SOURCES EXCLUDE REGEX "/tests?/")

add_library(my_matrix STATIC ${MY_SOURCES})

target_include_directories(my_matrix PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(my_matrix PRIVATE
	-Wall -Wextra -Werror -pedantic -std=c++20
)

# ---------- Тесты (опционально) ----------
option(MY_BUILD_TESTS "Build my_matrix tests" OFF)

if(MY_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include(GoogleTest)

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc"
    )

    add_executable(my_matrix_tests ${TEST_SOURCES})
    target_link_libraries(my_matrix_tests PRIVATE
        my_matrix
        GTest::gtest_main
    )
    target_compile_options(my_matrix_tests PRIVATE
        -Wall -Wextra -Werror
    )

    gtest_discover_tests(my_matrix_tests)
endif()
