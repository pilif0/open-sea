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
            //! Logger for this class
            static log::severity_logger lg = log::get_logger("Shader Program");

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

            void attachVertexFile(const std::string& path);
            void attachGeometryFile(const std::string& path);
            void attachFragmentFile(const std::string& path);

            void attachVertexSource(const std::string& src);
            void attachGeometrySource(const std::string& src);
            void attachFragmentSource(const std::string& src);

            void link();
            void use();
            static void unset();    //TODO: the name doesn't feel right (doesn't show being inverse of use())

            ~ShaderProgram();
    };

    void log_errors();
}

#endif //OPEN_SEA_GL_H
