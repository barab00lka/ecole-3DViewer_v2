prefix		 ?= ~/.local
APP_NAME      = 3DViewer
BUILD_DIR     = build
ROOT_DIR	 := $(shell 'dirname' $(realpath $(firstword $(MAKEFILE_LIST))))
NPROCS        = $(shell nproc --ignore=2 2>/dev/null || echo 1)

CONTAINER_CMD = $(shell command -v docker 2>/dev/null || command -v podman 2>/dev/null)

VALGRIND_CMD  = valgrind --leak-check=full --show-leak-kinds=all \
				--suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp \
				--suppressions=/usr/share/glib-2.0/valgrind/glib.supp

.PHONY: all compile run install uninstall clean dvi dist tests asan coverage valgrind debug win64-release glibc-release check-deps help

all: check-deps compile

check-deps:
	@command -v cmake >/dev/null 2>&1 || { echo >&2 "Please install cmake"; exit 1; }
	@command -v gtk4-launch >/dev/null 2>&1 || { echo >&2 "Please install gtk4 and gtkmm-4.0"; exit 1; }

compile:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target all

run: all
	./$(BUILD_DIR)/$(APP_NAME)

install: all
	cmake --install $(BUILD_DIR) --prefix $(prefix)

# Uninstalls the program if install_manifest exists (if installed somewhere)
uninstall: check-deps
	cmake --build $(BUILD_DIR) --target uninstall

clean:
	rm -rf $(BUILD_DIR) html

# Generate documentation
dvi:
	@if command -v doxygen >/dev/null 2>&1 && [ -f Doxyfile ]; then \
		doxygen Doxyfile; \
	else \
		echo "Create a Doxyfile or specify a documentation-generation command (texi2dvi/doxygen)"; \
	fi

dist: clean all
	cmake --build $(BUILD_DIR) --target package_source

tests: check-deps
	cmake -B $(BUILD_DIR) -DBUILD_TESTS=ON
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target tests

# Build with AddressSanitizer
asan: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Sanitize
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target all

# Generate a code coverage report using gcov/lcov (--with-coverage)
coverage: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target coverage

debug: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target all

run-debug: debug
	GTK_DEBUG=interactive ./$(BUILD_DIR)/$(APP_NAME)

# Check for memory leaks with Valgrind (--valgrind)
valgrind: debug
	@command -v valgrind >/dev/null 2>&1 || { echo >&2 "valgrind not found"; exit 1; }
	$(VALGRIND_CMD) ./$(BUILD_DIR)/$(APP_NAME)

win64-release:
	@if [ -z "$(CONTAINER_CMD)" ]; then \
		echo "docker or podman not found; can't build cross-container" >&2; \
		exit 1; \
	fi
	$(CONTAINER_CMD) build --target mingw-release -t 3dviewer-mingw-build .
	$(CONTAINER_CMD) run --rm --interactive --volume $(ROOT_DIR):/wrksrc 3dviewer-mingw-build
	@echo "Successfully built a self-contained Windows_x86_64 distribution"

glibc-release: 
	@if [ -z "$(CONTAINER_CMD)" ]; then \
		echo "docker or podman not found; can't build cross-container" >&2; \
		exit 1; \
	fi
	$(CONTAINER_CMD) build --target glibc-release -t 3dviewer-gnu-build .
	$(CONTAINER_CMD) run --rm --interactive --volume $(ROOT_DIR):/wrksrc 3dviewer-gnu-build
	@echo "Successfully built a GNU libc binary distribution"


deps:
	sudo apt install libgtk-4-dev libgtkmm-4.0-dev pkg-config -y
	sudo apt install zenity -y cmake
	sudo apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev

help:
	@echo "Available Makefile targets:"
	@echo "  all        - Release build of the project (default)"
	@echo "  run        - Build and run 3DViewer"
	@echo "  install --prefix= - Install the program in a custom directory (~/.local by default)"
	@echo "  uninstall  - Remove the installed program"
	@echo "  clean      - Remove build artifacts"
	@echo "  dvi        - Generate documentation"
	@echo "  dist       - Create a tar.gz / zip source archive"
	@echo "  tests      - Build and run unit tests"
	@echo "  asan       - Build with AddressSanitizer (Clang)"
	@echo "  coverage   - Generate a code coverage report (gcov/lcov)"
	@echo "  debug      - Debug build"
	@echo "  run-debug  - Run the Debug build with GTK_DEBUG=interactive"
	@echo "  valgrind   - Run the Debug build under Valgrind"
	@echo "  win64-release  - Cross-compile a Windows x86_64 distribution in a container (Fedora mingw64)"
	@echo "  glibc-release  - Binary distribution for GNU libc Linux in a Fedora container"
