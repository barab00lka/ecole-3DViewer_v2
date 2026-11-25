cmake_minimum_required(VERSION 3.27...3.28)
set(CMAKE_CXX_STANDARD 20)

# misc
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts/cmake)

# Name of the project
project(CPP_3DViewer VERSION 0.2.1)

# Options
option(BUILD_TESTS "Build tests" OFF)
# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS_LINUX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(OS_MACOS ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(OS_WINDOWS ON)
endif()

# Helper function to open URL/file in default browser
function(open_url URL)
    if(OS_LINUX)
        execute_process(COMMAND xdg-open "${URL}")
    elseif(OS_MACOS)
        execute_process(COMMAND open "${URL}")
    elseif(OS_WINDOWS)
        execute_process(COMMAND cmd /C start "" "${URL}")
    endif()
endfunction()

find_package(PkgConfig)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET 
	gtkmm-4.0
)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET 
	epoxy
)

find_program(CCACHE_PATH ccache HINTS ENV PATH)
if(CCACHE_PATH)
  message(STATUS "Using ccache: ${CCACHE_PATH}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
endif()

add_library(cgif STATIC
    thirdparty/cgif/src/cgif_raw.c
    thirdparty/cgif/src/cgif_rgb.c
    thirdparty/cgif/src/cgif.c
)
set_target_properties(cgif PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
	SKIP_LINTING ON
)
target_include_directories(cgif PUBLIC
    thirdparty/cgif/inc
)

add_subdirectory(thirdparty/fast_obj)
add_subdirectory(containers/ec_matrix)

add_library(model_lib STATIC
	misc/logger.cc
    model/wireframe_mesh.cc
    model/camera.cc
    model/color.cc
	model/texture.cc
)
target_include_directories(model_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/cgif/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb
)
target_link_libraries(model_lib PRIVATE
    fast_obj_lib
	ec_matrix
)
target_compile_features(model_lib PRIVATE cxx_std_20)
target_compile_options(model_lib PUBLIC 
	-Wall
	-Wextra
	-Werror
)

add_library(tinyfd_lib STATIC
    thirdparty/tinyfiledialogs/tinyfiledialogs.cc
)
target_include_directories(tinyfd_lib PUBLIC
    thirdparty/tinyfiledialogs
)
# fedora mingw-g++ does not like {0} initializer
target_compile_options(tinyfd_lib PRIVATE
	-Wall
	-Wextra
	-Werror
    -Wno-missing-field-initializers
    -Wno-cast-function-type
)

add_executable(3DViewer
	main.cc
	gui/main_window.cc
	gui/styling_widget.cc
	gui/affine_widget.cc
	gui/screencast_widget.cc
	gui/glarea_widget.cc
	gui/gl_renderer.cc
	gui/shader_manager.cc
	gui/config.cc
	gui/gif_writer.cc
	misc/logger.cc
	controller/scene_controller.cc
)

target_link_libraries(3DViewer PRIVATE
	model_lib
	tinyfd_lib
	cgif
	PkgConfig::GTKMM
	PkgConfig::EPOXY
)
target_compile_features(3DViewer PRIVATE cxx_std_20)
target_include_directories(3DViewer PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
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

# Sanitizers only on GCC/Clang (not MSVC)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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
endif()

if (BUILD_TESTS)
	include(FetchContent)
	FetchContent_Declare(
	  googletest
	  GIT_REPOSITORY https://github.com/google/googletest.git
	  GIT_TAG        988ea2c1798de7779f656df2281dd36d6039a17a
	  FIND_PACKAGE_ARGS NAMES GTest
	)
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
	    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
	    DEPENDS ${UNIT_TEST}
	)
endif()

# Linters
file(
	GLOB_RECURSE
	ALL_SOURCE_FILES
	./main.cc 
	./tests/*.cc
	./gui/*.cc
	./misc/*.cc
	./controller/*.cc
	./model/*.cc

	./tests/*.h
	./gui/*.h
	./misc/*.h
	./controller/*.h
	./model/*.h
	./containers/*.h
)
add_custom_target(fmt
   COMMAND clang-format -i ${ALL_SOURCE_FILES}
   USES_TERMINAL
   VERBATIM
)
set_source_files_properties( # skip clang-tidy for these sources
	thirdparty/tinyfiledialogs/tinyfiledialogs.cc
	thirdparty/tinyfiledialogs/tinyfiledialogs.h
	thirdparty/fast_obj/fast_obj.h
	thirdparty/fast_obj/fast_obj.c
	PROPERTIES
	SKIP_LINTING ON
)
include(Tidy) # if clang-tidy available runs it on all source files

# Doxygen documentation
find_program(DOXYGEN_PATH doxygen)
if(DOXYGEN_PATH)
    add_custom_target(dvi 
        COMMAND ${DOXYGEN_PATH} ${CMAKE_SOURCE_DIR}/Doxyfile
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/html ${CMAKE_BINARY_DIR}/html
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Documentation generated in ${CMAKE_BINARY_DIR}/html/index.html"
        USES_TERMINAL
        VERBATIM
    )
else()
    message(WARNING "Doxygen not found, 'dvi' target will not be available")
endif()

include(GNUInstallDirs)

install(TARGETS 3DViewer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(MINGW_SYSROOT "/usr/x86_64-w64-mingw32/sys-root/mingw"
        CACHE PATH "MinGW sysroot used for cross-compilation")

    install(DIRECTORY "${MINGW_SYSROOT}/bin/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        FILES_MATCHING PATTERN "*.dll"
    )

    install(DIRECTORY "${MINGW_SYSROOT}/share/icons"
        DESTINATION share
    )
    # Compile schemas in the staged install tree during installation/packaging.
    install(CODE "
        execute_process(
            COMMAND glib-compile-schemas ${MINGW_SYSROOT}/share/glib-2.0/schemas --strict
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR \"glib-compile-schemas failed\")
        endif()
    ")
    install(DIRECTORY "${MINGW_SYSROOT}/share/glib-2.0/schemas"
        DESTINATION share/glib-2.0
    )
	install(DIRECTORY "/tmp/Windows-10-Dark-master/"
	    DESTINATION "${CMAKE_INSTALL_DATADIR}/themes/Windows10"
	)
endif()

set(CPACK_PACKAGE_NAME        "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "3D Viewer")
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/\\.git/"
    "~$"
    "\\.o$"
    "\\.swp$"
)
include(CPack)

add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_MODULE_PATH}/uninstall.cmake"
)

# Coverage with lcov (Linux only)
if (CMAKE_BUILD_TYPE STREQUAL "Coverage")
    include(CodeCoverage)
    append_coverage_compiler_flags()
    setup_target_for_coverage_lcov(
        NAME coverage
        EXECUTABLE model_tests
        DEPENDENCIES model_tests
        BASE_DIRECTORY ${CMAKE_SOURCE_DIR}
        LCOV_ARGS --ignore-errors unused,inconsistent,inconsistent,mismatch 
        GENHTML_ARGS --ignore-errors mismatch
        EXCLUDE "/usr/include/*"
        "${CMAKE_SOURCE_DIR}/thirdparty/*"
        "${CMAKE_SOURCE_DIR}/containers/*"
        "${CMAKE_SOURCE_DIR}/tests/*"
    )
endif()
