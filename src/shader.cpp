#include "shader.h"
#include "utils.h"
#include <iostream>
#include <fstream>


std::string load_shader_source_to_string(std::string const& shaderPath) {
    std::string shaderCode;
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
        shaderCode = shaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        MRCERR("Shader file not successfully read!");
    }

    return shaderCode;
}