This project requires:
- VulkanSDK
- GLFW3
- GLM (can be installed with the Vulkan SDK)

Setup:
```sh
source ~/path/to/VulkanSDK/version/setup-env.sh # Setup env variables needed by cmake to find Vulkan
mkdir build
cd build
cmake ..
make
```

The above `source` is of course not valid on Windows, so you may need to do this instead:
```sh
export VULKAN_SDK=path/to/VulkanSDK/version/ # Git Bash
$Env:VULKAN_SDK = 'path/to/VulkanSDK/version/' # Powershell
```