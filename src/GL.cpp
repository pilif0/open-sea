/*
 * GL implementation
 */

#include <open-sea/GL.h>

#include <fstream>
#include <sstream>

namespace open_sea::gl {

    /**
     * \brief Construct an empty shader program
     * Create a shader program with no shaders attached.
     */
    ShaderProgram::ShaderProgram() {
        programID = glCreateProgram();
    }

    /**
     * \brief Read a file into a string
     *
     * \param path Path to the file
     * \return Contents of the file in one string
     */
    // Credit: https://stackoverflow.com/a/116220
    std::string read_file(const std::string& path) {
        std::ifstream stream(path);
        std::stringstream result;
        result << stream.rdbuf();
        return result.str();
    }

    /**
     * \brief Attach a vertex shader from a file
     * Read, compile and attach a vertex shader from the source file at the specified path.
     *
     * \param path Path to the source file
     */
    void ShaderProgram::attachVertexFile(const std::string& path) {
        attachVertexSource(read_file(path));
    }

    /**
     * \brief Attach a geometry shader from a file
     * Read, compile and attach a geometry shader from the source file at the specified path.
     *
     * \param path Path to the source file
     */
    void ShaderProgram::attachGeometryFile(const std::string& path) {
        attachGeometrySource(read_file(path));
    }

    /**
     * \brief Attach a fragment shader from a file
     * Read, compile and attach a fragment shader from the source file at the specified path.
     *
     * \param path Path to the source file
     */
    void ShaderProgram::attachFragmentFile(const std::string& path) {
        attachFragmentSource(read_file(path));
    }

    /**
     * \brief Attach a vertex shader
     * Compile and attach a vertex shader from the provided source.
     *
     * \param src Shader source
     */
    void ShaderProgram::attachVertexSource(const std::string& src) {
        // Create the shader if needed
        if (vertexShader == 0)
            vertexShader = glCreateShader(GL_VERTEX_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(vertexShader, 1, &source, nullptr);

        // Compile
        glCompileShader(vertexShader);

        // Attach
        glAttachShader(programID, vertexShader);
    }

    /**
     * \brief Attach a geometry shader
     * Compile and attach a geometry shader from the provided source.
     *
     * \param src Shader source
     */
    void ShaderProgram::attachGeometrySource(const std::string& src) {
        // Create the shader if needed
        if (geometryShader == 0)
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(geometryShader, 1, &source, nullptr);

        // Compile
        glCompileShader(geometryShader);

        // Attach
        glAttachShader(programID, geometryShader);
    }

    /**
     * \brief Attach a fragment shader
     * Compile and attach a fragment shader from the provided source.
     *
     * \param src Shader source
     */
    void ShaderProgram::attachFragmentSource(const std::string& src) {
        // Create the shader if needed
        if (fragmentShader == 0)
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(fragmentShader, 1, &source, nullptr);

        // Compile
        glCompileShader(fragmentShader);

        // Attach
        glAttachShader(programID, fragmentShader);
    }

    /**
     * \brief Link the shader program
     */
    void ShaderProgram::link() {
        glLinkProgram(programID);
    }

    /**
     * \brief Start using this shader program
     */
    void ShaderProgram::use() {
        glUseProgram(programID);
    }

    /**
     * \brief Stop using any shader program
     * Stop using any shader program, i.e. set used program to 0.
     */
    void ShaderProgram::unset() {
        glUseProgram(0);
    }

    /**
     * \brief Destroy the shaders and the program
     */
    ShaderProgram::~ShaderProgram() {
        // Destroy vertex shader
        if (vertexShader) {
            glDetachShader(programID, vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }

        // Destroy geometry shader
        if (geometryShader) {
            glDetachShader(programID, geometryShader);
            glDeleteShader(geometryShader);
            geometryShader = 0;
        }

        // Destroy fragment shader
        if (fragmentShader) {
            glDetachShader(programID, fragmentShader);
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }

        // Destroy the program
        glDeleteProgram(programID);
        programID = 0;
    }
}
