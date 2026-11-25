# ecole-3DViewer_v2
A simple GTK4 3D model viewer. 
Currently a work in progress.

## Screenshots
[![Screenshot](screenshots/image.png)](screenshots/image.png)
[![Screenshot](screenshots/ak.png)](screenshots/ak.png)

## Development builds
Dependencies:
 - make (gmake)

To build natively on Linux you also need these:
 - a C++20 compiler toolchain (gcc / clang)
 - cmake 
 - pkgconf
 - gtkmm-4.0-dev 
      
Then just run:

```sh
git clone --recurse-submodules https://github.com/barab00lka/ecole-3DViewer_v2.git
cd ecole-3DViewer_v2/src
make
```

for list of additional options: ` make help `

## Release builds
Releases are built in a Fedora container, so for this you need either Docker or Podman installed
### Linux GNU libc

```sh
make glibc-release
```

### Experimental native build for Windows x86_64

```sh
make win64-release
```
> [!NOTE]
> A Fedora container image with Mingw toolchain will be created and used for the build. You need ~2.5G free space for it.
> An archive with a PE executable (3DViewer.exe) and all needed DLLs will be created in build directory on the host
> You might need to turn off Defender if you get DLL errors when running the app  

## Usage

```sh
./3DViewer
```
> [!NOTE]
> By default config is saved in $XDG_CONFIG_HOME/3DViewer.ini but config location can be overriden if launched with -c/--config flag

## Implemented features:
 - loading model mesh from Waveform .obj file
 - making 5 second GIF screencasts of the model
 - config with autosave for styling settings persistence
 - translating and scaling the model
 - orthographic / perspective projection toggle
 - lighting with flat shading or smooth shading (Phong)
 
