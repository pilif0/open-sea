/** \file GL.cpp
 * OpenGL related implementation
 *
 * \author Filip Smola
 */

#include <open-sea/GL.h>
#include <open-sea/Log.h>
#include <open-sea/Debug.h>
#include <open-sea/ImGui.h>

#if not(defined(GLM_ENABLE_EXPERIMENTAL))
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>

namespace open_sea::gl {
    // Loggers
    //! Module logger
    log::severity_logger lg = log::get_logger("OpenGL");
    //! Shader specific logger
    log::severity_logger shader_lg = log::get_logger("OpenGL Shaders");

//--- start ShaderProgram implementation
    // Initailize counters
    uint ShaderProgram::program_count = 0;
    uint ShaderProgram::vertex_count = 0;
    uint ShaderProgram::geometry_count = 0;
    uint ShaderProgram::fragment_count = 0;
    uint ShaderProgram::tess_con_count = 0;
    uint ShaderProgram::tess_eval_count = 0;

    //! String to use when Info Log is empty
    constexpr std::initializer_list<GLchar> unknown_info = {'u','n','k','n','o','w','n'};

    /**
     * \brief Construct an empty shader program
     *
     * Create a shader program with no shaders attached.
     */
    ShaderProgram::ShaderProgram() {
        program_id = glCreateProgram();
        program_count++;
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
     *
     * Read, compile and attach a vertex shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_vertex_file(const std::string &path) {
        return attach_vertex_source(read_file(path));
    }

    /**
     * \brief Attach a geometry shader from a file
     *
     * Read, compile and attach a geometry shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_geometry_file(const std::string &path) {
        return attach_geometry_source(read_file(path));
    }

    /**
     * \brief Attach a fragment shader from a file
     *
     * Read, compile and attach a fragment shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_fragment_file(const std::string &path) {
        return attach_fragment_source(read_file(path));
    }

    /**
     * \brief Attach a tessellation control shader from a file
     *
     * Read, compile and attach a tessellation control shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_tess_con_file(const std::string &path) {
        return attach_tess_con_source(read_file(path));
    }

    /**
     * \brief Attach a tessellation evaluation shader from a file
     *
     * Read, compile and attach a tessellation evaluation shader from the source file at the specified path.
     *
     * \param path Path to the source file
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_tess_eval_file(const std::string &path) {
        return attach_tess_eval_source(read_file(path));
    }

    /**
     * \brief Attach a vertex shader
     *
     * Compile and attach a vertex shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_vertex_source(const std::string &src) {
        // Create the shader if needed
        if (vertex_shader == 0) {
            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            vertex_count++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(vertex_shader, 1, &source, nullptr);

        // Compile
        glCompileShader(vertex_shader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetShaderInfoLog(vertex_shader, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(vertex_shader);
            vertex_count--;

            return false;
        }

        // Attach
        glAttachShader(program_id, vertex_shader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a geometry shader
     *
     * Compile and attach a geometry shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_geometry_source(const std::string &src) {
        // Create the shader if needed
        if (geometry_shader == 0) {
            geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);
            geometry_count++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(geometry_shader, 1, &source, nullptr);

        // Compile
        glCompileShader(geometry_shader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(geometry_shader, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetShaderInfoLog(geometry_shader, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(geometry_shader);
            geometry_count--;

            return false;
        }

        // Attach
        glAttachShader(program_id, geometry_shader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a fragment shader
     *
     * Compile and attach a fragment shader from the provided source.
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_fragment_source(const std::string &src) {
        // Create the shader if needed
        if (fragment_shader == 0) {
            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
            fragment_count++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(fragment_shader, 1, &source, nullptr);

        // Compile
        glCompileShader(fragment_shader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetShaderInfoLog(fragment_shader, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(fragment_shader);
            fragment_count--;

            return false;
        }

        // Attach
        glAttachShader(program_id, fragment_shader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a tessellation control shader
     *
     * Compile and attach a tessellation control shader from the provided source.
     * Attachment is skipped if context version is less than 4.0 (still returns \c true, just doesn't apply the shader).
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_tess_con_source(const std::string &src) {
        // Skip if context version too low
        int major;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        if (major < 4) {
            log::log(shader_lg, log::warning, "Tessellation control shader skipped, because context version is too low");
        }

        // Create the shader if needed
        if (tess_con_shader == 0) {
            tess_con_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
            tess_con_count++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(tess_con_shader, 1, &source, nullptr);

        // Compile
        glCompileShader(tess_con_shader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(tess_con_shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(tess_con_shader, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetShaderInfoLog(tess_con_shader, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(tess_con_shader);
            tess_con_count--;

            return false;
        }

        // Attach
        glAttachShader(program_id, tess_con_shader);

        linked = false;
        return true;
    }

    /**
     * \brief Attach a tessellation evaluation shader
     *
     * Compile and attach a tessellation evaluation shader from the provided source.
     * Attachment is skipped if context version is less than 4.0 (still returns \c true, just doesn't apply the shader).
     *
     * \param src Shader source
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::attach_tess_eval_source(const std::string &src) {
        // Skip if context version too low
        int major;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        if (major < 4) {
            log::log(shader_lg, log::warning, "Tessellation evaluation shader skipped, because context version is too low");
        }

        // Create the shader if needed
        if (tess_eval_shader == 0) {
            tess_eval_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            tess_eval_count++;
        }

        // Set the source
        const char* source = src.c_str();
        glShaderSource(tess_eval_shader, 1, &source, nullptr);

        // Compile
        glCompileShader(tess_eval_shader);

        // Check for compile errors
        GLint status;
        glGetShaderiv(tess_eval_shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(tess_eval_shader, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetShaderInfoLog(tess_eval_shader, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Shader compilation failed: ").append(&info[0]));

            // Delete the shader
            glDeleteShader(tess_eval_shader);
            tess_eval_count--;

            return false;
        }

        // Attach
        glAttachShader(program_id, tess_eval_shader);

        linked = false;
        return true;
    }

    /**
     * \brief Get source code string from the vertex shader
     *
     * \param dest Destination buffer
     * \param size Size of the destination buffer
     * \return Length of the actual source code string
     */
    int ShaderProgram::get_vertex_source(char *dest, unsigned size) {
        GLsizei l;
        glGetShaderSource(vertex_shader, size, &l, dest);
        return l;
    }

    /**
     * \brief Get source code string from the geometry shader
     *
     * \param dest Destination buffer
     * \param size Size of the destination buffer
     * \return Length of the actual source code string
     */
    int ShaderProgram::get_geometry_source(char *dest, unsigned size) {
        GLsizei l;
        glGetShaderSource(geometry_shader, size, &l, dest);
        return l;
    }

    /**
     * \brief Get source code string from the fragment shader
     *
     * \param dest Destination buffer
     * \param size Size of the destination buffer
     * \return Length of the actual source code string
     */
    int ShaderProgram::get_fragment_source(char *dest, unsigned size) {
        GLsizei l;
        glGetShaderSource(fragment_shader, size, &l, dest);
        return l;
    }

    /**
     * \brief Get source code string from the tessellation control shader
     *
     * \param dest Destination buffer
     * \param size Size of the destination buffer
     * \return Length of the actual source code string
     */
    int ShaderProgram::get_tess_con_source(char *dest, unsigned size) {
        GLsizei l;
        glGetShaderSource(tess_con_shader, size, &l, dest);
        return l;
    }

    /**
     * \brief Get source code string from the tessellation evaluation shader
     *
     * \param dest Destination buffer
     * \param size Size of the destination buffer
     * \return Length of the actual source code string
     */
    int ShaderProgram::get_tess_eval_source(char *dest, unsigned size) {
        GLsizei l;
        glGetShaderSource(tess_eval_shader, size, &l, dest);
        return l;
    }

    /**
     * \brief Detach the vertex shader
     * Detach and delete the vertex shader
     */
    void ShaderProgram::detach_vertex() {
        if (vertex_shader) {
            glDetachShader(program_id, vertex_shader);
            glDeleteShader(vertex_shader);
            vertex_shader = 0;
            vertex_count--;
            linked = false;
        }
    }

    /**
     * \brief Detach the geometry shader
     * Detach and delete the geometry shader
     */
    void ShaderProgram::detach_geometry() {
        if (geometry_shader) {
            glDetachShader(program_id, geometry_shader);
            glDeleteShader(geometry_shader);
            geometry_shader = 0;
            geometry_count--;
            linked = false;
        }
    }

    /**
     * \brief Detach the fragment shader
     * Detach and delete the fragment shader
     */
    void ShaderProgram::detach_fragment() {
        if (fragment_shader) {
            glDetachShader(program_id, fragment_shader);
            glDeleteShader(fragment_shader);
            fragment_shader = 0;
            fragment_count--;
            linked = false;
        }
    }

    /**
     * \brief Detach the tessellation control shader
     * Detach and delete the tessellation control shader
     */
    void ShaderProgram::detach_tess_con() {
        if (tess_con_shader) {
            glDetachShader(program_id, tess_con_shader);
            glDeleteShader(tess_con_shader);
            tess_con_shader = 0;
            fragment_count--;
            linked = false;
        }
    }

    /**
     * \brief Detach the tessellation evaluation shader
     * Detach and delete the tessellation evaluation shader
     */
    void ShaderProgram::detach_tess_eval() {
        if (tess_eval_shader) {
            glDetachShader(program_id, tess_eval_shader);
            glDeleteShader(tess_eval_shader);
            tess_eval_shader = 0;
            fragment_count--;
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
        if (linked) {
            return true;
        }

        glLinkProgram(program_id);
        GLint status;
        glGetProgramiv(program_id, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetProgramInfoLog(program_id, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Program linking failed: ").append(&info[0]));
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
    bool ShaderProgram::is_linked() const {
        return linked;
    }

    /**
     * \brief Validate the shader program
     *
     * \return \c false on failure, \c true otherwise
     */
    bool ShaderProgram::validate() {
        glValidateProgram(program_id);
        GLint status;
        glGetProgramiv(program_id, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint max_length = 0;
            glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> info(max_length);
            glGetProgramInfoLog(program_id, max_length, nullptr, &info[0]);

            if (info.empty()) {
                info = unknown_info;
            }

            log::log(shader_lg, log::error, std::string("Program validation failed: ").append(&info[0]));
            return false;
        }
        return true;
    }

    /**
     * \brief Start using this shader program
     */
    void ShaderProgram::use() const {
        glUseProgram(program_id);
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
    GLint ShaderProgram::get_uniform_location(const std::string &name) const {
        return glGetUniformLocation(program_id, name.c_str());
    }

    /**
     * \brief Get the location of an attribute in this shader program
     *
     * \param name Name of the attribute
     * \return Location of the attribute or \c -1 if the parameter is not a valid name or the attribute is not present
     */
    GLint ShaderProgram::get_attribute_location(const std::string &name) const {
        return glGetAttribLocation(program_id, name.c_str());
    }

    /**
     * \brief Destroy the shaders and the program
     */
    ShaderProgram::~ShaderProgram() {
        // Destroy all the shaders
        detach_vertex();
        detach_geometry();
        detach_fragment();
        detach_tess_con();
        detach_tess_eval();

        // Destroy the program
        glDeleteProgram(program_id);
        program_id = 0;
        program_count--;
    }

    /**
     * \brief Whether the programs are equal
     *
     * Shader programs are equal if they have the same program ID.
     *
     * \param rhs Right hand side
     * \return \c true when equal, \c false otherwise
     */
    bool ShaderProgram::operator==(const ShaderProgram &rhs) const {
        return program_id == rhs.program_id;
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
     * \brief Show the source edit popup
     */
    void ShaderProgram::modify_popup() {
        // Source input field
        ImGui::InputTextMultiline("source", modify_source.get(), source_buffer_size, ImVec2(2 * debug::standard_width, 0));

        // Close button
        ImGui::Separator();
        if (ImGui::Button("Close")) {
            modify_source.reset();
            ImGui::CloseCurrentPopup();
        }

        // Save button
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            std::string src(modify_source.get());

            // Attach
            switch (modify_type) {
                case type::vertex: modify_attached = attach_vertex_source(src); break;
                case type::geometry: modify_attached = attach_geometry_source(src); break;
                case type::fragment: modify_attached = attach_fragment_source(src); break;
                case type::tessellation_control: modify_attached = attach_tess_con_source(src); break;
                case type::tessellation_evaluation: modify_attached = attach_tess_eval_source(src); break;
            }

            // Link
            if (modify_attached) {
                modify_linked = link();
            }

            // Validate
            if (modify_linked) {
                modify_validated = validate();
            }
        }

        // Reset button
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            switch (modify_type) {
                case type::vertex: get_vertex_source(modify_source.get(), source_buffer_size); break;
                case type::geometry: get_geometry_source(modify_source.get(), source_buffer_size); break;
                case type::fragment: get_fragment_source(modify_source.get(), source_buffer_size); break;
                case type::tessellation_control: get_tess_con_source(modify_source.get(), source_buffer_size); break;
                case type::tessellation_evaluation: get_tess_eval_source(modify_source.get(), source_buffer_size); break;
            }
        }

        // Save error and success messages
        if (!modify_attached) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Shader source attachment error");
        } else if(!modify_linked) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Shader program link error");
        } else if(!modify_validated) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Shader program validation error");
        } else {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved");
        }
    }

    /**
     * \brief Show shader program information
     */
    // Note: ## in button name needed for them to detect clicks (need unique labels)
    void ShaderProgram::show_debug() {
        // Vertex shader info
        if (vertex_shader > 0) {
            ImGui::TextUnformatted("Vertex Shader - ");

            // View and modify source button
            ImGui::SameLine();
            if (ImGui::SmallButton("source##0")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                get_vertex_source(modify_source.get(), source_buffer_size);
                modify_type = type::vertex;
                ImGui::OpenPopup("modify");
            }

            // Detach shader button
            ImGui::SameLine();
            if (ImGui::SmallButton("detach##0")) {
                detach_vertex();
                link();
            }
        } else {
            ImGui::TextUnformatted("Vertex Shader - ");

            // Add shader button (view and modify empty source)
            ImGui::SameLine();
            if (ImGui::SmallButton("add##0")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                modify_source.get()[0] = '\0';
                modify_type = type::vertex;
                ImGui::OpenPopup("modify");
            }
        }

        // Geometry shader info
        if (geometry_shader > 0) {
            ImGui::TextUnformatted("Geometry Shader - ");

            // View and modify source button
            ImGui::SameLine();
            if (ImGui::SmallButton("source##1")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                get_geometry_source(modify_source.get(), source_buffer_size);
                modify_type = type::geometry;
                ImGui::OpenPopup("modify");
            }

            // Detach shader button
            ImGui::SameLine();
            if (ImGui::SmallButton("detach##1")) {
                detach_geometry();
                link();
            }
        } else {
            ImGui::TextUnformatted("Geometry Shader - ");

            // Add shader button (view and modify empty source)
            ImGui::SameLine();
            if (ImGui::SmallButton("add##1")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                modify_source.get()[0] = '\0';
                modify_type = type::geometry;
                ImGui::OpenPopup("modify");
            }
        }

        // Fragment shader info
        if (fragment_shader > 0) {
            ImGui::TextUnformatted("Fragment Shader - ");

            // View and modify source button
            ImGui::SameLine();
            if (ImGui::SmallButton("source##2")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                get_fragment_source(modify_source.get(), source_buffer_size);
                modify_type = type::fragment;
                ImGui::OpenPopup("modify");
            }

            // Detach shader button
            ImGui::SameLine();
            if (ImGui::SmallButton("detach##2")) {
                detach_fragment();
                link();
            }
        } else {
            ImGui::TextUnformatted("Fragment Shader - ");

            // Add shader button (view and modify empty source)
            ImGui::SameLine();
            if (ImGui::SmallButton("add##2")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                modify_source.get()[0] = '\0';
                modify_type = type::fragment;
                ImGui::OpenPopup("modify");
            }
        }

        // Tessellation control shader info
        if (tess_con_shader > 0) {
            ImGui::TextUnformatted("Tessellation Control Shader - ");

            // View and modify source button
            ImGui::SameLine();
            if (ImGui::SmallButton("source##3")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                get_tess_con_source(modify_source.get(), source_buffer_size);
                modify_type = type::tessellation_control;
                ImGui::OpenPopup("modify");
            }

            // Detach shader button
            ImGui::SameLine();
            if (ImGui::SmallButton("detach##3")) {
                detach_tess_con();
                link();
            }
        } else {
            ImGui::TextUnformatted("Tessellation Control Shader - ");

            // Add shader button (view and modify empty source)
            ImGui::SameLine();
            if (ImGui::SmallButton("add##3")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                modify_source.get()[0] = '\0';
                modify_type = type::tessellation_control;
                ImGui::OpenPopup("modify");
            }
        }

        // Tessellation evaluation shader info
        if (tess_eval_shader > 0) {
            ImGui::TextUnformatted("Tessellation Evaluation Shader - ");

            // View and modify source button
            ImGui::SameLine();
            if (ImGui::SmallButton("source##4")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                get_tess_eval_source(modify_source.get(), source_buffer_size);
                modify_type = type::tessellation_evaluation;
                ImGui::OpenPopup("modify");
            }

            // Detach shader button
            ImGui::SameLine();
            if (ImGui::SmallButton("detach##4")) {
                detach_tess_eval();
                link();
            }
        } else {
            ImGui::TextUnformatted("Tessellation Evaluation Shader - ");

            // Add shader button (view and modify empty source)
            ImGui::SameLine();
            if (ImGui::SmallButton("add##4")) {
                modify_source = std::make_unique<char[]>(source_buffer_size);
                modify_source.get()[0] = '\0';
                modify_type = type::tessellation_evaluation;
                ImGui::OpenPopup("modify");
            }
        }

        // Show the view and modify dialog
        if (ImGui::BeginPopupModal("modify", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            modify_popup();
            
            ImGui::EndPopup();
        }
    }

    /**
     * \brief Show the ImGui debug widget
     */
    void ShaderProgram::debug_widget() {
        ImGui::Text("Shader programs: %d", program_count);
        ImGui::Text("Vertex shaders: %d", vertex_count);
        ImGui::Text("Geometry shaders: %d", geometry_count);
        ImGui::Text("Fragment shaders: %d", fragment_count);
        ImGui::Text("Tessellation control shaders: %d", tess_con_count);
        ImGui::Text("Tessellation evaluation shaders: %d", tess_eval_count);
    }
//--- end ShaderProgram implementation

//--- start Camera implementation
    /**
     * \brief Construct a camera from all relevant data
     *
     * Construct the camera from all data needed to calculate the matrices
     *
     * \param transformation Position and orientation
     * \param size Size of the viewport
     * \param near Near clipping plane
     * \param far Far clipping plane
     */
    Camera::Camera(const glm::mat4 &transformation, const glm::vec2 &size, float near, float far) : near(near), far(far) {
        this->set_transformation(transformation);
        this->size = glm::vec2(size);
    }

    /**
     * \brief Set view matrix to inverse of the transformation
     *
     * \param transformation Transformation to use
     */
    void Camera::set_transformation(const glm::mat4 &transformation) {
        view_matrix = glm::inverse(transformation);
        recalculate_pv = true;
    }

    void Camera::set_size(const glm::vec2 &newValue) {
        size = newValue;
        recalculate_proj = true;
    }

    glm::vec2 Camera::get_size() const {
        return glm::vec2(size);
    }

    void Camera::set_near(float newValue) {
        near = newValue;
        recalculate_proj = true;
    }

    float Camera::get_near() const {
        return near;
    }

    void Camera::set_far(float newValue) {
        far = newValue;
        recalculate_proj = true;
    }

    float Camera::get_far() const {
        return far;
    }

    /**
     * \brief Show camera information
     */
    void Camera::show_debug() {
        ImGui::InputFloat2("size", &size[0], "%.0f");
        ImGui::InputFloat("near", &near, 0, 0);
        ImGui::InputFloat("far", &far, 0, 0);
        if (ImGui::Button("Recalculate")) {
            recalculate_proj = true;
            get_proj_view_matrix();
        }
        ImGui::Spacing();

        ImGui::TextUnformatted("Projection-view Matrix");
        debug::show_matrix(proj_view_matrix);
        ImGui::Spacing();

        ImGui::TextUnformatted("View Matrix");
        debug::show_matrix(view_matrix);
    }

//--- end Camera implementation

//--- start OrthographicCamera implementation

    /**
     * \brief Construct the camera from all relevant data
     *
     * Construct the orthographic camera from all data needed to calculate the matrices.
     * 
     * \param transformation Position and orientation
     * \param size Size of the viewport
     * \param near Near clipping plane
     * \param far Far clipping plane
     */
    OrthographicCamera::OrthographicCamera(const glm::mat4 &transformation, const glm::vec2& size,float near, float far)
            : Camera(transformation, size, near, far) {}

    glm::mat4 OrthographicCamera::get_proj_view_matrix() {
        if (recalculate_proj) {
            // Projection assumes origin in centre
            proj_matrix = glm::ortho(
                    -size.x / 2, size.x / 2,
                    -size.y / 2, size.y / 2,
                    near, far);

            // Update flag
            recalculate_proj = false;
            recalculate_pv = true;
        }

        if (recalculate_pv) {
            proj_view_matrix = proj_matrix;
            proj_view_matrix *= view_matrix;

            // Reset flag
            recalculate_pv = false;
        }

        return proj_view_matrix;
    }

//--- end OrthographicCamera implementation

//--- start PerspectiveCamera implementation
    /**
     * \brief Construct a camera from all relevant data
     *
     * Construct the perspective camera from all data needed to calculate the matrices
     *
     * \param transformation Position and orientation
     * \param size Size of the viewport
     * \param near Near clipping plane
     * \param far Far clipping plane
     * \param fov Field of view
     */
    PerspectiveCamera::PerspectiveCamera(const glm::mat4 &transformation, const glm::vec2 &size,float near, float far, float fov)
            : Camera(transformation, size, near, far), fov(fov) {}

    glm::mat4 PerspectiveCamera::get_proj_view_matrix() {
        if (recalculate_proj) {
            proj_matrix = glm::perspectiveFov(
                    glm::radians(fov),
                    size.x, size.y,
                    near, far);

            proj_view_matrix = proj_matrix;
            proj_view_matrix *= view_matrix;

            // Update flag
            recalculate_proj = false;
            recalculate_pv = true;
        }

        if (recalculate_pv) {
            proj_view_matrix = proj_matrix;
            proj_view_matrix *= view_matrix;

            // Reset flag
            recalculate_pv = false;
        }

        return proj_view_matrix;
    }

    void PerspectiveCamera::set_fov(float new_value) {
        fov = new_value;
        recalculate_proj = true;
    }

    float PerspectiveCamera::get_fov() const {
        return fov;
    }

    void PerspectiveCamera::show_debug() {
        ImGui::InputFloat("FOV", &fov);
        Camera::show_debug();
    }

//--- end PerspectiveCamera implementation

    /**
     * \brief Show the ImGui debug window
     *
     * \param open Pointer to window's open flag for the close widget
     */
    void debug_window(bool *open) {
        debug::set_standard_width();

        if (ImGui::Begin("OpenGL", open)) {
            if (ImGui::CollapsingHeader("Shaders", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent();
                ShaderProgram::debug_widget();
                ImGui::Unindent();
            }
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
                log::log(shader_lg, log::error, message);
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
