/** \file Model.h
 * Model related code
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_MODEL_H
#define OPEN_SEA_MODEL_H

#include <glad/glad.h>
#include <glm/vec3.hpp>

#include <vector>
#include <string>

namespace open_sea::model {

    /** \class Model
     * \brief 3D model representation
     * Representation of a 3D model and all information needed to render it.
     * Can be loaded directly from data (for dynamic models) or from a file.
     * Data is interlaced in the vertex buffer.
     * Assumes data is already triangulated.
     */
    class Model {
        public:
            //! Position and UV coordinates of a vertex
            struct Vertex {
                glm::vec3 position;
                glm::vec3 UV;
            };
        protected:
            //! Vertex data
            std::vector<Vertex> vertices;
            //! ID of the vertex buffer
            GLuint vertexBuffer;
            //! Vertex indices
            std::vector<int> indices;
            //! ID of the index buffer
            GLuint idxBuffer;
            //! ID of the vertex array
            GLuint vertexArray;
        public:
            Model(const std::string& path);
            Model(const Vertex& vertices, const int& indices, uint count);
            void draw();
    };

    /** \class UntexModel
     * \brief Untextured 3D model representation
     * Specialisation of the general model class that assumes no texture is used.
     * Therefore UV coordinates are not used and memory is saved by not having to store and move them.
     */
    class UntexModel : Model {
        public:
            //! Position of a vertex
            struct Vertex {
                glm::vec3 position;
            };
            UntexModel(const std::string& path);
            UntexModel(const Vertex& vertices, const int& indices, uint count);
    };
}

#endif //OPEN_SEA_MODEL_H
