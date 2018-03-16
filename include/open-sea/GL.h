/** \file GL.h
 * OpenGL related code
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_GL_H
#define OPEN_SEA_GL_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <open-sea/Log.h>
namespace log = open_sea::log;

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

    void log_errors();

    void debug_window();
}

#endif //OPEN_SEA_GL_H
