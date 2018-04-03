/*
 * Model implementation
 */

#include <open-sea/Model.h>
#include <open-sea/Log.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <imgui.h>

#include <fstream>
#include <sstream>
#include <iterator>
#include <exception>
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
        uniqueVertexCount = static_cast<unsigned int>(vertices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glEnableVertexAttribArray(0);   // position
        glEnableVertexAttribArray(1);   // UV

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(Vertex::position));
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // Create and fill the index buffer
        glGenBuffers(1, &idxBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices.size(), &indices[0], GL_STATIC_DRAW);

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
        int f = 1;
        bool untextured = true;
        do {
            // Ignore non-face lines
            if (!boost::starts_with(line, "f "))
                break;

            // Separate the three vertex descriptions
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_space(), boost::token_compress_on);

            // For each vertex, find if it is already in vertices
            for (int j = 1; j <= 3; j++) {
                // Separate the position, texture and normal indices
                std::vector<std::string> descs;
                boost::split(descs, parts[j], boost::is_any_of("/"), boost::token_compress_on);

                // Parse the position index and get the position
                int iP;
                if (descs.empty() || descs[0].empty()) {
                    std::ostringstream message;
                    message << "Face " << f << " missing vertex " << j << " position in " << path;
                    log::log(lg, log::error, message.str());
                    return std::unique_ptr<Model>{};
                } else {
                    try {
                        iP = std::stoi(descs[0]);
                    } catch (std::exception &e) {
                        std::ostringstream message;
                        message << "Face " << f << " referencing non-numeric vertex " << j <<" position ('" << descs[0]
                                << "') in " << path;
                        log::log(lg, log::error, message.str());
                        return std::unique_ptr<Model>{};
                    }
                }
                glm::vec3 p{};
                if (iP < 1 || iP > positions.size()) {
                    // Undeclared position
                    std::ostringstream message;
                    message << "Face " << f << " referencing unknown vertex " << j <<" position ('" << descs[0]
                            << "') in " << path;
                    log::log(lg, log::error, message.str());
                    return std::unique_ptr<Model>{};
                } else {
                    p = positions[iP - 1];
                }

                // Parse the UV index and get the UV
                int iUV;
                if (descs.size() < 2 || descs[1].empty()) {
                    // Leave index as 0 and use [0,0] as UV
                    iUV = 0;
                } else {
                    untextured = false;
                    try {
                        iUV = std::stoi(descs[0]);
                    } catch (std::exception &e) {
                        std::ostringstream message;
                        message << "Face " << f << " referencing non-numeric vertex " << j <<" UV ('" << descs[1]
                                << "') in " << path;
                        log::log(lg, log::error, message.str());
                        return std::unique_ptr<Model>{};
                    }
                }
                glm::vec2 t{};
                if (iUV == 0) {
                    // Leave t as [0,0]
                } else if(iUV < 1 || iUV > UVs.size()) {
                    // Undeclared UV
                    std::ostringstream message;
                    message << "Face " << f << " referencing unknown vertex " << j <<" UV ('" << descs[1]
                            << "') in " << path;
                    log::log(lg, log::error, message.str());
                    return std::unique_ptr<Model>{};
                } else {
                    t = UVs[iUV - 1];
                }
                
                // Build the vertex representation and try to find it in the vertex container
                Vertex v{
                        .position = p,
                        .UV = t
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

            f++;
        } while (std::getline(stream, line));

        // Warn if no vertex had a texture
        if (untextured)
            log::log(lg, log::warning, std::string("Using Model for and untextured model (should use UntexModel) in ").append(path));


        // Create and return the model form the data
        log::log(lg, log::info, std::string("Model loaded from ").append(path));
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

    /**
     * \brief Show ImGui debug information for this model
     */
    void Model::showDebug() {
        ImGui::Text("Vertices (unique): %i (%i)", vertexCount, uniqueVertexCount);
        ImGui::Text("Memory size: %i B", sizeof(unsigned int) * vertexCount + sizeof(Vertex) * uniqueVertexCount);
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
        uniqueVertexCount = static_cast<unsigned int>(vertices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glEnableVertexAttribArray(0);   // position

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // Create and fill the index buffer
        glGenBuffers(1, &idxBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*indices.size(), &indices[0], GL_STATIC_DRAW);

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
        int f = 1;
        do {
            // Ignore non-face lines
            if (!boost::starts_with(line, "f "))
                continue;

            // Separate the three vertex descriptions
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_space(), boost::token_compress_on);

            // For each vertex, find if it is already in vertices
            for (int j = 1; j <= 3; j++) {
                // Separate the position, texture and normal indices
                std::vector<std::string> descs;
                boost::split(descs, parts[j], boost::is_any_of("/"), boost::token_compress_on);

                // Parse the position index and get the position
                int iP;
                if (descs.empty() || descs[0].empty()) {
                    std::ostringstream message;
                    message << "Face " << f << " missing vertex " << j << " position in " << path;
                    log::log(lg, log::error, message.str());
                    return std::unique_ptr<UntexModel>{};
                } else {
                    try {
                        iP = std::stoi(descs[0]);
                    } catch (std::exception &e) {
                        std::ostringstream message;
                        message << "Face " << f << " referencing non-numeric vertex " << j <<" position ('" << descs[0]
                                << "') in " << path;
                        log::log(lg, log::error, message.str());
                        return std::unique_ptr<UntexModel>{};
                    }
                }
                glm::vec3 p{};
                if (iP < 1 || iP > positions.size()) {
                    // Undeclared position
                    std::ostringstream message;
                    message << "Face " << f << " referencing unknown vertex " << j <<" position ('" << descs[0]
                            << "') in " << path;
                    log::log(lg, log::error, message.str());
                    return std::unique_ptr<UntexModel>{};
                } else {
                    p = positions[iP - 1];
                }

                // Build the vertex representation and try to find it in the vertex container
                Vertex v{ .position = p };
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

            f++;
        } while (std::getline(stream, line));

        // Create and return the model form the data
        log::log(lg, log::info, std::string("Untextured model loaded from ").append(path));
        return std::make_unique<UntexModel>(vertices, indices);
    }

    /**
     * \brief Show ImGui debug information for this model
     */
    void UntexModel::showDebug() {
        ImGui::Text("Vertices (unique): %i (%i)", vertexCount, uniqueVertexCount);
        ImGui::Text("Memory size: %i B", sizeof(unsigned int) * vertexCount + sizeof(Vertex) * uniqueVertexCount);
    }
    //--- end UntexModel implementation
}
