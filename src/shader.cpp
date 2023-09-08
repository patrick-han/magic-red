#include "shader.h"
#include "utils.h"
#include <iostream>
#include <fstream>


std::string load_shader_source_to_string(std::string const& shaderPath) {
    std::string shaderSource;
    std::ifstream shaderFile;
    // Ensure ifstream objects can throw exceptions:
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        shaderFile.open(shaderPath.c_str());
        std::stringstream shaderStream;
        // read file's buffer contents into stream
        shaderStream << shaderFile.rdbuf();
        // close file handlers
        shaderFile.close();
        // convert stream into string
        shaderSource = shaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        MRCERR("Shader file not successfully read!");
    }

    return shaderSource;
}

std::vector<uint32_t> compile_shader(std::string shaderSource, shaderc_shader_kind shaderKind, const char *inputFileName) {
    // TODO: Not sure what the cost of doing this compiler init everytime is, fine for now
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    //A compilation result for a SPIR-V binary module, which is an array of uint32_t words.
    shaderc::SpvCompilationResult shaderModuleCompile = compiler.CompileGlslToSpv(shaderSource, shaderKind, inputFileName, options);
    if (shaderModuleCompile.GetCompilationStatus() != shaderc_compilation_status_success) {
        MRCERR(shaderModuleCompile.GetErrorMessage());
        exit(0); // TODO: Do something else instead?
    }
    std::vector<uint32_t> shaderCode = { shaderModuleCompile.cbegin(), shaderModuleCompile.cend() };

    return shaderCode;
}