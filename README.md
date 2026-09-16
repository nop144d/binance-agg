# Build

Requires CMake >= 3.25 and Conan >= 2.0. Once per machine: `conan profile detect`.

## Linux

```bash
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Release
cmake --preset linux-release
cmake --build --preset linux-release -j
ctest --preset linux-release
```

Binaries: `build/linux/release/bin/`.

For a debug build use `-s build_type=Debug` and the `linux-debug` preset.

## Windows

Debug and Release need separate dependency installs.

```bat
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Release
conan install . --build=missing -s compiler.cppstd=23 -s build_type=Debug
cmake --preset windows-msvc
cmake --build --preset windows-release
ctest --preset windows-release
```

Binaries: `build\windows\bin\release\`.

`cmake --preset windows-msvc` also generates `build\windows\binance-agg.slnx` for Visual Studio.
