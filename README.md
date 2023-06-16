This project requires:
- VulkanSDK
- GLFW3
- GLM (can be installed with the Vulkan SDK)

Setup:
```sh
mkdir build
cd build
cmake ..
make
```

WIP
```sh
source ~/path/to/VulkanSDK/version/setup-env.sh # Setup env variables needed by cmake to find Vulkan
export VULKAN_SDK=path/to/VulkanSDK/version/ # Git Bash
$Env:VULKAN_SDK = 'path/to/VulkanSDK/version/' # Powershell
```