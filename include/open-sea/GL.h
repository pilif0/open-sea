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
     * The supported shaders are: vertex, geometry, fragment
     */
    //TODO: possibly add support for tessellation and compute shaders
    class ShaderProgram {
        private:
            //! Vertex shader reference
            GLuint vertexShader   = 0;
            //! Geometry shader reference
            GLuint geometryShader = 0;
            //! Fragment shader reference
            GLuint fragmentShader = 0;

        public:
            //! Shader program reference
            GLuint programID = 0;

            ShaderProgram();

            bool attachVertexFile(const std::string& path);
            bool attachGeometryFile(const std::string& path);
            bool attachFragmentFile(const std::string& path);

            bool attachVertexSource(const std::string& src);
            bool attachGeometrySource(const std::string& src);
            bool attachFragmentSource(const std::string& src);

            bool link();
            void use();
            static void unset();    //TODO: the name doesn't feel right (doesn't show being inverse of use())

            GLint getUniformLocation(const std::string& name);
            GLint getAttributeLocation(const std::string& name);

            ~ShaderProgram();
    };

    void log_errors();
}

#endif //OPEN_SEA_GL_H
