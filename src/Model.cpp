/*
 * Model implementation
 */

#include <open-sea/Model.h>
#include <open-sea/Log.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <fstream>
#include <sstream>
#include <iterator>
#include <algorithm>

namespace open_sea::model {
    log::severity_logger lg = log::get_logger("Model");

    // Note: OBJ files have 1-based indexing

    //--- start Model implementation
    /**
     * \brief Construct a model from vertices and indices
     * Construct a model from vertex descriptions and indices of the vertices in triangular faces.
     *
     * \param vertices Vertex descriptions
     * \param indices Indices of vertices
     */
    Model::Model(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        vertexCount = static_cast<unsigned int>(indices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glEnableVertexAttribArray(0);   // position
        glEnableVertexAttribArray(1);   // UV

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex::UV), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex::position), (void*) sizeof(Vertex::position));
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Create and fill the index buffer
        glGenBuffers(1, &idxBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*indices.size(), &indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }

    /**
     * \brief Read a model from an OBJ file
     * Read a model from an OBJ file, taking into account vertex positions, UV coordinates and face descriptions.
     *
     * \param path Path to the file
     * \return Unique pointer to \c nullptr on file read failure, or to \c Model object otherwise
     */
    std::unique_ptr<Model> Model::fromFile(const std::string &path) {
        // Verify the file exists and is readable
        std::ifstream stream(path);
        if (stream.fail()) {
            log::log(lg, log::info, std::string("Failed to read file ").append(path));
            return std::unique_ptr<Model>{};
        }

        // Read the vertex descriptions until the first face
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> UVs;
        std::string line;
        while (std::getline(stream, line)) {
            if (boost::starts_with(line, "v ")) {
                // Line is a vertex position (3 floats)
                std::vector<std::string> parts;
                boost::split(parts, line, boost::is_space(), boost::token_compress_on);
                positions.emplace_back(std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
            } else if (boost::starts_with(line, "vt ")) {
                // Line is UV coordinate (2 floats)
                std::vector<std::string> parts;
                boost::split(parts, line, boost::is_space(), boost::token_compress_on);
                UVs.emplace_back(std::stof(parts[1]), std::stof(parts[2]));
            } else if (boost::starts_with(line, "f ")) {
                // Line is a face
                break;
            }
        }

        // Read the face description to construct Vertex instances and add corresponding indices
        // Note: first face is already in line
        // Note: assuming triangulated, therefore exactly three vertices per face
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        do {
            // Ignore non-face lines
            if (!boost::starts_with(line, "f "))
                break;

            // Separate the three vertex descriptions
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_space(), boost::token_compress_on);

            // For each vertex, find if it is already in vertices
            for (int j = 0; j < 3; j++) {
                // Separate the position, texture and normal indices
                std::vector<std::string> descs;
                boost::split(descs, parts[j + 1], boost::is_any_of("/"), boost::token_compress_on);

                // Build the vertex representation and try to find it in the vertex container
                glm::vec3 p = positions[std::stoi(descs[0]) - 1];
                glm::vec2 t = UVs[std::stoi(descs[2]) - 1];
                Vertex v{
                        .position = {p.x, p.y, p.z},
                        .UV = {t.x, t.y}
                };
                auto i = std::find(vertices.begin(), vertices.end(), v);

                // Add if not present, use the index if present
                if (i == vertices.end()) {
                    // New vertex
                    vertices.push_back(v);
                    indices.push_back(vertices.size() - 1);
                } else {
                    // Already present
                    indices.push_back(i - vertices.begin());
                }
            }
        } while (std::getline(stream, line));

        // Create and return the model form the data
        return std::make_unique<Model>(vertices, indices);
    }

    /**
     * \brief Draw the model
     * Bind the vertex array, draw the model, unbind the vertex array
     */
    void Model::draw() {
        // Bind the vertex array
        glBindVertexArray(vertexArray);

        // Draw the triangles from the start of the index buffer
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, nullptr);

        // Unbind the vertex array
        glBindVertexArray(0);
    }

    Model::~Model() {
        // Delete buffers and vertex array
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteBuffers(1, &idxBuffer);
        glDeleteVertexArrays(1, &vertexArray);
    }
    //--- end Model implementation

    //--- start UntexModel implementation
    /**
     * \brief Construct an untextured model from vertices and indices
     * Construct an untextured model from vertex descriptions and indices of the vertices in triangular faces.
     *
     * \param vertices Vertex descriptions
     * \param indices Indices of vertices
     */
    UntexModel::UntexModel(const std::vector<UntexModel::Vertex> &vertices, const std::vector<unsigned int> &indices)
            : Model(){
        vertexCount = static_cast<unsigned int>(indices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glEnableVertexAttribArray(0);   // position

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Create and fill the index buffer
        glGenBuffers(1, &idxBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*indices.size(), &indices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }

    /**
     * \brief Read an untextured model from an OBJ file
     * Read an untextured model from an OBJ file, taking into account vertex positions and face descriptions.
     *
     * \param path Path to the file
     * \return Unique pointer to \c nullptr on file read failure, or to \c UntexModel object otherwise
     */
    std::unique_ptr<UntexModel> UntexModel::fromFile(const std::string &path) {
        // Verify the file exists and is readable
        std::ifstream stream(path);
        if (stream.fail()) {
            log::log(lg, log::info, std::string("Failed to read file ").append(path));
            return std::unique_ptr<UntexModel>{};
        }

        // Read the vertex descriptions until the first face
        std::vector<glm::vec3> positions;
        std::string line;
        while (std::getline(stream, line)) {
            if (boost::starts_with(line, "v ")) {
                // Line is a vertex position (3 floats)
                std::vector<std::string> parts;
                boost::split(parts, line, boost::is_space(), boost::token_compress_on);
                positions.emplace_back(std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
            } else if (boost::starts_with(line, "f ")) {
                // Line is a face
                break;
            }
        }

        // Read the face description to construct Vertex instances and add corresponding indices
        // Note: first face is already in line
        // Note: assuming triangulated, therefore exactly three vertices per face
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        do {
            // Ignore non-face lines
            if (!boost::starts_with(line, "f "))
                break;

            // Separate the three vertex descriptions
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_space(), boost::token_compress_on);

            // For each vertex, find if it is already in vertices
            for (int j = 0; j < 3; j++) {
                // Separate the position, texture and normal indices
                std::vector<std::string> descs;
                boost::split(descs, parts[j + 1], boost::is_any_of("/"), boost::token_compress_on);

                // Build the vertex representation and try to find it in the vertex container
                glm::vec3 p = positions[std::stoi(descs[0]) - 1];
                Vertex v{ .position = {p.x, p.y, p.z} };
                auto i = std::find(vertices.begin(), vertices.end(), v);

                // Add if not present, use the index if present
                if (i == vertices.end()) {
                    // New vertex
                    vertices.push_back(v);
                    indices.push_back(vertices.size() - 1);
                } else {
                    // Already present
                    indices.push_back(i - vertices.begin());
                }
            }
        } while (std::getline(stream, line));

        // Create and return the model form the data
        return std::make_unique<UntexModel>(vertices, indices);
    }
    //--- end UntexModel implementation
}
