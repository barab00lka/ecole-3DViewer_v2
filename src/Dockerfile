FROM fedora:45 AS base
RUN dnf install --assumeyes cmake pkgconf git meson ninja-build
# the resource compiler for gtk is needed for some gtkmm-demo program, probably can be omitted if the demo is not mandatory
RUN dnf install --assumeyes glib2-devel  

FROM base AS mingw-release
# build gtkmm-4.0 and its dependencies from source 
RUN dnf install --assumeyes mingw64-crt mingw64-gtk4 mingw64-gcc-c++ mingw64-cmake mingw64-libsigc++30 mingw64-glib2
WORKDIR /tmp
# source dependencies first
ARG CAIROMM_VER="1.19.1" \
	GLIBMM_VER="2.89" \
	PANGOMM_VER="2.58" \
	GTKMM_VER="4.22"
# --- cairomm ---
RUN curl -fsSLo "cairomm-$CAIROMM_VER.tar.xz" "https://cairo.freedesktop.org/releases/cairomm-$CAIROMM_VER.tar.xz" \
 && curl -fsSLo "cairomm-$CAIROMM_VER.tar.xz.sha256sum" "https://cairo.freedesktop.org/releases/cairomm-$CAIROMM_VER.tar.xz.sha256sum" \
 && sha256sum -c --ignore-missing "cairomm-$CAIROMM_VER.tar.xz.sha256sum" \
 && tar xf "cairomm-$CAIROMM_VER.tar.xz" \
 && rm "cairomm-$CAIROMM_VER.tar.xz" "cairomm-$CAIROMM_VER.tar.xz.sha256sum"
# --- glibmm ---
RUN curl -fsSLo "glibmm-$GLIBMM_VER.0.tar.xz" "https://download.gnome.org/sources/glibmm/$GLIBMM_VER/glibmm-$GLIBMM_VER.0.tar.xz" \
 && curl -fsSLo "glibmm-$GLIBMM_VER.0.sha256sum" "https://download.gnome.org/sources/glibmm/$GLIBMM_VER/glibmm-$GLIBMM_VER.0.sha256sum" \
 && sha256sum -c --ignore-missing "glibmm-$GLIBMM_VER.0.sha256sum" \
 && tar xf "glibmm-$GLIBMM_VER.0.tar.xz" \
 && rm "glibmm-$GLIBMM_VER.0.tar.xz" "glibmm-$GLIBMM_VER.0.sha256sum"
# --- pangomm ---
RUN curl -fsSLo "pangomm-$PANGOMM_VER.0.tar.xz" "https://download.gnome.org/sources/pangomm/$PANGOMM_VER/pangomm-$PANGOMM_VER.0.tar.xz" \
 && curl -fsSLo "pangomm-$PANGOMM_VER.0.sha256sum" "https://download.gnome.org/sources/pangomm/$PANGOMM_VER/pangomm-$PANGOMM_VER.0.sha256sum" \
 && sha256sum -c --ignore-missing "pangomm-$PANGOMM_VER.0.sha256sum" \
 && tar xf "pangomm-$PANGOMM_VER.0.tar.xz" \
 && rm "pangomm-$PANGOMM_VER.0.tar.xz" "pangomm-$PANGOMM_VER.0.sha256sum"
# --- gtkmm ---
RUN curl -fsSLo "gtkmm-$GTKMM_VER.0.tar.xz" "https://download.gnome.org/sources/gtkmm/$GTKMM_VER/gtkmm-$GTKMM_VER.0.tar.xz" \
 && curl -fsSLo "gtkmm-$GTKMM_VER.0.sha256sum" "https://download.gnome.org/sources/gtkmm/$GTKMM_VER/gtkmm-$GTKMM_VER.0.sha256sum" \
 && sha256sum -c --ignore-missing "gtkmm-$GTKMM_VER.0.sha256sum" \
 && tar xf "gtkmm-$GTKMM_VER.0.tar.xz" \
 && rm "gtkmm-$GTKMM_VER.0.tar.xz" "gtkmm-$GTKMM_VER.0.sha256sum"
# set environment variables for mingw cross-compilation
ENV CC="x86_64-w64-mingw32-gcc" \
	CXX="x86_64-w64-mingw32-g++" \
	AR="x86_64-w64-mingw32-ar" \
	PKG_CONFIG="x86_64-w64-mingw32-pkg-config" \
	WINDRES="x86_64-w64-mingw32-windres"
RUN meson env2mfile --cross -o mingw64-cross.txt \
    --system windows \
	--subsystem windows \
	--kernel nt \
    --cpu-family x86_64 \
    --cpu x86_64 \
    --endian little

RUN cd cairomm-$CAIROMM_VER \
	&& meson setup \
	--prefix=/usr/x86_64-w64-mingw32/sys-root/mingw \
	--cross-file ../mingw64-cross.txt \
	-Dmaintainer-mode=false \
	-Dbuild-documentation=false \
	_build . \
	&& cd _build \
	&& ninja
RUN cd cairomm-$CAIROMM_VER/_build \
	&& ninja install

RUN cd glibmm-$GLIBMM_VER.0 \
	&& meson setup \
	--prefix=/usr/x86_64-w64-mingw32/sys-root/mingw \
	--cross-file ../mingw64-cross.txt \
	-Dmaintainer-mode=false \
	-Dbuild-documentation=false \
	_build . \
	&& cd _build \
	&& ninja
RUN cd glibmm-$GLIBMM_VER.0/_build \
	&& ninja install

RUN cd pangomm-$PANGOMM_VER.0 \
	&& meson setup \
	--prefix=/usr/x86_64-w64-mingw32/sys-root/mingw \
	--cross-file ../mingw64-cross.txt \
	-Dmaintainer-mode=false \
	-Dbuild-documentation=false \
	_build . \
	&& cd _build \
	&& ninja
RUN cd pangomm-$PANGOMM_VER.0/_build \
	&& ninja install

RUN cd gtkmm-$GTKMM_VER.0 \
	&& meson setup \
	--prefix=/usr/x86_64-w64-mingw32/sys-root/mingw \
	--cross-file ../mingw64-cross.txt \
	-Dmaintainer-mode=false \
	-Dbuild-demos=false \
	-Dbuild-documentation=false \
	_build . \
	&& cd _build \
	&& ninja
RUN cd gtkmm-$GTKMM_VER.0/_build \
	&& ninja install
 # --- DL gtk4 assets win10 theme ---
RUN curl -fsSLo "w10-dark.tar.gz" "https://github.com/B00merang-Project/Windows-10-Dark/archive/refs/heads/master.tar.gz" \
 && tar xzvf "w10-dark.tar.gz" Windows-10-Dark-master/gtk-4.0 \
 && rm "w10-dark.tar.gz"
 RUN dnf install --assumeyes mingw64-adwaita-icon-theme mingw64-hicolor-icon-theme
# cleanup image
RUN dnf remove --assumeyes glib2-devel && dnf --assumeyes clean all
WORKDIR /wrksrc
# build binary release with DLLs for Win64 by default
CMD cmake -B build-windows -DCMAKE_SYSTEM_NAME="Windows" -DCMAKE_SYSTEM_PROCESSOR="x86_64" -DBUILD_TESTS=OFF && \
	cmake --build build-windows -j$(nproc --ignore 2) -t package

# Linux GNU libc build
FROM base as glibc-release
RUN dnf install --assumeyes gtkmm4.0-devel gtest lcov g++
RUN dnf install --assumeyes libepoxy-devel 
WORKDIR /wrksrc
# build binary release for Linux by default
CMD cmake -B build-linux -DCMAKE_SYSTEM_NAME="Linux" -DBUILD_TESTS=ON && \
	cmake --build build-linux -j$(nproc --ignore 2) -t package
