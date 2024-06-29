# Magic Red
Magic Red is a cross-platform 3D rendering engine built on top of Vulkan.

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
It's recommended to use the CMake gui application from here. After configuring and generating, open the solution and build the `magic-red` project.

### MacOS:
```
cmake -S . -B build -G "Xcode"
```
Then open the Xcode project in the `build` folder and build the magic-red target.
\
\
Or if you're feeling spicy and don't like Xcode:
```
cmake -S . -B build -G "Unix Makefiles"
cd build
make -j `sysctl -n hw.ncpu` magic-red
```

# Vulkan extensions used:
- Dynamic rendering
- Buffer device address
- Scalar block layout

# Immediate Roadmap:
- [x] Mesh loading and drawing
- [ ] Diffuse texture loading and drawing
- [ ] Blinn-Phong lighting
- [ ] Gamma-correction
- [ ] Directional light shadowmaps
- [ ] Point light shadowmaps
- [ ] Other texture maps (specular, normal, etc.)
- [ ] Bloom in compute
- [ ] SSAO
- [ ] PBR
- [ ] Forward+


# Non-essential for now:
- [ ] HDR
- [ ] Hybrid RT
- [ ] DoF