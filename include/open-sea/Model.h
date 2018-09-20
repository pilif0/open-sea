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

//! %Model creation and transformations
namespace open_sea::model {
    /**
     * \addtogroup Model
     * \brief %Model creation and transformations
     *
     * %Model creation, parsing and transformations.
     * Various types of models can be loaded from files or directly from vertex data.
     * The different model types contain different information required by different renderers.
     * The different models should be interchangable when it comes to storage (see \ref open_sea::ecs::ModelComponent "Model Component").
     *
     * @{
     */

    /** \class Model
     * \brief 3D model representation
     *
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
                //! Position
                glm::vec3 position;
                //! Texture coordinate
                glm::vec2 UV;

                bool operator==(const Vertex &rhs) const { return position == rhs.position && UV == rhs.UV; }
                bool operator!=(const Vertex &rhs) const { return !(rhs == *this); }
            };
            Model(const std::vector<Model::Vertex>& vertices, const std::vector<unsigned int>& indices);
            static std::unique_ptr<Model> fromFile(const std::string& path);

            void draw() const;
            virtual void showDebug();

            //! Get VAO ID
            GLuint getVertexArray() { return vertexArray; }
            //! Get vertex count
            unsigned getVertexCount() { return vertexCount; }

            ~Model();
    };

    /** \class UntexModel
     * \brief Untextured 3D model representation
     *
     * Specialisation of the general model class that assumes no texture is used.
     * Therefore UV coordinates are not used and memory is saved by not having to store and move them.
     */
    class UntexModel : public Model {
        public:
            //! Position of a vertex
            struct Vertex {
                //! Position
                glm::vec3 position;

                bool operator==(const Vertex &rhs) const { return position == rhs.position; }
                bool operator!=(const Vertex &rhs) const { return !(rhs == *this); }
                static Vertex reduce(Model::Vertex source);
            };
            UntexModel(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
            static std::unique_ptr<UntexModel> fromFile(const std::string& path);
            void showDebug() override;
    };

    // OBJ functions
    bool read_OBJ_vertices(std::ifstream& stream, const std::string& path,
                           std::shared_ptr<std::vector<glm::vec3>> &positions, std::shared_ptr<std::vector<glm::vec2>> &UVs);
    bool read_OBJ_faces(std::ifstream& stream, const std::string& path,
                        std::shared_ptr<std::vector<glm::vec3>> &positions, std::shared_ptr<std::vector<glm::vec2>> &UVs,
                        std::vector<Model::Vertex> &vertices, std::vector<unsigned int> &indices);

    /**
     * @}
     */
}

#endif //OPEN_SEA_MODEL_H
