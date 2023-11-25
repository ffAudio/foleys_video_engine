This repository contains CMake scripts wrapping FFmpeg's horrendous build system into something sane and usable.

On Mac, clang and yasm are required.

On MacOS, universal binaries are supported. (Technically, this repo's scripts build FFmpeg twice, once for arm64 and once for x86-64.) This can be turned off by passing `-D FFMPEG_MAC_UNIVERSAL_BINARY=OFF` to the CMake configure step.

To use FFmpeg from within a CMake project, run `cmake --install` within this directory, and then from the consuming project you can do:
```cmake
find_package (ffmpeg)

target_link_libraries (yourTarget PUBLIC ffmpeg::ffmpeg)
```
and it should work out of the box.

Sometimes repeated builds can fail spuriously; if in doubt, try deleting your builds directory and reconfiguring.

TO DO: Windows - seems to require MinGW-w64, according to https://www.ffmpeg.org/platform.html#toc-Native-Windows-compilation-using-MinGW-or-MinGW_002dw64
TO DO: iOS and Android
