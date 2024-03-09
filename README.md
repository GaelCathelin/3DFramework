# 3DFramework

A minimal application framework in C for easy 3D rendering prototyping, based on [SDL2](https://github.com/libsdl-org/SDL), [NVRHI](https://github.com/GaelCathelin/nvrhi), [ImGui](https://github.com/ocornut/imgui) and [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap).


# Usage

For now, only the Win64 binaries of the framework are provided (along with the headers). Source code and Linux support should come gradually, no ETA.

Some sample applications are provided to show some basic usages of the framework, and an example of a relatively generic but Windows specific Makefile that can also compile your GLSL shaders to SPIR-V blobs.

To use it, you will need a [64 bit Mingw](https://winlibs.com) environment (MSVCRT, POSIX threads, [direct link to 13.2.0](https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-17.0.6-11.0.1-msvcrt-r5/winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64msvcrt-11.0.1-r5.zip)), with its 'bin' subfolder in your 'PATH' environment variable.

To build a sample with the provided Makefile, run the 'mingw32-make' command at the root of the sample. To also run it, and parallelize the build, run 'mingw32-make run -j'.