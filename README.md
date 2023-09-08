This project requires from you:
- Vulkan SDK
- CMake 3.20 or later

All other dependencies are pulled as submodules and built from source

Initial clone:
```sh
git clone --recurse-submodules git@github.com:patrick-han/magic-red.git
```

If you forgot to fetch the submodules:
```sh
cd magic-red
git submodule update --init --recursive
```

Build:
```sh
cd magic-red
mkdir build
cd build
cmake ..
make debug # or release
```