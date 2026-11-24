prefix		 ?= ~/.local
APP_NAME      = 3DViewer
BUILD_DIR     = build
ROOT_DIR	 := $(shell 'dirname' $(realpath $(firstword $(MAKEFILE_LIST))))
NPROCS        = $(shell nproc --ignore=2 2>/dev/null || echo 1)

CONTAINER_CMD = $(shell command -v docker 2>/dev/null || command -v podman 2>/dev/null)

VALGRIND_CMD  = valgrind --leak-check=full --show-leak-kinds=all \
                --suppressions=/usr/share/gtk-4.0/valgrind/gtk.supp \
                --suppressions=/usr/share/glib-2.0/valgrind/glib.supp

.PHONY: all compile run install uninstall clean dvi dist tests asan coverage valgrind debug cross-win check-deps help

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

# will uninstall if install_manifest exist (if installed somewhere)
uninstall: check-deps
	cmake --build $(BUILD_DIR) --target uninstall

clean:
	rm -rf $(BUILD_DIR) html

# Генерация документации 
dvi:
	@if command -v doxygen >/dev/null 2>&1 && [ -f Doxyfile ]; then \
		doxygen Doxyfile; \
	else \
		echo "Создайте Doxyfile или укажите команду генерации документации (texi2dvi/doxygen)"; \
	fi

dist: clean all
	cmake --build $(BUILD_DIR) --target package_source

tests: check-deps
	cmake -B $(BUILD_DIR) -DBUILD_TESTS=ON
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target tests

# Сборка с AddressSanitizer
asan: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Sanitize
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target all

# Отчет о покрытии кода тестами gcov/lcov (--with-coverage)
coverage: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target coverage

debug: check-deps
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR) -j$(NPROCS) --target all

run-debug: debug
	GTK_DEBUG=interactive ./$(BUILD_DIR)/$(APP_NAME)

# Проверка утечек памяти через Valgrind (--valgrind)
valgrind: debug
	@command -v valgrind >/dev/null 2>&1 || { echo >&2 "valgrind not found"; exit 1; }
	$(VALGRIND_CMD) ./$(BUILD_DIR)/$(APP_NAME)

# Кросс-компиляция под Windows в Docker/Podman (--cross-win)
cross-win:
	@if [ -z "$(CONTAINER_CMD)" ]; then \
		echo "docker or podman not found; can't build cross-container" >&2; \
		exit 1; \
	fi
	$(CONTAINER_CMD) build -t mingw-cross .
	$(CONTAINER_CMD) run --rm --interactive --volume $(ROOT_DIR):/wrksrc mingw-cross
	@echo "Successfully built an Windows_x86_64 self-contained distribution"


deps:
	sudo apt install libgtk-4-dev libgtkmm-4.0-dev pkg-config -y
	sudo apt install zenity -y cmake
	sudo apt install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev

help:
	@echo "Доступные цели Makefile:"
	@echo "  all        - Релизная сборка проекта (по умолчанию)"
	@echo "  run        - Собрать и запустить 3DViewer"
	@echo "  install --prefix=	- Установить программу в произвольной директории (~/.local по умолчанию)"
	@echo "  uninstall  - Удалить установленную программу"
	@echo "  clean      - Очистить артефакты сборки"
	@echo "  dvi        - Сгенерировать документацию"
	@echo "  dist       - Собрать tar.gz / zip архив с исходниками"
	@echo "  tests      - Собрать и запустить unit-тесты"
	@echo "  asan       - Сборка с AddressSanitizer (Clang)"
	@echo "  coverage   - Генерация отчета о покрытии кода (gcov/lcov)"
	@echo "  debug      - Debug-сборка"
	@echo "  run-debug	- запуск Debug сборки с GTK_DEBUG=interactive"
	@echo "  valgrind   - запуск Debug сборки под Valgrind"
	@echo "  cross-win  - Кросс-компиляция дистрибутива под Windows x86_64 в контейнере (Fedora mingw64)"
