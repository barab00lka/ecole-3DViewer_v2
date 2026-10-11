# ecole-3DViewer_v2
A simple GTK4 .obj model viewer. WiP.

**implemented features:**
 - making 5 second GIF screencasts of the model
 - configuration file in $XDG_CONFIG_HOME/3DViewer.ini with autosave for styling settings persistence
 - translating and scaling the model
 - orthographic / perspective projection toggle
 
## Building on Linux
**First you need to install dependencies:**
     * a C++20 compiler toolchain (tested with gcc and clang)
     * cmake 
     * pkgconf
     * gtkmm-4.0 
     * doxygen
**To build and run the app just run:**
    ``` ./build.sh ```
 - 
**for additional build options:**
    ``` ./build.sh --help ```
