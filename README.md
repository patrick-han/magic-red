# Magic Red
Magic Red is a cross-platform 3D rendering engine built on top of the Diligent Core library.

# Requirements
This project requires from you:
- Vulkan SDK
- CMake 3.20 or later

All other dependencies are pulled as submodules and built from source.

# Build:

Initial clone:
```sh
git clone --recurse-submodules git@github.com:patrick-han/magic-red.git
```

If you forgot to fetch the submodules:
```sh
cd magic-red
git submodule update --init --recursive
```

Create build folder:
```sh
cd magic-red
mkdir build
```

### Windows:
It's recommended to use the CMake gui application from here.

### MacOS:
```
cmake -S . -B build -G "Xcode"
```
or if you're feeling spicy and don't like Xcode:
```
cmake -S . -B build -G "Unix Makefiles"
```