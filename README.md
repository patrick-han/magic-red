This project requires from you:
- Vulkan SDK
- CMake 3.20 or later

Initial clone:
```sh
git clone --recurse-submodules git@github.com:patrick-han/magic-red.git
```

If you forgot to fetch the submodules:
```sh
cd magic-red
git submodule update --init --recursive
```


Build setup:
```sh
cd magic-red
mkdir build
cd build
cmake ..
make debug # or release
```

WIP
```sh
source ~/path/to/VulkanSDK/version/setup-env.sh # Setup env variables needed by cmake to find Vulkan
export VULKAN_SDK=path/to/VulkanSDK/version/ # Git Bash
$Env:VULKAN_SDK = 'path/to/VulkanSDK/version/' # Powershell
```