/*
 * GL implementation
 */

#include <open-sea/GL.h>
#include <open-sea/Log.h>

#include <imgui.h>
#if not(defined(GLM_ENABLE_EXPERIMENTAL))
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>

namespace open_sea::gl {
    // Loggers
    log::severity_logger lg = log::get_logger("OpenGL");
    log::severity_logger shaderLG = log::get_logger("OpenGL Shaders");

//--- start ShaderProgram implementation
    // Initailize counters
    uint ShaderProgram::programCount = 0;
    uint ShaderProgram::vertexCount = 0;
    uint ShaderProgram::geometryCount = 0;
    uint ShaderProgram::fragmentCount = 0;
    uint ShaderProgram::tessConCount = 0;
    uint ShaderProgram::tessEvalCount = 0;

    //! String to use when Info Log is empty
    constexpr std::initializer_list<GLchar> unknown_info = {'u','n','k','n','o','w','n'};

    /**
     * \brief Construct an empty shader program
     * Create a shader program with no shaders attached.
     */
    ShaderProgram::ShaderProgram() {
        programID = glCreateProgram();
        programCount++;
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
     * \brief Attach a tessellation control shader from a file
     * Read, compile and attach a tessellation control shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachTessConFile(const std::string& path) {
        return attachTessConSource(read_file(path));
    }

    /**
     * \brief Attach a tessellation evaluation shader from a file
     * Read, compile and attach a tessellation evaluation shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachTessEvalFile(const std::string& path) {
        return attachTessEvalSource(read_file(path));
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
        if (vertexShader == 0) {
            vertexShader = glCreateShader(GL_VERTEX_SHADER);
            vertexCount++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(vertexShader, 1, &source, nullptr);

        // Compile
        glCompileShader(vertexShader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(vertexShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(vertexShader);
            vertexCount--;

            return false;
        }

        // Attach
        glAttachShader(programID, vertexShader);

        linked = false;
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
        if (geometryShader == 0) {
            geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
            geometryCount++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(geometryShader, 1, &source, nullptr);

        // Compile
        glCompileShader(geometryShader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(geometryShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(geometryShader);
            geometryCount--;

            return false;
        }

        // Attach
        glAttachShader(programID, geometryShader);

        linked = false;
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
        if (fragmentShader == 0) {
            fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            fragmentCount++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(fragmentShader, 1, &source, nullptr);

        // Compile
        glCompileShader(fragmentShader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(fragmentShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(fragmentShader);
            fragmentCount--;

            return false;
        }

        // Attach
        glAttachShader(programID, fragmentShader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a tessellation control shader
     * Compile and attach a tessellation control shader from the provided source.
     * Attachment is skipped if context version is less than 4.0 (still returns \c true, just doesn't apply the shader).
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachTessConSource(const std::string& src) {
        // Skip if context version too low
        int major;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        if (major < 4) {
            log::log(shaderLG, log::warning, "Tessellation control shader skipped, because context version is too low");
        }

        // Create the shader if needed
        if (tessConShader == 0) {
            tessConShader = glCreateShader(GL_TESS_CONTROL_SHADER);
            tessConCount++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(tessConShader, 1, &source, nullptr);

        // Compile
        glCompileShader(tessConShader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(tessConShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(tessConShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(tessConShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(tessConShader);
            tessConCount--;

            return false;
        }

        // Attach
        glAttachShader(programID, tessConShader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a tessellation evaluation shader
     * Compile and attach a tessellation evaluation shader from the provided source.
     * Attachment is skipped if context version is less than 4.0 (still returns \c true, just doesn't apply the shader).
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attachTessEvalSource(const std::string& src) {
        // Skip if context version too low
        int major;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        if (major < 4) {
            log::log(shaderLG, log::warning, "Tessellation evaluation shader skipped, because context version is too low");
        }

        // Create the shader if needed
        if (tessEvalShader == 0) {
            tessEvalShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            tessEvalCount++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(tessEvalShader, 1, &source, nullptr);

        // Compile
        glCompileShader(tessEvalShader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(tessEvalShader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(tessEvalShader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetShaderInfoLog(tessEvalShader, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(tessEvalShader);
            tessEvalCount--;

            return false;
        }

        // Attach
        glAttachShader(programID, tessEvalShader);

        linked = false;
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
            vertexCount--;
            linked = false;
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
            geometryCount--;
            linked = false;
        }
    }

    /**
     * \brief Detach the fragment shader
     * Detach and delete the fragment shader
     */
    void ShaderProgram::detachFragment() {
        if (fragmentShader) {
            glDetachShader(programID, fragmentShader);
            glDeleteShader(fragmentShader);
            fragmentShader = 0;
            fragmentCount--;
            linked = false;
        }
    }

    /**
     * \brief Detach the tessellation control shader
     * Detach and delete the tessellation control shader
     */
    void ShaderProgram::detachTessCon() {
        if (tessConShader) {
            glDetachShader(programID, tessConShader);
            glDeleteShader(tessConShader);
            tessConShader = 0;
            fragmentCount--;
            linked = false;
        }
    }

    /**
     * \brief Detach the tessellation evaluation shader
     * Detach and delete the tessellation evaluation shader
     */
    void ShaderProgram::detachTessEval() {
        if (tessEvalShader) {
            glDetachShader(programID, tessEvalShader);
            glDeleteShader(tessEvalShader);
            tessEvalShader = 0;
            fragmentCount--;
            linked = false;
        }
    }

    /**
     * \brief Link the shader program
     *
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::link() {
        // Skip if already linked
        if (linked)
            return true;

        glLinkProgram(programID);
        GLint status;
        glGetProgramiv(programID, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint maxLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> info(maxLength);
            glGetProgramInfoLog(programID, maxLength, nullptr, &info[0]);

            if (info.empty())
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Program linking failed: ").append(&info[0]));
            return false;
        }

        linked = true;
        return true;
    }

    /**
     * \brief Whether the program has been linked since the last shader change
     *
     * \return Whether the program has been linked
     */
    bool ShaderProgram::isLinked() const {
        return linked;
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
                info = unknown_info;

            log::log(shaderLG, log::error, std::string("Program validation failed: ").append(&info[0]));
            return false;
        }
        return true;
    }

    /**
     * \brief Start using this shader program
     */
    void ShaderProgram::use() const {
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
        // Destroy all the shaders
        detachVertex();
        detachGeometry();
        detachFragment();
        detachTessCon();
        detachTessEval();

        // Destroy the program
        glDeleteProgram(programID);
        programID = 0;
        programCount--;
    }

    /**
     * \brief Whether the programs are equal
     * Shader programs are equal if they have the same program ID.
     *
     * \param rhs Right hand side
     * \return \c true when equal, \c false otherwise
     */
    bool ShaderProgram::operator==(const ShaderProgram &rhs) const {
        return programID == rhs.programID;
    }

    /**
     * \sa operator==
     *
     * \param rhs Right hand side
     * \return \c false when equal, \c true otherwise
     */
    bool ShaderProgram::operator!=(const ShaderProgram &rhs) const {
        return !(rhs == *this);
    }

    /**
     * \brief Show the ImGui debug widget
     */
    void ShaderProgram::debugWidget() {
        ImGui::Text("Shader programs: %d", programCount);
        ImGui::Text("Vertex shaders: %d", vertexCount);
        ImGui::Text("Geometry shaders: %d", geometryCount);
        ImGui::Text("Fragment shaders: %d", fragmentCount);
        ImGui::Text("Tessellation control shaders: %d", tessConCount);
        ImGui::Text("Tessellation evaluation shaders: %d", tessEvalCount);
    }
//--- end ShaderProgram implementation

//--- start Camera implementation

    Camera::Camera(const glm::vec3 &position, const glm::quat &orientation, const glm::vec2 &size, float near,
                   float far) : near(near), far(far) {
        this->position = glm::vec3(position);
        this->orientation = glm::quat(orientation);
        this->size = glm::vec2(size);

        viewMatrix = glm::mat4();
        projMatrix = glm::mat4();
        projViewMatrix = glm::mat4();

        recalculateView = true;
        recalculateProj = true;
    }

    void Camera::setPosition(const glm::vec3& newValue) {
        position = newValue;
        recalculateView = true;
    }

    glm::vec3 Camera::getPosition() const {
        return glm::vec3(position);
    }

    void Camera::setRotation(const glm::quat& newValue) {
        orientation = newValue;
        recalculateView = true;
    }

    glm::quat Camera::getRotation() const {
        return glm::quat(orientation);
    }

    /**
     * \brief Translate the camera
     *
     * \param d Translation vector
     */
    void Camera::translate(const glm::vec3& d) {
        position += d;
    }

    /**
     * \brief Rotate the camera
     *
     * \param d Rotation quaternion
     */
    void Camera::rotate(const glm::quat& d) {
        orientation *= d;
    }

    void Camera::setSize(const glm::vec2& newValue) {
        size = newValue;
        recalculateProj = true;
    }

    glm::vec2 Camera::getSize() const {
        return glm::vec2(size);
    }

    void Camera::setNear(float newValue) {
        near = newValue;
        recalculateProj = true;
    }

    float Camera::getNear() const {
        return near;
    }

    void Camera::setFar(float newValue) {
        far = newValue;
        recalculateProj = true;
    }

    float Camera::getFar() const {
        return far;
    }

//--- end Camera implementation

//--- start OrthographicCamera implementation

    /**
     * \brief Construct the camera from all relevant data
     * Construct the orthographic camera from all data needed to calculate the matrices.
     * 
     * \param position Position of the camera
     * \param orientation Orientation of the camera
     * \param size Size of the viewport
     * \param near Near clipping plane
     * \param far Far clipping plane
     */
    OrthographicCamera::OrthographicCamera(const glm::vec3& position, const glm::quat& orientation, const glm::vec2& size,
                                           float near, float far) : Camera(position, orientation, size, near, far) {}

    glm::mat4 OrthographicCamera::getProjViewMatrix() {
        if (recalculateView) {
            viewMatrix = glm::translate(position) * glm::toMat4(orientation);
        }

        if (recalculateProj) {
            // Projection assumes origin in bottom left
            projMatrix = glm::ortho(
                    0.0f, size.x,
                    0.0f, size.y,
                    near, far);
        }

        if (recalculateView || recalculateProj) {
            projViewMatrix = projMatrix;
            projViewMatrix *= viewMatrix;
        }

        return projViewMatrix;
    }

//--- end OrthographicCamera implementation

    /**
     * \brief Show the ImGui debug window
     */
    void debug_window() {
        ImGui::Begin("OpenGL");

        if (ImGui::CollapsingHeader("Shaders")) {
            ShaderProgram::debugWidget();
        }

        ImGui::End();
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
        if (severity >= GL_DEBUG_SEVERITY_LOW) {
            // Use special logger for shader errors
            if (source == GL_DEBUG_SOURCE_SHADER_COMPILER) {
                log::log(shaderLG, log::error, message);
                return;
            }

            // Otherwise just log the message normally
            log::log(lg, log::error, message);
        }
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
