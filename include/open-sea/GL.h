/** \file GL.h
 * OpenGL related code
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_GL_H
#define OPEN_SEA_GL_H

#include <open-sea/Debuggable.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <memory>

//! OpenGL specific code
namespace open_sea::gl {
    /**
     * \addtogroup Shaders
     * \brief OpenGL shader program
     *
     * OpenGL shader program representation and relevant functions.
     *
     * @{
     */

    /** \class ShaderProgram
     * \brief OpenGL shader program representation
     *
     * OpenGL shader program representation that ties together different shaders.
     * The supported shaders are: vertex, geometry, fragment, tessellation.
     * If the context version is less than 4.0, attempts to attach tessellation shaders are ignored.
     */
    class ShaderProgram : public debug::Debuggable {
        private:
            //! Vertex shader reference
            GLuint vertex_shader   = 0;
            //! Geometry shader reference
            GLuint geometry_shader = 0;
            //! Fragment shader reference
            GLuint fragment_shader = 0;
            //! Tessellation control shader reference
            GLuint tess_con_shader = 0;
            //! Tessellation evaluation shader reference
            GLuint tess_eval_shader = 0;
            //! Whether the program has been linked since the last shader change
            bool linked = false;

            // Debug info
            //! Number of existing vertex shaders
            static uint vertex_count;
            //! Number of existing geometry shaders
            static uint geometry_count;
            //! Number of existing fragment shaders
            static uint fragment_count;
            //! Number of tessellation control shaders
            static uint tess_con_count;
            //! Number of tessellation evaluation shaders
            static uint tess_eval_count;
            //! Number of existing shader programs
            static uint program_count;

        public:
            //! Supported shader types
            enum class type {vertex, geometry, fragment, tessellation_control, tessellation_evaluation};

            //! Shader program reference
            GLuint program_id = 0;

            ShaderProgram();

            bool attach_vertex_file(const std::string &path);
            bool attach_geometry_file(const std::string &path);
            bool attach_fragment_file(const std::string &path);
            bool attach_tess_con_file(const std::string &path);
            bool attach_tess_eval_file(const std::string &path);

            bool attach_vertex_source(const std::string &src);
            bool attach_geometry_source(const std::string &src);
            bool attach_fragment_source(const std::string &src);
            bool attach_tess_con_source(const std::string &src);
            bool attach_tess_eval_source(const std::string &src);

            int get_vertex_source(char *dest, unsigned size);
            int get_geometry_source(char *dest, unsigned size);
            int get_fragment_source(char *dest, unsigned size);
            int get_tess_con_source(char *dest, unsigned size);
            int get_tess_eval_source(char *dest, unsigned size);

            void detach_vertex();
            void detach_geometry();
            void detach_fragment();
            void detach_tess_con();
            void detach_tess_eval();

            bool link();
            bool is_linked() const;
            bool validate();
            void use() const;
            static void unset();

            GLint get_uniform_location(const std::string &name);
            GLint get_attribute_location(const std::string &name);

            ~ShaderProgram();
            //! Size of the buffer for shader source modification
            static constexpr unsigned source_buffer_size = 1 << 16;
            //! Pointer to the buffer for shader source modification
            std::unique_ptr<char[]> modify_source{};
            //! Type of the shader being modified
            type modify_type;
            //! Whether modified shader was successfully attached
            bool modify_attached = true;
            //! Whether modified shader was successfully linked
            bool modify_linked = true;
            //! Whether modified shader was successfully validated
            bool modify_validated = true;
            void modify_popup();
            void show_debug() override;
            static void debug_widget();

            bool operator==(const ShaderProgram &rhs) const;
            bool operator!=(const ShaderProgram &rhs) const;
    };

    /**
     * @}
     */

    /**
     * \addtogroup Cameras
     * \brief OpenGL cameras
     *
     * Various cameras useable for OpenGL projection.
     *
     * @{
     */

    /** \class Camera
     * \brief General camera representation
     *
     * General camera representation.
     * All cameras are objects that produce a projection-view matrix based on some properties (transformation, size, ...).
     */
    class Camera : public debug::Debuggable {
        protected:
            //! View matrix (inverse of transformation)
            glm::mat4 view_matrix;

            //! Width and height of the camera view
            glm::vec2 size;
            //! Near clipping plane
            float near;
            //! Far clipping plane
            float far;
            //! Projection matrix (changes when camera size or clipping planes change)
            glm::mat4 proj_matrix;
            //! \c true when projection matrix needs to be recalculated before use
            bool recalculate_proj;

            //! Projection-view matrix (changes when either component matrix changes)
            glm::mat4 proj_view_matrix;
            //! \c true when projection-view matrix needs to be recalculated
            bool recalculate_pv;

            Camera(const glm::mat4& transformation, const glm::vec2& size, float near, float far);
        public:
            /**
             * \brief Get the current projection-view matrix
             *
             * \return Current projection-view matrix
             */
            virtual glm::mat4 get_proj_view_matrix() = 0;

            void set_transformation(const glm::mat4 &transformation);

            void set_size(const glm::vec2 &newValue);
            glm::vec2 get_size() const;
            void set_near(float newValue);
            float get_near() const;
            void set_far(float newValue);
            float get_far() const;

            void show_debug() override;
    };

    /** \class OrthographicCamera
     * \brief Orthographic camera representation
     *
     * Orthographic camera that produces an orthographic projection-view matrix based on position, orientation and view size.
     * Camera position is the position of the centre of the view.
     */
    class OrthographicCamera : public Camera {
        public:
            OrthographicCamera(const glm::mat4 &transformation, const glm::vec2 &size, float near, float far);
            glm::mat4 get_proj_view_matrix() override;
    };

    /** \class PerspectiveCamera
     * \brief Perspective camera representation
     *
     * Perspecitve camera that produces a perspective projection-view matrix based on position, orientation, view size and
     * field of view.
     */
    class PerspectiveCamera : public Camera {
        private:
            //! Field of view in degrees
            float fov;
        public:
            PerspectiveCamera(const glm::mat4 &transformation, const glm::vec2& size, float near, float far, float fov);
            glm::mat4 get_proj_view_matrix() override;
            void show_debug() override;

            void set_fov(float new_value);
            float get_fov() const;
    };

    void log_errors();

    void debug_window(bool *open);

    /**
     * @}
     */
}

#endif //OPEN_SEA_GL_H
