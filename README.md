# Magic Red
Magic Red is a cross-platform 3D rendering engine built on top of Vulkan.

# Requirements
This project requires from you:
- The latest Vulkan SDK
- CMake 3.20 or later

All other dependencies are pulled as submodules and built from source.

# Build:

Initial clone:
```sh
git clone --recurse-submodules git@github.com:patrick-han/magic-red.git
```

If you forget to fetch the submodules:
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

### Linux:
Will probably work with a few tweaks, there are always some compiler quirks.

# Vulkan extensions used:
- VK_KHR_dynamic_rendering
- VK_KHR_buffer_device_address
- VK_EXT_scalar_block_layout
- VK_EXT_descriptor_indexing

# Rendering Roadmap
- [x] Static mesh loading
- [x] Simple Phong shading
- [ ] Blinn-Phong lighting
- [ ] Bindless textures w/ PBR Materials
- [ ] Normal mapping
- [ ] HDR
- [ ] Shadowmaps
- [ ] Deferred architecture
- [ ] PBR lighting model
- [ ] Bloom
- [ ] SSAO
- [ ] RT path for GI, shadows, and reflections
- [ ] Skinned meshes and animation

# Engine Roadmap
- [ ] Job system
- [ ] Simple "ECS"
- [ ] Asset cooker (including texture compression)
- [ ] Scene saving/loading
- [ ] Physics
- [ ] Developer console