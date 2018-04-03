/** \file Model.h
 * Model related code
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_MODEL_H
#define OPEN_SEA_MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <memory>

namespace open_sea::model {

    /** \class Model
     * \brief 3D model representation
     * Representation of a 3D model and all information needed to render it.
     * Can be loaded directly from data (for dynamic models) or from a file.
     * Data is interlaced in the vertex buffer.
     * Assumes data is already triangulated.
     */
    class Model {
        protected:
            //! Default constructor for purposes of inheritance
            Model() {};
            //! ID of the vertex buffer
            GLuint vertexBuffer;
            //! ID of the index buffer
            GLuint idxBuffer;
            //! ID of the vertex array
            GLuint vertexArray;
            //! Number of vertices to draw
            unsigned int vertexCount;
            //! Number of unique vertices
            unsigned int uniqueVertexCount;
        public:
            //! Position and UV coordinates of a vertex
            struct Vertex {
                glm::vec3 position;
                glm::vec2 UV;

                bool operator==(const Vertex &rhs) const { return position == rhs.position && UV == rhs.UV; }
                bool operator!=(const Vertex &rhs) const { return !(rhs == *this); }
            };
            Model(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
            static std::unique_ptr<Model> fromFile(const std::string& path);

            void draw();
            void showDebug();

            ~Model();
    };

    /** \class UntexModel
     * \brief Untextured 3D model representation
     * Specialisation of the general model class that assumes no texture is used.
     * Therefore UV coordinates are not used and memory is saved by not having to store and move them.
     */
    class UntexModel : public Model {
        public:
            //! Position of a vertex
            struct Vertex {
                glm::vec3 position;

                bool operator==(const Vertex &rhs) const { return position == rhs.position; }
                bool operator!=(const Vertex &rhs) const { return !(rhs == *this); }
            };
            UntexModel(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
            static std::unique_ptr<UntexModel> fromFile(const std::string& path);
    };
}

#endif //OPEN_SEA_MODEL_H
