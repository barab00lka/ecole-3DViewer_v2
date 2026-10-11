cmake_minimum_required(VERSION 3.14...3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
## misc
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=-*,google-*")
# set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

# Name of the project
project(CPP_3DViewer VERSION 2.0.0)

find_package(PkgConfig)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET 
	gtkmm-4.0
)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET 
	epoxy
)
add_library(cgif STATIC
    thirdparty/cgif/src/cgif_raw.c
    thirdparty/cgif/src/cgif_rgb.c
    thirdparty/cgif/src/cgif.c
)
set_target_properties(cgif PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
target_include_directories(cgif PUBLIC
    thirdparty/cgif/inc
)
add_subdirectory(thirdparty/fast_obj)
add_subdirectory(containers/my_matrix)

add_library(model_lib STATIC
    model/wireframe_mesh.cc
    model/camera.cc
    model/color.cc
)
target_include_directories(model_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cgif/inc
)
target_link_libraries(model_lib PUBLIC
    fast_obj_lib
	my_matrix
)

add_executable(3DViewer 
	main.cc
	gui/main_window.cc
	gui/glarea_widget.cc
	gui/gl_renderer.cc
	gui/shader_manager.cc
	gui/config.cc
	gui/gif_writer.cc
	controller/scene_controller.cc
	thirdparty/tinyfiledialogs/tinyfiledialogs.cc
)
target_link_libraries(3DViewer PRIVATE
	model_lib
	my_matrix
	PkgConfig::GTKMM
	PkgConfig::EPOXY
	fast_obj_lib
	cgif
)
target_include_directories(3DViewer PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	thirdparty/tinyfiledialogs
	thirdparty/cgif/inc
)
# target_precompile_headers(3DViewer PRIVATE 
#     <gtkmm.h>
#     <epoxy/gl.h>
# )
target_compile_options(3DViewer PUBLIC 
	-Wall
	-Wextra
	-Werror
)
if (CMAKE_BUILD_TYPE STREQUAL "Sanitize")
	target_compile_options(3DViewer PUBLIC 
		-fsanitize=address
		-fsanitize=leak
		-fsanitize=undefined
	)
	target_link_options(3DViewer PUBLIC
		-fsanitize=address
		-fsanitize=leak
		-fsanitize=undefined
	)
endif()

# Tests
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/988ea2c1798de7779f656df2281dd36d6039a17a.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(model_tests
    tests/tests.cc
)
target_link_libraries(model_tests PRIVATE
    model_lib
    GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(model_tests)

enable_testing()
set(UNIT_TEST model_tests)
add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TEST})
add_custom_target(tests
	# ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${UNIT_TEST}
)

add_custom_target(lcov_report
    COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}
        -B ${CMAKE_BINARY_DIR}/gcov
        -DCMAKE_BUILD_TYPE=Coverage
    COMMAND ${CMAKE_COMMAND}
        --build ${CMAKE_BINARY_DIR}/gcov --target gcov
    COMMENT "Building coverage reports in build/gcov/coverage"
	COMMAND wslview ${CMAKE_BINARY_DIR}/gcov/coverage/index.html || xdg-open ${CMAKE_BINARY_DIR}/gcov/coverage/index.html

    USES_TERMINAL
    VERBATIM
)

#clang-format 
file(GLOB_RECURSE ALL_SOURCE_FILES *.cc *.h)
add_custom_target(fmt
   # ALL # uncomment to include in default 'make all' target
   COMMAND clang-format
   -style=Google
   -i
   ${ALL_SOURCE_FILES}
    USES_TERMINAL
	VERBATIM
)

add_custom_target(dvi 
	find_program(DOXYGEN_PATH doxygen REQUIRED)
	COMMAND ${DOXYGEN_PATH} ${CMAKE_SOURCE_DIR}/Doxyfile && mv html ${CMAKE_BINARY_DIR}/. 
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMAND wslview realpath ${CMAKE_BINARY_DIR}/html/index.html || xdg-open ${CMAKE_BINARY_DIR}/html/index.html
    USES_TERMINAL
	VERBATIM
)

include(GNUInstallDirs)   ######################## даёт CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_DATADIR и др.
install(TARGETS 3DViewer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) #######################################

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake"
)

set(CPACK_PACKAGE_NAME        "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "3D Viewer")
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
# Files to include in a *source* tarball:
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/\\.git/"
    "~$"
    "\\.o$"
    "\\.swp$"
)
include(CPack)
add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package_source
    COMMENT "Building source distribution (package_source)"
    USES_TERMINAL
    VERBATIM
)
# Source - https://stackoverflow.com/a/37967856
# Posted by galsh83, modified by community. See post 'Timeline' for change history
# Retrieved 2026-07-27, License - CC BY-SA 3.0
if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)

	#	1. Add necessary compiler flags to targets
    append_coverage_compiler_flags()

	  # 2. Создаём цель для генерации отчёта (например, с lcov)
	  setup_target_for_coverage_lcov(
	      NAME coverage
	      EXECUTABLE model_tests      # исполняемый файл (тесты)
	      DEPENDENCIES model_tests    # dependency targets
	      BASE_DIRECTORY ${CMAKE_SOURCE_DIR}
	LCOV_ARGS --ignore-errors unused,inconsistent,inconsistent,mismatch 
	GENHTML_ARGS --ignore-errors mismatch
	EXCLUDE "/usr/include/*"
	      "${CMAKE_SOURCE_DIR}/thirdparty/*"
	"${CMAKE_SOURCE_DIR}/containers/*"
	      "${CMAKE_SOURCE_DIR}/tests/*"
	  )
endif()
