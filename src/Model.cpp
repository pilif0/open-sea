/** \file Model.cpp
 * Model implementation
 *
 * \author Filip Smola
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
    //! Module logger
    log::severity_logger lg = log::get_logger("Model");

    // Note: OBJ files have 1-based indexing

    /**
     * \brief Read vertex descriptions from an OBJ file
     *
     * Read vertex positions and UV coordinates from a stream of an OBJ file.
     * Read until the next line would be a face definition.
     * Skip any empty, comment or unsupported lines.
     * The destinations get cleared before use.
     *
     * \param stream File as stream
     * \param path Path to the file (for error messages)
     * \param positions Positions destination vector
     * \param UVs UV coordinates destination vector (can be empty)
     */
    bool read_obj_vertices(std::ifstream &stream, const std::string &path,
                           std::shared_ptr<std::vector<glm::vec3>> &positions,
                           std::shared_ptr<std::vector<glm::vec2>> &UVs) {
        // Check positions is present
        if (!positions) {
            log::log(lg, log::error, std::string("Positions not present when reading vertices from ").append(path));
            return false;
        }

        // Clear the destination vectors
        if (positions) {
            positions->clear();
        }
        if (UVs) {
            UVs->clear();
        }

        // Read the vertex descriptions until the first face
        while (!stream.eof()) {
            // Peek
            int peek = stream.peek();

            // Stop when next would be a face definition
            if (peek == 'f') {
                break;
            }

            // Skip empty or comment lines
            if (peek == '\n' || peek == '#') {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            // Decide based on first two characters
            int start[2]{stream.get(), stream.get()};
            if (start[0] == 'v') {
                if (start[1] == 't' && UVs) {
                    // Line is UV coordinate and the UV destination is present
                    std::string line;
                    std::getline(stream, line);
                    std::vector<std::string> parts;
                    boost::split(parts, line, boost::is_space(), boost::token_compress_on);
                    positions->emplace_back(std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2]));
                } else if (start[1] == ' ') {
                    // Line is position
                    std::string line;
                    std::getline(stream, line);
                    std::vector<std::string> parts;
                    boost::split(parts, line, boost::is_space(), boost::token_compress_on);
                    positions->emplace_back(std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2]));
                } else {
                    // Not supported -> skip line
                    stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
            } else {
                // Not supported -> skip line
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }

        // Success
        return true;
    }

    /**
     * \brief Read faces from an OBJ file given previously read positions and UV coordinates
     *
     * Read face descriptions form an OBJ file and use previously read vertex positions and UV coordinates to construct
     * a list of unique vertices and indices to use.
     * Assumes triangular faces.
     * Any non-face line until the end of file gets ignored.
     * The destinations get cleared before use.
     *
     * \param stream File as stream just before the first face line
     * \param path Path to the file (for error messages)
     * \param positions Vertex positions
     * \param UVs UV coordinates
     * \param vertices Unique vertices destination
     * \param indices Indices destination
     * \return \c false on failure, \c true otherwise
     */
    bool read_obj_faces(std::ifstream &stream, const std::string &path,
                        std::shared_ptr<std::vector<glm::vec3>> &positions,
                        std::shared_ptr<std::vector<glm::vec2>> &UVs,
                        std::vector<Model::Vertex> &vertices, std::vector<unsigned int> &indices) {
        // Check positions is present
        if (!positions) {
            log::log(lg, log::error, std::string("Positions not present when reading faces from ").append(path));
            return false;
        }

        // Clear the destination vectors
        vertices.clear();
        indices.clear();

        // Parse line by line
        int f = 1;
        while (!stream.eof()) {
            // Decide based on first two characters
            int start[2]{stream.get(), stream.get()};

            // Skip non-face lines
            if (start[0] != 'f' || start[1] != ' ') {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            // Separate rest of the line by spaces into vertex description references
            std::string line;
            std::getline(stream, line);
            std::vector<std::string> parts;
            boost::split(parts, line, boost::is_space(), boost::token_compress_on);

            // Check the face is a triangle
            if (parts.size() != 3) {
                std::ostringstream message;
                message << "Face " << f << " doesn't have exactly three vertices in " << path;
                log::log(lg, log::error, message.str());
                return false;
            }

            // Process each vertex
            for (int v = 0; v < 3; v++) {
                // Separate description references
                std::vector<std::string> refs;
                boost::split(refs, parts[v], boost::is_any_of("/"), boost::token_compress_off);

                // Parse position index and get the position
                int i_p;
                if (refs.empty() || refs[0].empty()) {
                    // Position reference not present
                    std::ostringstream message;
                    message << "Face " << f << " missing vertex " << v << " position in " << path;
                    log::log(lg, log::error, message.str());
                    return false;
                } else {
                    try {
                        i_p = std::stoi(refs[0]);
                    } catch (std::exception &e) {
                        // Position reference not a valid number
                        std::ostringstream message;
                        message << "Face " << f << " referencing non-numeric vertex " << v <<" position ('" << refs[0]
                                << "') in " << path;
                        log::log(lg, log::error, message.str());
                        return false;
                    }
                }
                glm::vec3 p{};
                if (i_p < 1 || i_p > positions->size()) {
                    // Undeclared position
                    std::ostringstream message;
                    message << "Face " << f << " referencing unknown vertex " << v <<" position ('" << refs[0]
                            << "') in " << path;
                    log::log(lg, log::error, message.str());
                    return false;
                } else {
                    p = (*positions)[i_p - 1];
                }

                // Parse the UV index and get the UV
                int i_uv;
                if (refs.size() < 2 || refs[1].empty()) {
                    // UV reference not present -> Set index as 0 and use [0,0] as UV
                    i_uv = 0;
                } else {
                    try {
                        i_uv = std::stoi(refs[1]);
                    } catch (std::exception &e) {
                        // UV reference not a valid number
                        std::ostringstream message;
                        message << "Face " << f << " referencing non-numeric vertex " << v <<" UV ('" << refs[1]
                                << "') in " << path;
                        log::log(lg, log::error, message.str());
                        return false;
                    }
                }
                glm::vec2 t{};
                if (i_uv == 0 || !UVs) {
                    // Leave t as [0,0]
                } else if(i_uv < 1 || i_uv > UVs->size()) {
                    // Undeclared UV
                    std::ostringstream message;
                    message << "Face " << f << " referencing unknown vertex " << v <<" UV ('" << refs[1]
                            << "') in " << path;
                    log::log(lg, log::error, message.str());
                    return false;
                } else {
                    t = (*UVs)[i_uv - 1];
                }

                // Construct the vertex
                Model::Vertex vertex{.position = p, .UV = t};

                // Try to find the vertex in already used ones
                auto i = std::find(vertices.begin(), vertices.end(), vertex);

                // Add if not present, use the index if present
                if (i == vertices.end()) {
                    // Not present -> new vertex -> add it
                    vertices.push_back(vertex);
                    indices.push_back(static_cast<unsigned int &&>(vertices.size() - 1));
                } else {
                    // Present -> repeated vertex -> use its index
                    indices.push_back(static_cast<unsigned int &&>(i - vertices.begin()));
                }
            }

            // Increment face counter
            f++;
        }

        // Success
        return true;
    }

    //--- start Model implementation
    /**
     * \brief Construct a model from vertices and indices
     *
     * Construct a model from vertex descriptions and indices of the vertices in triangular faces.
     *
     * \param vertices Vertex descriptions
     * \param indices Indices of vertices
     */
    Model::Model(const std::vector<Model::Vertex>& vertices, const std::vector<unsigned int>& indices) {
        vertex_count = static_cast<unsigned int>(indices.size());
        unique_vertex_count = static_cast<unsigned int>(vertices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);
        glEnableVertexAttribArray(0);   // position
        glEnableVertexAttribArray(1);   // UV

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) sizeof(Vertex::position));
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // Create and fill the index buffer
        glGenBuffers(1, &idx_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices.size(), &indices[0], GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    /**
     * \brief Read a model from an OBJ file
     *
     * Read a model from an OBJ file, taking into account vertex positions, UV coordinates and face descriptions.
     *
     * \param path Path to the file
     * \return Unique pointer to \c nullptr on file read failure, or to \c Model object otherwise
     */
    std::unique_ptr<Model> Model::from_file(const std::string &path) {
        // Verify the file exists and is readable
        std::ifstream stream(path);
        if (stream.fail()) {
            log::log(lg, log::info, std::string("Failed to read file ").append(path));
            return std::unique_ptr<Model>{};
        }

        // Read the vertex descriptions until the first face
        std::shared_ptr<std::vector<glm::vec3>> positions = std::make_shared<std::vector<glm::vec3>>();
        std::shared_ptr<std::vector<glm::vec2>> uvs = std::make_shared<std::vector<glm::vec2>>();
        read_obj_vertices(stream, path, positions, uvs);

        // Read the face descriptions
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        if (!read_obj_faces(stream, path, positions, uvs, vertices, indices)) {
            log::log(lg, log::info, std::string("Failed to read faces from ").append(path));
            return std::unique_ptr<Model>{};
        }

        // Create and return the model form the data
        log::log(lg, log::info, std::string("Model loaded from ").append(path));
        return std::make_unique<Model>(vertices, indices);
    }

    /**
     * \brief Draw the model
     *
     * Bind the vertex array, draw the model, unbind the vertex array
     */
    void Model::draw() const {
        // Bind the vertex array
        glBindVertexArray(vertex_array);

        // Draw the triangles from the start of the index buffer
        glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, nullptr);

        // Unbind the vertex array
        glBindVertexArray(0);
    }

    /**
     * \brief Show ImGui debug information for this model
     */
    void Model::show_debug() {
        ImGui::Text("Vertices (unique): %i (%i)", vertex_count, unique_vertex_count);
        ImGui::Text("Memory size: %i B", static_cast<int>(
                sizeof(unsigned int) * vertex_count + sizeof(Vertex) * unique_vertex_count));
    }

    Model::~Model() {
        // Delete buffers and vertex array
        glDeleteBuffers(1, &vertex_buffer);
        glDeleteBuffers(1, &idx_buffer);
        glDeleteVertexArrays(1, &vertex_array);
    }
    //--- end Model implementation

    //--- start UntexModel implementation
    /**
     * \brief Reduce general vertex to untextured vertex
     *
     * \param source General vertex
     * \return Untextured vertex
     */
    UntexModel::Vertex UntexModel::Vertex::reduce(const Model::Vertex source) {
        return UntexModel::Vertex{.position = source.position};
    }

    /**
     * \brief Construct an untextured model from vertices and indices
     *
     * Construct an untextured model from vertex descriptions and indices of the vertices in triangular faces.
     *
     * \param vertices Vertex descriptions
     * \param indices Indices of vertices
     */
    UntexModel::UntexModel(const std::vector<UntexModel::Vertex> &vertices, const std::vector<unsigned int> &indices)
            : Model(){
        vertex_count = static_cast<unsigned int>(indices.size());
        unique_vertex_count = static_cast<unsigned int>(vertices.size());

        // Create vertex array and enable attributes
        glGenVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);
        glEnableVertexAttribArray(0);   // position

        // Create and fill the vertex buffer
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // Create and fill the index buffer
        glGenBuffers(1, &idx_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*indices.size(), &indices[0], GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    /**
     * \brief Read an untextured model from an OBJ file
     *
     * Read an untextured model from an OBJ file, taking into account vertex positions and face descriptions.
     *
     * \param path Path to the file
     * \return Unique pointer to \c nullptr on file read failure, or to \c UntexModel object otherwise
     */
    std::unique_ptr<UntexModel> UntexModel::from_file(const std::string &path) {
        // Verify the file exists and is readable
        std::ifstream stream(path);
        if (stream.fail()) {
            log::log(lg, log::info, std::string("Failed to read file ").append(path));
            return std::unique_ptr<UntexModel>{};
        }

        // Read the vertex descriptions until the first face
        std::shared_ptr<std::vector<glm::vec3>> positions = std::make_shared<std::vector<glm::vec3>>();
        std::shared_ptr<std::vector<glm::vec2>> uvs{};
        read_obj_vertices(stream, path, positions, uvs);

        // Read the face descriptions
        std::vector<Model::Vertex> vertices;
        std::vector<unsigned int> indices;
        if (!read_obj_faces(stream, path, positions, uvs, vertices, indices)) {
            log::log(lg, log::info, std::string("Failed to read faces from ").append(path));
            return std::unique_ptr<UntexModel>{};
        }

        // Reduce vertices
        std::vector<Vertex> reduced;
        reduced.resize(vertices.size());
        std::transform(vertices.begin(), vertices.end(), reduced.begin(), [](Model::Vertex v){return Vertex::reduce(v);});

        // Create and return the model form the data
        log::log(lg, log::info, std::string("Untextured model loaded from ").append(path));
        return std::make_unique<UntexModel>(reduced, indices);
    }

    /**
     * \brief Show ImGui debug information for this model
     */
    void UntexModel::show_debug() {
        ImGui::Text("Vertices (unique): %i (%i)", vertex_count, unique_vertex_count);
        ImGui::Text("Memory size: %i B", static_cast<int>(
                sizeof(unsigned int) * vertex_count + sizeof(Vertex) * unique_vertex_count));
    }
    //--- end UntexModel implementation
}
