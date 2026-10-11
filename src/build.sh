#!/bin/sh

# Check prerequisites
if ! command -v cmake > /dev/null 2>&1; then
    echo "Please install cmake"
    exit 1
fi

if ! command -v gtk4-launch > /dev/null 2>&1; then
    echo "Please install gtk4 and gtkmm-4.0"
    exit 1
fi

# Function to display help
show_help() {
    echo "OPTIONS:"
    echo
    echo " --asan"
    echo "    build with Asan enabled (clang only with -fsanitize) and run executable"
    echo
    echo " --test"
    echo "    build and run tests"
    echo
    echo " --with-coverage"
    echo "    build tests with gcov coverage (gcc toolchain) and create html with coverage data using lcov"
    echo
    echo " --valgrind"
    echo "    build with debug info and run executable under valgrind"
    echo
    echo " --debug"
    echo "    build with debug info and run"
    echo
    echo " -h | --help"
    echo "    show available options"
}

# Determine build options
BUILD_TYPE=""
TARGET=""
CMD=""

case "$1" in
    -h|--help)
        show_help
        exit 0
        ;;
    --asan)
        BUILD_TYPE="Sanitize"
        TARGET="all"
        ;;
    --test)
        TARGET="tests"
        ;;
    --with-coverage)
        BUILD_TYPE="Coverage"
        TARGET="coverage"
        ;;
    --debug)
        BUILD_TYPE="Debug"
        TARGET="all"
        ;;
    --valgrind)
        BUILD_TYPE="Debug"
        TARGET="all"
        CMD="valgrind --leak-check=summary --show-leak-kinds=all --suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp --suppressions=/usr/share/glib-2.0/valgrind/glib.supp"
        ;;
    *)
        BUILD_TYPE="Release"
        TARGET="all"
        ;;
esac

# Run build
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE" && cmake --build build -j"$(nproc --ignore=2)" -t "$TARGET" && $CMD ./build/3DViewer

