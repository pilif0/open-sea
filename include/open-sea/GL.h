/** \file GL.h
 * OpenGL related code
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_GL_H
#define OPEN_SEA_GL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>

namespace open_sea::gl {

    /** \class ShaderProgram
     * \brief OpenGL shader program representation
     * OpenGL shader program representation that ties together different shaders.
     * The supported shaders are: vertex, geometry, fragment, tessellation
     * If the context version is less than 4.0, attempts to attach tessellation shaders are ignored.
     */
    class ShaderProgram {
        private:
            //! Vertex shader reference
            GLuint vertexShader   = 0;
            //! Geometry shader reference
            GLuint geometryShader = 0;
            //! Fragment shader reference
            GLuint fragmentShader = 0;
            //! Tessellation control shader reference
            GLuint tessConShader = 0;
            //! Tessellation evaluation shader reference
            GLuint tessEvalShader = 0;
            //! Whether the program has been linked since the last shader change
            bool linked = false;

            // Debug info
            //! Number of existing vertex shaders
            static uint vertexCount;
            //! Number of existing geometry shaders
            static uint geometryCount;
            //! Number of existing fragment shaders
            static uint fragmentCount;
            //! Number of tessellation control shaders
            static uint tessConCount;
            //! Number of tessellation evaluation shaders
            static uint tessEvalCount;
            //! Number of existing shader programs
            static uint programCount;

        public:
            //! Shader program reference
            GLuint programID = 0;

            ShaderProgram();

            bool attachVertexFile(const std::string& path);
            bool attachGeometryFile(const std::string& path);
            bool attachFragmentFile(const std::string& path);
            bool attachTessConFile(const std::string& path);
            bool attachTessEvalFile(const std::string& path);

            bool attachVertexSource(const std::string& src);
            bool attachGeometrySource(const std::string& src);
            bool attachFragmentSource(const std::string& src);
            bool attachTessConSource(const std::string& src);
            bool attachTessEvalSource(const std::string& src);

            void detachVertex();
            void detachGeometry();
            void detachFragment();
            void detachTessCon();
            void detachTessEval();

            bool link();
            bool isLinked() const;
            bool validate();
            void use() const;
            static void unset();

            GLint getUniformLocation(const std::string& name);
            GLint getAttributeLocation(const std::string& name);

            ~ShaderProgram();
            static void debugWidget();

            bool operator==(const ShaderProgram &rhs) const;
            bool operator!=(const ShaderProgram &rhs) const;
    };

    //--- Cameras
    // Note: The system assumes origin in bottom-left corner with x to right, y up and z backward.

    /** \class Camera
     * \brief General camera representation
     * General camera representation.
     * All cameras are objects that produce a projection-view matrix based on some properties (position, orientation, resolution, ...).
     */
    class Camera {
        protected:
            //! Position in world space
            glm::vec3 position;
            //! Orientation in world space
            glm::quat orientation;
            //! View matrix (changes when camera position or orientation change)
            glm::mat4 viewMatrix;
            //! \c true when view matrix needs to be recalculated before use
            bool recalculateView;

            //! Width and height of the camera view
            glm::vec2 size;
            //! Near clipping plane
            float near;
            //! Far clipping plane
            float far;
            //! Projection matrix (changes when camera size or clipping planes change)
            glm::mat4 projMatrix;
            //! \c true when projection matrix needs to be recalculated before use
            bool recalculateProj;

            //! Projection-view matrix (changes when either component matrix changes)
            glm::mat4 projViewMatrix;

            Camera(const glm::vec3& position, const glm::quat& orientation, const glm::vec2& size, float near, float far);
        public:
            /**
             * \brief Get the current projection-view matrix
             *
             * \return Current projection-view matrix
             */
            virtual glm::mat4 getProjViewMatrix() = 0;

            void setPosition(const glm::vec3& newValue);
            glm::vec3 getPosition() const;
            void setRotation(const glm::quat& newValue);
            glm::quat getRotation() const;
            void translate(const glm::vec3& d);
            void rotate(const glm::quat& d);

            void setSize(const glm::vec2& newValue);
            glm::vec2 getSize() const;
            void setNear(float newValue);
            float getNear() const;
            void setFar(float newValue);
            float getFar() const;

            virtual void showDebugControls();
    };

    /** \class OrthographicCamera
     * \brief Orthographic camera representation
     * Orthographic camera that produces an orthographic projection-view matrix based on position, orientation and view size.
     * Camera position is the position of the centre of the view.
     */
    class OrthographicCamera : public Camera {
        public:
            OrthographicCamera(const glm::vec3& position, const glm::quat& orientation, const glm::vec2& size, float near,
                               float far);
            glm::mat4 getProjViewMatrix() override;
    };

    /** \class PerspectiveCamera
     * \brief Perspective camera representation
     * Perspecitve camera that produces a perspective projection-view matrix based on position, orientation, view size and
     * field of view.
     */
    class PerspectiveCamera : public Camera {
        private:
            //! Field of view in degrees
            float fov;
        public:
            PerspectiveCamera(const glm::vec3& position, const glm::quat& orientation, const glm::vec2& size, float near,
                              float far, float fov);
            glm::mat4 getProjViewMatrix() override;
            void showDebugControls() override;

            void setFOV(float newValue);
            float getFOV() const;
    };

    void log_errors();

    void debug_window();
}

#endif //OPEN_SEA_GL_H
