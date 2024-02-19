#include <Shader/Shader.h>
#include <string>
#include <fstream>
#include <Common/Log.h>
#include <vector>
// #include <iostream>

// #include <sstream>


bool load_shader_spirv_source_to_module(const std::string& shaderSpirvPath, VkDevice device, VkShaderModule& shaderModule) {
    // Open the file. With cursor at the end
    std::ifstream file(shaderSpirvPath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // Find what the size of the file is by looking up the location of the cursor
    // Because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)file.tellg();

    // Spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // Put file cursor at beginning
    file.seekg(0);

    // Load the entire file into the buffer
    file.read((char*)buffer.data(), fileSize);

    // Now that the file is loaded into the buffer, we can close it
    file.close();

    VkShaderModuleCreateInfo shaderCreateInfo = { 
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            nullptr,
            VkShaderModuleCreateFlags(),
            std::distance(buffer.begin(), buffer.end()) * sizeof(uint32_t),
            buffer.data() 
    };
    if (vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        MRCERR("Could not compile shader: " << shaderSpirvPath);
        return false;
    };
    return true;
}

// std::string load_shader_source_to_string(std::string const& shaderPath) {
//     std::string shaderSource;
//     std::ifstream shaderFile;
//     // Ensure ifstream objects can throw exceptions:
//     shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
//     try
//     {
//         // open files
//         shaderFile.open(shaderPath.c_str());
//         std::stringstream shaderStream;
//         // read file's buffer contents into stream
//         shaderStream << shaderFile.rdbuf();
//         // close file handlers
//         shaderFile.close();
//         // convert stream into string
//         shaderSource = shaderStream.str();
//     }
//     catch (std::ifstream::failure e)
//     {
//         MRCERR("Shader file not successfully read!");
//     }

//     return shaderSource;
// }

// void compile_shader(VkDevice device, VkShaderModule& shaderModule, std::string shaderSource, shaderc_shader_kind shaderKind, const char *inputFileName) {
//     // TODO: Not sure what the cost of doing this compiler init everytime is, fine for now
//     shaderc::Compiler compiler;
//     shaderc::CompileOptions options;
//     options.SetOptimizationLevel(shaderc_optimization_level_performance);

//     //A compilation result for a SPIR-V binary module, which is an array of uint32_t words.
//     shaderc::SpvCompilationResult shaderModuleCompile = compiler.CompileGlslToSpv(shaderSource, shaderKind, inputFileName, options);
//     if (shaderModuleCompile.GetCompilationStatus() != shaderc_compilation_status_success) {
//         MRCERR(shaderModuleCompile.GetErrorMessage());
//     }
//     std::vector<uint32_t> shaderCode = { shaderModuleCompile.cbegin(), shaderModuleCompile.cend() };

//     VkShaderModuleCreateInfo shaderCreateInfo = { 
//             VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
//             nullptr,
//             VkShaderModuleCreateFlags(),
//             std::distance(shaderCode.begin(), shaderCode.end()) * sizeof(uint32_t),
//             shaderCode.data() 
//     };
//     vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);
// }