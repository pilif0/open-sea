/*
 * GL implementation
 */

#include <open-sea/GL.h>

#include <fstream>
#include <sstream>

namespace open_sea::gl {
    log::severity_logger lg = log::get_logger("OpenGL");
    log::severity_logger shaderLG = log::get_logger("OpenGL Shaders");

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
        if (stream.fail()) {
            log::log(lg, log::info, std::string("Failed to read file ").append(path));
            return std::string();
        }
        std::stringstream result;
        result << stream.rdbuf();
        return result.str();
    }

    /**
     * \brief Attach a vertex shader from a file
     * Read, compile and attach a vertex shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachVertexFile(const std::string& path) {
        return attachVertexSource(read_file(path));
    }

    /**
     * \brief Attach a geometry shader from a file
     * Read, compile and attach a geometry shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachGeometryFile(const std::string& path) {
        return attachGeometrySource(read_file(path));
    }

    /**
     * \brief Attach a fragment shader from a file
     * Read, compile and attach a fragment shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachFragmentFile(const std::string& path) {
        return attachFragmentSource(read_file(path));
    }

    /**
     * \brief Attach a vertex shader
     * Compile and attach a vertex shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachVertexSource(const std::string& src) {
        // Create the shader if needed
        if (vertexShader == 0)
            vertexShader = glCreateShader(GL_VERTEX_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(vertexShader, 1, &source, nullptr);

        // Compile
        glCompileShader(vertexShader);
        GLint status;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = {'u','n','k','n','o','w','n'};

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));
            return false;
        }

        // Attach
        glAttachShader(programID, vertexShader);

        return true;
    }

    /**
     * \brief Attach a geometry shader
     * Compile and attach a geometry shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachGeometrySource(const std::string& src) {
        // Create the shader if needed
        if (geometryShader == 0)
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(geometryShader, 1, &source, nullptr);

        // Compile
        glCompileShader(geometryShader);
        GLint status;
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(geometryShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = {'u','n','k','n','o','w','n'};

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));
            return false;
        }

        // Attach
        glAttachShader(programID, geometryShader);

        return true;
    }

    /**
     * \brief Attach a fragment shader
     * Compile and attach a fragment shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachFragmentSource(const std::string& src) {
        // Create the shader if needed
        if (fragmentShader == 0)
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        // Set the source
        const char* source = src.c_str();
        glShaderSource(fragmentShader, 1, &source, nullptr);

        // Compile
        glCompileShader(fragmentShader);
        GLint status;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = {'u','n','k','n','o','w','n'};

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));
            return false;
        }

        // Attach
        glAttachShader(programID, fragmentShader);

        return true;
    }

    /**
     * \brief Detach the vertex shader
     * Detach and delete the vertex shader
     */
    void ShaderProgram::detachVertex() {
        if (vertexShader) {
            glDetachShader(programID, vertexShader);
            glDeleteShader(vertexShader);
            vertexShader = 0;
        }
    }

    /**
     * \brief Detach the geometry shader
     * Detach and delete the geometry shader
     */
    void ShaderProgram::detachGeometry() {
        if (geometryShader) {
            glDetachShader(programID, geometryShader);
            glDeleteShader(geometryShader);
            geometryShader = 0;
        }
    }

    /**
     * \biref Detach the fragment shader
     * Detach and delete the fragment shader
     */
    void ShaderProgram::detachFragment() {
        if (fragmentShader) {
            glDetachShader(programID, fragmentShader);
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
        }
    }

    /**
     * \brief Link the shader program
     *
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::link() {
        glLinkProgram(programID);
        GLint status;
        glGetProgramiv(programID, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetProgramInfoLog(programID, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = {'u','n','k','n','o','w','n'};

            log::log(shaderLG, log::error, std::string("Program linking failed: ").append(&info[0]));
            return false;
        }

        return true;
    }

    /**
     * \brief Validate the shader program
     *
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::validate() {
        glValidateProgram(programID);
        GLint status;
        glGetProgramiv(programID, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetProgramInfoLog(programID, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = {'u','n','k','n','o','w','n'};

            log::log(shaderLG, log::error, std::string("Program validation failed: ").append(&info[0]));
            return false;
        }
        return true;
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
     * \brief Get the location of a uniform in this shader program
     *
     * \param name Name of the uniform
     * \return Location of the uniform or \c -1 if the parameter is not a valid name or the uniform is not present
     */
    GLint ShaderProgram::getUniformLocation(const std::string &name) {
        return glGetUniformLocation(programID, name.c_str());
    }

    /**
     * \brief Get the location of an attribute in this shader program
     *
     * \param name Name of the attribute
     * \return Location of the attribute or \c -1 if the parameter is not a valid name or the attribute is not present
     */
    GLint ShaderProgram::getAttributeLocation(const std::string &name) {
        return glGetAttribLocation(programID, name.c_str());
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

    /**
     * \brief OpenGL error callback
     *
     * \param source Source that produced the message
     * \param type Type of the message
     * \param id ID
     * \param severity Message severity
     * \param length Message length
     * \param message Message contents
     * \param userParam User parameter
     */
    void error_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                        GLsizei length, const GLchar* message, const void* userParam) {
        // Log only errors and performance issues
        //if (severity >= GL_DEBUG_SEVERITY_LOW) {
            // Use special logger for shader errors
            if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
                log::log(shaderLG, log::error, message);
                return;
            }

            // Otherwise just log the message normally
            log::log(lg, log::error, message);
        //}
    }

    /**
     * \brief Start logging OpenGL errors into the log
     */
    void log_errors() {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback((GLDEBUGPROC) error_callback, nullptr);
        log::log(lg, log::info, "OpenGL error logging started");
    }
}
