/** \file Components.cpp
 * Component implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Components.h>
#include <open-sea/Debug.h>
#include <open-sea/Model.h>
#include <open-sea/GL.h>

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <stdexcept>
#include <random>
#include <algorithm>
#include <sstream>

namespace open_sea::ecs {

    //--- start ModelComponent implementation
    /**
     * \brief Construct a model component manager
     *
     * Construct a model component manager and pre-allocate space for the given number of components.
     *
     * \param size Number of components
     */
    ModelComponent::ModelComponent(unsigned size) {
        // Allocate the desired size
        allocate(size);
    }

    /**
     * \brief Allocate space for components
     *
     * Allocate space for the given number of components
     *
     * \param size Number of components
     */
    void ModelComponent::allocate(unsigned size) {
        // Make sure data will fit into new space
        assert(size > data.n);

        // Allocate new space
        InstanceData new_data;
        unsigned byte_count = size * record_size;

        new_data.buffer = allocator.allocate(byte_count);
        new_data.n = data.n;
        new_data.allocated = size;

        // Compute pointers to data arrays
        new_data.entity = (Entity*) new_data.buffer;          // Entity is at the start
        new_data.model = (int*) (new_data.entity + size);     // followed by model indices

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(new_data.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(new_data.model, data.model, data.n * sizeof(int));


        }

        // Deallocate old data
        if (data.allocated > 0) {
            allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
        }

        // Set the data
        data = new_data;
    }

    /**
     * \brief Get index to a model
     *
     * Get index to a model, adding the model to the manager's storage if necessary.
     *
     * \param model Pointer to model
     * \return Index into the \c models storage of this manager
     */
    int ModelComponent::model_to_index(std::shared_ptr<model::Model> model) {
        int model_idx;

        // Try to find the model in storage
        auto model_pos = std::find(models.begin(), models.end(), model);
        if (model_pos == models.end()) {
            // Not found -> add the pointer (passed by value, is a copy)
            model_idx = static_cast<int>(models.size());
            models.push_back(model);
        } else {
            // Found -> use found index
            model_idx = static_cast<int>(model_pos - models.begin());
        }

        return model_idx;
    }

    /**
     * \brief Look up index of the entity
     *
     * \param e Entity to look up
     * \return Index of the entity in the data arrays, or \c -1 if the entity was not found
     */
    // O(1)
    int ModelComponent::lookup(Entity e) const {
        try {
            return map.at(e);
        } catch (std::out_of_range &e) {
            return -1;
        }
    }

    /**
     * \brief Look up indices of the entities
     *
     * Look up indices of the entities in the data arrays and write them into the destination.
     * Indices are written in the same order as the entities.
     * Index of \c -1 means the entity was not found.
     *
     * \param e Entities to look up
     * \param count Number of entities
     * \param dest Destination
     */
    // O(count)
    void ModelComponent::lookup(Entity *e, int *dest, unsigned count) const {
        // For each entity retrieve its position from the map
        for (unsigned i = 0; i < count; i++, e++, dest++) {
            try {
                *dest = map.at(*e);
            } catch (std::out_of_range &e) {
                // If not found, set to -1
                *dest = -1;
            }
        }
    }

    /**
     * \brief Get the model at the index
     *
     * Get the model at the index from the internal storage
     *
     * \param i Index
     * \return Model pointer
     */
    std::shared_ptr<model::Model> ModelComponent::get_model(int i) const {
        return std::shared_ptr<model::Model>(models[i]);
    }

    /**
     * \brief Add the component to the entities
     *
     * Add the component to the entities.
     * The model pointers get converted to indices using \c modelToIndex .
     * The number of entities and models must match.
     * Skips any attempt to add the component to an entity that already has it.
     *
     * \param e Entities
     * \param m Models to assign
     * \param count Number of entities
     */
    void ModelComponent::add(Entity *e, std::shared_ptr<model::Model> *m, unsigned count) {
        // Convert pointers to indices
        int indices[count];
        for (int *i = indices; i - indices < count; i++, m++) {
            *i = model_to_index(*m);
        }

        return add(e, indices, count);
    }

    /**
     * \brief Add the component to the entities
     *
     * Add the component to the entities.
     * The indices are passed naively and are not checked against the actual \c models storage.
     * The number of entities and indices must match.
     * Skips any attempt to add the component to an entity that already has it.
     *
     * \param e Entities
     * \param m Indices into the \c models storage of this manager
     * \param count Number of entities
     */
    void ModelComponent::add(Entity *e, int *m, unsigned count) {
        // Check data has enough space
        if (data.allocated < (data.n + count)) {
            // Too small -> reallocate
            //TODO is there a better increment?
            allocate(data.n + count);
        }

        // For every entity, add a new record
        Entity *dest_e = data.entity + data.n;
        int *dest_m = data.model + data.n;
        for (unsigned i = 0; i < count; i++, e++, m++, dest_e++, dest_m++) {
            // Check the entity doesn't have this component yet
            try {
                map.at(*e);     // Produces the exception when entity doesn't have the component
                std::ostringstream message;
                message << "Tried to add component to entity " << e->index() << "-" << e->generation()
                        << " that already has this component";
                log::log(lg, log::warning, message.str());
                continue;
            } catch (std::out_of_range &e) {}

            // Write the data into the buffer
            *dest_e = *e;
            *dest_m = *m;

            // Increment data count and add map entry for the new record
            data.n++;
            map[*e] = data.n - 1;
        }
    }

    /**
     * \brief Set the models at the indices
     *
     * Set the models at the indices.
     * If an index is out of range (greater than or equal to \c data.n), nothing is set (points either to destroyed record
     * or out of allocated range for that data array).
     * The number of indices and models must match
     *
     * \param i Indices into the data arrays
     * \param m Models to assign
     * \param count Number of indices
     */
    void ModelComponent::set(int *i, std::shared_ptr<model::Model> *m, unsigned count) {
        // Convert pointers to indices
        int indices[count];
        for (int *j = indices; j - indices < count; j++, m++) {
            *j = model_to_index(*m);
        }

        set(i, indices, count);
    }

    /**
     * \brief Set the models at the indices
     *
     * Set the models at the indices.
     * If an index is out of range (greater than or equal to \c data.n), nothing is set (points either to destroyed record
     * or out of allocated range fro that data array).
     * The model indices are passed naively and are not checked against the actual \c models storage.
     * The number of indices and model indices must match.
     *
     * \param i Indices into the data arrays
     * \param m Indices into the \c models storage of this manager
     * \param count Number of indices
     */
    void ModelComponent::set(int *i, int *m, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, m++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the value
            data.model[*i] = *m;
        }
    }

    /**
     * \brief Destroy the record at the index
     *
     * Destroy the record at the index.
     * This can lead to reshuffling of the data arrays (to keep them tightly packed) and therefore invalidation of
     * previously looked up indices.
     *
     * \param i Index
     */
    void ModelComponent::destroy(int i) {
        // Check index is in range
        if (i < 0 || static_cast<unsigned>(i) >= data.n) {
            // Records out of range are considered already destroyed
            return;
        }

        // Move last record into i-th place
        int last_idx = data.n - 1;
        Entity e = data.entity[i];
        Entity last = data.entity[last_idx];
        data.entity[i] = data.entity[last_idx];
        data.model[i] = data.model[last_idx];

        // Update data counts and index map
        map.erase(e);
        map[last] = i;
        data.n--;
    }

    /**
     * \brief Collect garbage
     *
     * Destroy records for dead entities.
     * This is done by checking random records until a certain number of live entities in a row are found.
     * Therefore with few dead entities not much time is wasted iterating through the array, and with many dead entities
     * they are destroyed within couple calls.
     *
     * \param manager Entity manager to check entities against
     */
    void ModelComponent::gc(const EntityManager &manager) {
        // Randomly check records until 4 live entities in a row are found
        unsigned alive_in_row = 0;
        static std::random_device device;
        static std::mt19937_64 generator;
        std::uniform_int_distribution<int> distribution(0, data.n);
        while (data.n > 0 && alive_in_row < 4) {
            // Note: % data.n required because data.n can change and this keeps indices in valid range
            unsigned i = distribution(generator) % data.n;
            if (manager.alive(data.entity[i])) {
                alive_in_row++;
            } else {
                alive_in_row = 0;
                destroy(i);
            }
        }
    }

    /**
     * \brief Destroy the component manager, freeing up the used memory
     */
    ModelComponent::~ModelComponent() {
        // Reset model pointers
        for (auto &model : models) {
            model.reset();
        }
        models.clear();

        // Deallocate the data buffer
        allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
    }

    /**
     * \brief Show ImGui debug information
     */
    void ModelComponent::show_debug() {
        ImGui::Text("Record size: %i bytes", record_size);
        ImGui::Text("Records (allocated): %i (%i)", data.n, data.allocated);
        ImGui::Text("Stored models: %i", static_cast<int>(models.size()));
        ImGui::Text("Size data arrays (allocated): %i (%i) bytes", record_size * data.n, record_size * data.allocated);
        if (ImGui::Button("Query")) {
            ImGui::OpenPopup("Component Manager Query");
        }
        if (ImGui::BeginPopupModal("Component Manager Query", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            show_query();

            ImGui::Separator();
            if (ImGui::Button("Close")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

    /**
     * \brief Show ImGui form for querying data from a manager of this component
     */
    void ModelComponent::show_query() {
        ImGui::TextUnformatted("Entity:");
        ImGui::InputInt2("index - generation", query_idx_gen);
        if (ImGui::Button("Refresh")) {
            if (query_idx_gen[0] >= 0 && query_idx_gen[1] >= 0) {
                int i = lookup(ecs::Entity(static_cast<unsigned>(query_idx_gen[0]), static_cast<unsigned>(query_idx_gen[1])));
                query_idx = i;
            } else {
                query_idx = -1;
            }
        }
        ImGui::Separator();
        if (query_idx != -1) {
            ImGui::Text("Model index: %i", data.model[query_idx]);
            ImGui::TextUnformatted("Model information:");
            ImGui::Indent();
            get_model(data.model[query_idx])->show_debug();
            ImGui::Unindent();
        } else {
            ImGui::TextUnformatted("No record found");
        }
    }
    //--- end ModelComponent implementation

    /**
     * \brief Compute transformation matrix
     *
     * \param position Position
     * \param orientation Orientation
     * \param scale Scale
     * \return Transformation
     */
    glm::mat4 transformation(glm::vec3 position, glm::quat orientation, glm::vec3 scale) {
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 r = glm::mat4_cast(orientation);
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        return t * r * s;
    }

    //--- start TransformationComponent implementation
    /**
     * \brief Construct a transformation component manager
     *
     * Construct a transformation component manager and pre-allocate space for the given number of components.
     *
     * \param size Number of components
     */
    TransformationComponent::TransformationComponent(unsigned size) {
        // Allocate the desired size
        allocate(size);
    }

    /**
     * \brief Allocate space for components
     *
     * Allocate space for the given number of components
     *
     * \param size Number of components
     */
    void TransformationComponent::allocate(unsigned size) {
        // Make sure data will fit into new space
        assert(size > data.n);

        // Allocate new space
        InstanceData new_data;
        unsigned byte_count = size * record_size;

        new_data.buffer = allocator.allocate(byte_count);
        new_data.n = data.n;
        new_data.allocated = size;

        // Compute pointers to data arrays
        new_data.entity = (Entity*) new_data.buffer;                      // Entity is at the start
        new_data.position = (glm::vec3*) (new_data.entity + size);        // followed by position
        new_data.orientation = (glm::quat*) (new_data.position + size);   // followed by orientation
        new_data.scale = (glm::vec3*) (new_data.orientation + size);      // followed by scale
        new_data.matrix = (glm::mat4*) (new_data.scale + size);           // followed by matrix
        new_data.parent = (int*) (new_data.matrix + size);                // followed by parent
        new_data.first_child = new_data.parent + size;                     // followed by first child
        new_data.next_sibling = new_data.first_child + size;                // followed by next sibling
        new_data.prev_sibling = new_data.next_sibling + size;               // followed by previous sibling

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(new_data.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(new_data.position, data.position, data.n * sizeof(glm::vec3));
            std::memcpy(new_data.orientation, data.orientation, data.n * sizeof(glm::quat));
            std::memcpy(new_data.scale, data.scale, data.n * sizeof(glm::vec3));
            std::memcpy(new_data.matrix, data.matrix, data.n * sizeof(glm::mat4));
            std::memcpy(new_data.parent, data.parent, data.n * sizeof(int));
            std::memcpy(new_data.first_child, data.first_child, data.n * sizeof(int));
            std::memcpy(new_data.next_sibling, data.next_sibling, data.n * sizeof(int));
            std::memcpy(new_data.prev_sibling, data.prev_sibling, data.n * sizeof(int));


        }

        // Deallocate old data
        if (data.allocated > 0) {
            allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
        }

        // Set the data
        data = new_data;
    }

    /**
     * \brief Look up index of the entity
     *
     * \param e Entity to look up
     * \return Index of the entity in the data arrays, or \c -1 if the entity was not found
     */
    int TransformationComponent::lookup(Entity e) const {
        try {
            return map.at(e);
        } catch (std::out_of_range &e) {
            return -1;
        }
    }

    /**
     * \brief Look up indices of the entities
     *
     * Look up indices of the entities in the data arrays and write them into the destination.
     * Indices are written in the same order as the entities.
     * Index of \c -1 means the entity was not found.
     *
     * \param e Entities to look up
     * \param count Number of entities
     * \param dest Destination
     */
    void TransformationComponent::lookup(Entity *e, int *dest, unsigned count) const {
        // For each entity retrieve its position from the map
        for (unsigned i = 0; i < count; i++, e++, dest++) {
            try {
                *dest = map.at(*e);
            } catch (std::out_of_range &e) {
                // If not found, set to -1
                *dest = -1;
            }
        }
    }

    /**
     * \brief Add the component to the entities
     *
     * Add the component to the entities.
     * The number of each argument must match.
     * Skips any attempt to add the component to an entity that already has it.
     *
     * \param e Entities
     * \param position Positions
     * \param orientation Orientations
     * \param scale Scales
     * \param count Number of entities
     * \param parent Parent index (-1 if root)
     */
    void TransformationComponent::add(Entity *e, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale,
                                      unsigned count, int parent) {
        // Check data has enough space
        if (data.allocated < (data.n + count)) {
            // Too small -> reallocate
            //TODO is there a better increment?
            allocate(data.n + count);
        }

        // Find last child of parent
        int prev_sib = -1;
        if (parent != -1) {
            prev_sib = data.first_child[parent];
            while (prev_sib != -1) {
                if (data.next_sibling[prev_sib] != -1) {
                    // Next sibling present, move to it
                    prev_sib = data.next_sibling[prev_sib];
                } else {
                    // Found last sibling, break out
                    break;
                }
            }
        }

        // For every entity, add a new record
        Entity *dest_e = data.entity + data.n;
        glm::vec3 *dest_p = data.position + data.n;
        glm::quat *dest_o = data.orientation + data.n;
        glm::vec3 *dest_s = data.scale + data.n;
        glm::mat4 *dest_m = data.matrix + data.n;
        int *dest_parent = data.parent + data.n;
        int *dest_first_ch = data.first_child + data.n;
        int *dest_next_sib = data.next_sibling + data.n;
        int *dest_prev_sib = data.prev_sibling + data.n;
        for (unsigned i = 0; i < count; i++, e++, position++, orientation++, scale++,
                dest_e++, dest_p++, dest_o++, dest_s++, dest_m++, dest_parent++, dest_first_ch++, dest_next_sib++, dest_prev_sib++) {
            // Check that the entity doesn't yet have this component
            try {
                map.at(*e);     // Produces the exception when entity doesn't have the component
                std::ostringstream message;
                message << "Tried to add component to entity " << e->index() << "-" << e->generation()
                        << " that already has this component";
                log::log(lg, log::warning, message.str());
                continue;
            } catch (std::out_of_range &e) {}

            // Add as first child to parent if it had no children and this is the first child being added
            if (count > 0 && parent != -1 && prev_sib == -1) {
                data.first_child[parent] = data.n;
            }

            // Write the data into the buffer
            *dest_e = *e;
            *dest_p = *position;
            *dest_o = *orientation;
            *dest_s = *scale;
            *dest_m = transformation(*position, *orientation, *scale);
            *dest_parent = parent;
            *dest_first_ch = -1;
            if (i == count - 1 || parent == -1) {
                *dest_next_sib = -1;
            } else {
                *dest_next_sib = data.n + 1;
            }
            *dest_prev_sib = (parent == -1) ? -1 : prev_sib;

            // Current is previous sibling for next iteration
            prev_sib = data.n;

            // Increment data count and add map entry for the new record
            data.n++;
            map[*e] = data.n - 1;
        }
    }

    /**
     * \brief Set the transformation at the indices
     *
     * Set the transformation at the indices.
     * If an index is out of range (greater than or equal to \c data.n), nothing is set (points either to destroyed record
     * or out of allocated range for that data array).
     * The number of indices and models must match
     *
     * \param i Indices into the data arrays
     * \param position Positions
     * \param orientation Orientations
     * \param scale Scales
     * \param count Number of indices
     */
    void TransformationComponent::set(int *i, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale,
                                      unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, position++, orientation++, scale++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.position[*i] = *position;
            data.orientation[*i] = *orientation;
            data.scale[*i] = *scale;
            update_matrix(*i);
        }
    }

    /**
     * \brief Swap two records
     *
     * Swap two records and adjust references to them to refer to new positions.
     *
     * \param i Record
     * \param j Record
     */
    void TransformationComponent::swap(int i, int j) {
        // Buffer i
        Entity buffer_ent = data.entity[i];
        glm::vec3 buffer_pos = data.position[i];
        glm::quat buffer_ori = data.orientation[i];
        glm::vec3 buffer_sca = data.scale[i];
        glm::mat4 buffer_mat = data.matrix[i];
        int buffer_par = data.parent[i];
        int buffer_fir = data.first_child[i];
        int buffer_nex = data.next_sibling[i];
        int buffer_pre = data.prev_sibling[i];

        // Move j to i
        data.entity[i] = data.entity[j];
        data.position[i] = data.position[j];
        data.orientation[i] = data.orientation[j];
        data.scale[i] = data.scale[j];
        data.matrix[i] = data.matrix[j];
        data.parent[i] = data.parent[j];
        data.first_child[i] = data.first_child[j];
        data.next_sibling[i] = data.next_sibling[j];
        data.prev_sibling[i] = data.prev_sibling[j];
        
        // Change references to j to refer to i (write to buffer if i is the target)
        int prev_sib = data.prev_sibling[j];
        int next_sib = data.next_sibling[j];
        int parent = data.parent[j];
        int ignore_parent = -1;  // Used to fix problem when i an j have a common parent and j is the first child
        if (prev_sib != -1) {
            if (prev_sib == i) {
                buffer_nex = i;
            } else {
                data.next_sibling[prev_sib] = i;
            }
        }
        if (next_sib != -1) {
            if (next_sib == i) {
                buffer_pre = i;
            } else {
                data.prev_sibling[next_sib] = i;
            }
        }
        if (parent != -1 && data.first_child[parent] == j) {
            if (parent == i) {
                buffer_fir = i;
            } else {
                if (buffer_par == parent) {
                    ignore_parent = parent;
                }
                data.first_child[parent] = i;
            }
        }
        
        // Move buffer to j
        data.entity[j] = buffer_ent;
        data.position[j] = buffer_pos;
        data.orientation[j] = buffer_ori;
        data.scale[j] = buffer_sca;
        data.matrix[j] = buffer_mat;
        data.parent[j] = buffer_par;
        data.first_child[j] = buffer_fir;
        data.next_sibling[j] = buffer_nex;
        data.prev_sibling[j] = buffer_pre;

        // Change references to i to refer to j (data in the buffer already knows that j moved to i)
        prev_sib = buffer_pre;
        next_sib = buffer_nex;
        parent = buffer_par;
        if (prev_sib != -1) {
            data.next_sibling[prev_sib] = j;
        }
        if (next_sib != -1) {
            data.prev_sibling[next_sib] = j;
        }
        if (parent != -1 && data.first_child[parent] == i && parent != ignore_parent) {
            data.first_child[parent] = j;
        }

        // Update entity-index mappings
        map[data.entity[i]] = i;
        map[data.entity[j]] = j;
    }

    /**
     * \brief Change the parent at the index
     *
     * \param i Index
     * \param parent Parent (-1 for root)
     */
    void TransformationComponent::adopt(int i, int parent) {
        // Remove from original tree
        int original_parent = data.parent[i];
        if (original_parent != -1) {
            // Make parent point to next sibling if necessary
            if (data.first_child[original_parent] == i) {
                data.first_child[original_parent] = data.next_sibling[i];
            }

            // Connect neighbouring siblings
            int prev_sib = data.prev_sibling[i];
            int next_sib = data.next_sibling[i];
            if (prev_sib != -1) {
                data.next_sibling[prev_sib] = next_sib;
            }
            if (next_sib != -1) {
                data.prev_sibling[next_sib] = prev_sib;
            }
        }

        // Add to destination tree
        data.parent[i] = parent;
        if (parent == -1) {
            // Make root
            data.prev_sibling[i] = -1;
            data.next_sibling[i] = -1;
        } else {
            // Find last child
            int last_child = data.first_child[parent];
            if (last_child != -1) {
                while (data.next_sibling[last_child] != -1) {
                    last_child = data.next_sibling[last_child];
                }
            }

            // Append self
            data.next_sibling[last_child] = i;
            data.prev_sibling[i] = last_child;
        }

        /* --- Ordering not enforced
        // Enforce ordering (only needed when not root)
        if (parent != -1) {
            // Check parent before
            if (parent > i) {
                swap(parent, i);
            }

            // Check siblings (all have to be before)
            if (data.first_child[parent] != i) {
                int sibling = data.first_child[parent];
                while (sibling != i) {
                    if (sibling > i) {
                        swap(sibling, i);
                    }
                    sibling = data.next_sibling[sibling];
                }
            }
        }*/
    }

    /**
     * \brief Update world transformation matrix at the index
     *
     * \param i Index
     */
    void TransformationComponent::update_matrix(int i) {
        // Retreive parent matrix
        glm::mat4 parent;
        if (data.parent[i] == -1) {
            parent = glm::mat4(1.0f);
        } else {
            parent = data.matrix[data.parent[i]];
        }

        // Update own matrix
        data.matrix[i] = parent * transformation(data.position[i], data.orientation[i], data.scale[i]);

        //Update the children
        int child = data.first_child[i];
        while (child != -1) {
            update_matrix(child);
            child = data.next_sibling[child];
        }
    }

    /**
     * \brief Destroy the record at the index
     *
     * Destroy the record at the index.
     * This can lead to reshuffling of the data arrays (to keep them tightly packed) and therefore invalidation of
     * previously looked up indices.
     * All the children of this record are also destroyed.
     *
     * \param i Index
     */
    // Depth-first recursive
    void TransformationComponent::destroy(int i) {
        // Check index is in range
        if (i < 0 || static_cast<unsigned>(i) >= data.n) {
            // Records out of range are considered already destroyed
            return;
        }

        // Destroy children
        int child = data.first_child[i];
        while (child != -1) {
            destroy(child);
            child = data.first_child[i]; // Gets changed to next sibling or -1 by destruction
        }

        // Remove references to i
        int next_sib = data.next_sibling[i];
        int prev_sib = data.prev_sibling[i];
        int parent = data.parent[i];
        if (parent != -1 && data.first_child[parent] == i) {
            data.first_child[parent] = next_sib;
        }
        if (next_sib != -1) {
            data.prev_sibling[next_sib] = prev_sib;
        }
        if (prev_sib != -1) {
            data.next_sibling[prev_sib] = next_sib;
        }

        // Remove entity-index mapping and update data count
        map.erase(data.entity[i]);
        data.n--;

        // Move last record into i-th place
        int last_idx = data.n - 1;
        Entity last = data.entity[last_idx];
        int parent_last = data.parent[last_idx];
        int next_sib_last = data.next_sibling[last_idx];
        int prev_sib_last = data.prev_sibling[last_idx];
        data.entity[i] = last;
        data.position[i] = data.position[last_idx];
        data.orientation[i] = data.orientation[last_idx];
        data.scale[i] = data.scale[last_idx];
        data.matrix[i] = data.matrix[last_idx];
        data.parent[i] = parent_last;
        data.first_child[i] = data.first_child[last_idx];
        data.next_sibling[i] = next_sib_last;
        data.prev_sibling[i] = prev_sib_last;
        if (parent_last != -1 && data.first_child[parent_last] == last_idx) {
            data.first_child[parent_last] = i;
        }
        if (prev_sib_last != -1) {
            data.next_sibling[prev_sib_last] = i;
        }
        if (next_sib_last != -1) {
            data.prev_sibling[next_sib_last] = i;
        }

        // Update entity-index mapping
        map[last] = i;
    }

    /**
     * \brief Collect garbage
     *
     * Destroy records for dead entities.
     * This is done by checking random records until a certain number of live entities in a row are found.
     * Therefore with few dead entities not much time is wasted iterating through the array, and with many dead entities
     * they are destroyed within couple calls.
     *
     * \param manager Entity manager to check entities against
     */
    void TransformationComponent::gc(const EntityManager &manager) {
        // Randomly check records until 4 live entities in a row are found
        unsigned alive_in_row = 0;
        static std::random_device device;
        static std::mt19937_64 generator;
        std::uniform_int_distribution<int> distribution(0, data.n);
        while (data.n > 0 && alive_in_row < 4) {
            // Note: % data.n required because data.n can change and this keeps indices in valid range
            unsigned i = distribution(generator) % data.n;
            if (manager.alive(data.entity[i])) {
                alive_in_row++;
            } else {
                alive_in_row = 0;
                destroy(i);
            }
        }
    }

    /**
     * \brief Destroy the component manager, freeing up the used memory
     */
    TransformationComponent::~TransformationComponent() {
        // Deallocate the data buffer
        allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
    }

    /**
     * \brief Translate entities at indices
     *
     * Translate entities at given indices.
     * Updates both the position and the matrix.
     *
     * \param i Indices
     * \param delta Translation vectors
     * \param count Number of indices
     */
    void TransformationComponent::translate(int *i, glm::vec3 *delta, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.position[*i] += *delta;
            update_matrix(*i);
        }
    }

    /**
     * \brief Rotate entities at indices
     *
     * Rotate entities at given indices.
     * Updates both the orientation and the matrix.
     *
     * \param i Indices
     * \param delta Rotation quaternions
     * \param count Number of indices
     */
    void TransformationComponent::rotate(int *i, glm::quat *delta, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.orientation[*i] = *delta * data.orientation[*i];
            update_matrix(*i);
        }
    }

    /**
     * \brief Scale entities at indices
     *
     * Scale (multiplicatively) entities at given indices.
     * Updates both the scale and the matrix.
     * Scaling is done by component-wise multiplying current scale by the vector.
     *
     * \param i Indices
     * \param delta Scale vectors
     * \param count Number of indices
     */
    void TransformationComponent::scale(int *i, glm::vec3 *delta, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.scale[*i] *= *delta;
            update_matrix(*i);
        }
    }

    /**
     * \brief Set position of entities at indices
     *
     * Set position of entities at given indices.
     * Updates both the position and the matrix.
     *
     * \param i Indices
     * \param position New position vectors
     * \param count Number of indices
     */
    void TransformationComponent::set_position(int *i, glm::vec3 *position, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, position++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.position[*i] = *position;
            update_matrix(*i);
        }
    }

    /**
     * \brief Set orientation of entities at indices
     *
     * Set orientation of entities at given indices.
     * Updates both the orientation and the matrix.
     *
     * \param i Indices
     * \param orientation New orientation quaternions
     * \param count Number of indices
     */
    void TransformationComponent::set_orientation(int *i, glm::quat *orientation, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, orientation++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.orientation[*i] = *orientation;
            update_matrix(*i);
        }
    }

    /**
     * \brief Set scale of entities at indices
     *
     * Set scale of entities at given indices.
     * Updates both the scale and the matrix.
     *
     * \param i Indices
     * \param scale New scale vectors
     * \param count Number of indices
     */
    void TransformationComponent::set_scale(int *i, glm::vec3 *scale, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, scale++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Set the values
            data.scale[*i] = *scale;
            update_matrix(*i);
        }
    }

    /**
     * \brief Show ImGui debug information
     */
    void TransformationComponent::show_debug() {
        ImGui::Text("Record size: %i bytes", record_size);
        ImGui::Text("Records (allocated): %i (%i)", data.n, data.allocated);
        ImGui::Text("Size data arrays (allocated): %i (%i) bytes", record_size * data.n, record_size * data.allocated);
        if (ImGui::Button("Query")) {
            ImGui::OpenPopup("Component Manager Query");
        }
        if (ImGui::BeginPopupModal("Component Manager Query", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            show_query();

            ImGui::Separator();
            if (ImGui::Button("Close")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

    /**
     * \brief Show ImGui form for querying data from a manager of this component
     */
    void TransformationComponent::show_query() {
        ImGui::TextUnformatted("Entity:");
        ImGui::InputInt2("index - generation", query_idx_gen);
        if (ImGui::Button("Refresh")) {
            if (query_idx_gen[0] >= 0 && query_idx_gen[1] >= 0) {
                int i = lookup(ecs::Entity(static_cast<unsigned>(query_idx_gen[0]), static_cast<unsigned>(query_idx_gen[1])));
                query_idx = i;
            } else {
                query_idx = -1;
            }
        }
        ImGui::Separator();
        if (query_idx != -1) {
            ImGui::Text("Position: %.3f, %.3f, %.3f", data.position[query_idx].x, data.position[query_idx].y, data.position[query_idx].z);
            ImGui::TextUnformatted("Orientation:");
            ImGui::SameLine();
            debug::show_quat(data.orientation[query_idx]);
            ImGui::Text("Scale: %.3f, %.3f, %.3f", data.scale[query_idx].x, data.scale[query_idx].y, data.scale[query_idx].z);
            debug::show_matrix(data.matrix[query_idx]);
            ImGui::Text("Parent: %s", (data.parent[query_idx] == -1) ? "none" : data.entity[data.parent[query_idx]].str().c_str());
            ImGui::Text("First child: %s", (data.first_child[query_idx] == -1) ? "none" : data.entity[data.first_child[query_idx]].str().c_str());
            ImGui::Text("Next sibling: %s", (data.next_sibling[query_idx] == -1) ? "none" : data.entity[data.next_sibling[query_idx]].str().c_str());
            ImGui::Text("Previous sibling: %s", (data.prev_sibling[query_idx] == -1) ? "none" : data.entity[data.prev_sibling[query_idx]].str().c_str());
            ImGui::Spacing();
            if (ImGui::Button("Set Position")) {
                query_pos = data.position[query_idx];
                ImGui::OpenPopup("set position");
            } ImGui::SameLine();
            if (ImGui::Button("Set Orientation")) {
                query_ori = data.orientation[query_idx];
                ImGui::OpenPopup("set orientation");
            } ImGui::SameLine();
            if (ImGui::Button("Set Scale")) {
                query_sca = data.scale[query_idx];
                ImGui::OpenPopup("set scale");
            }
            ImGui::Spacing();
            if (ImGui::Button("Translate")) {
                query_pos_delta = {};
                ImGui::OpenPopup("translate");
            } ImGui::SameLine();
            if (ImGui::Button("Rotate")) {
                query_ori_delta = glm::quat();
                ImGui::OpenPopup("rotate");
            } ImGui::SameLine();
            if (ImGui::Button("Scale")) {
                query_sca_fac = {};
                ImGui::OpenPopup("scale");
            }
        } else {
            ImGui::TextUnformatted("No record found");
        }

        // Set position dialog
        if (ImGui::BeginPopupModal("set position", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat3("position", &query_pos.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
            if (ImGui::Button("Set")) {
                set_position(&query_idx, &query_pos, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Set orientation dialog
        if (ImGui::BeginPopupModal("set orientation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat4("orientation", &query_ori.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
            if (ImGui::Button("Set")) {
                set_orientation(&query_idx, &query_ori, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Set scale dialog
        if (ImGui::BeginPopupModal("set scale", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat3("scale", &query_sca.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
            if (ImGui::Button("Set")) {
                set_scale(&query_idx, &query_sca, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Translate dialog
        if (ImGui::BeginPopupModal("translate", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat3("delta", &query_pos_delta.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); } ImGui::SameLine();
            if (ImGui::Button("Translate")) {
                translate(&query_idx, &query_pos_delta, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Rotate dialog
        if (ImGui::BeginPopupModal("rotate", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat4("delta", &query_ori_delta.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); } ImGui::SameLine();
            if (ImGui::Button("Rotate")) {
                rotate(&query_idx, &query_ori_delta, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Scale dialog
        if (ImGui::BeginPopupModal("scale", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputFloat3("factor", &query_sca_fac.x);
            ImGui::Separator();
            if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); } ImGui::SameLine();
            if (ImGui::Button("Scale")) {
                scale(&query_idx, &query_sca_fac, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    //--- end TransformationComponent implementation

    //--- start CameraComponent implementation
    /**
     * \brief Construct a camera component manager
     *
     * Construct a camera component manager and pre-allocate space for the given number of components.
     *
     * \param size Number of components
     */
    CameraComponent::CameraComponent(unsigned size) {
        // Allocate the desired size
        allocate(size);
    }

    /**
     * \brief Allocate space for components
     *
     * Allocate space for the given number of components
     *
     * \param size Number of components
     */
    void CameraComponent::allocate(unsigned size) {
        // Make sure data will fit into new space
        assert(size > data.n);

        // Allocate new space
        InstanceData new_data;
        unsigned byte_count = size * record_size;

        new_data.buffer = allocator.allocate(byte_count);
        new_data.n = data.n;
        new_data.allocated = size;

        // Compute pointers to data arrays
        new_data.entity = (Entity*) new_data.buffer;
        new_data.camera = (std::shared_ptr<gl::Camera>*) (new_data.entity + size);

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(new_data.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(new_data.camera, data.camera, data.n * sizeof(std::shared_ptr<gl::Camera>));
        }

        // Deallocate old data
        if (data.allocated > 0) {
            allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
        }

        // Set the data
        data = new_data;
    }

    /**
     * \brief Look up first index of the entity
     *
     * \param e Entity to look up
     * \return First index of the entity in the data arrays, or \c -1 if the entity was not found
     */
    int CameraComponent::lookup(Entity e) const {
        Entity *d = data.entity;
        for (unsigned i = 0; i < data.n; i++, d++) {
            if (e == *d) {
                return i;
            }
        }
        return -1;
    }

    /**
     * \brief Look up first indices of the entities
     *
     * Look up first indices of the entities in the data arrays and write them into the destination.
     * Indices are written in the same order as the entities.
     * Index of \c -1 means the entity was not found.
     *
     * \param e Entities to look up
     * \param count Number of entities
     * \param dest Destination
     */
    void CameraComponent::lookup(Entity *e, int *dest, unsigned count) const {
        // For each entity retrieve its position from the map
        for (unsigned i = 0; i < count; i++, e++, dest++) {
            *dest = lookup(*e);
        }
    }

    /**
     * \brief Add the component to the entities
     *
     * Add the component to the entities.
     * The number of entities and cameras must match.
     *
     * \param e Entities
     * \param c Cameras to assign
     * \param count Number of entities
     */
    void CameraComponent::add(Entity *e, std::shared_ptr<gl::Camera> *c, unsigned count) {
        // Check data has enough space
        if (data.allocated < (data.n + count)) {
            // Too small -> reallocate
            //TODO is there a better increment?
            allocate(data.n + count);
        }

        // For every entity, add a new record
        Entity *dest_e = data.entity + data.n;
        std::shared_ptr<gl::Camera> *dest_c = data.camera + data.n;
        for (unsigned i = 0; i < count; i++, e++, c++, dest_e++, dest_c++) {
            // Write the data into the buffer
            *dest_e = *e;
            new (dest_c) std::shared_ptr<gl::Camera>(*c);

            // Increment data count
            data.n++;
        }
    }


    /**
     * \brief Set the cameras at the indices
     *
     * Set the cameras at the indices.
     * If an index is out of range (greater than or equal to \c data.n), nothing is set (points either to destroyed record
     * or out of allocated range for that data array).
     * The number of indices and cameras must match
     *
     * \param i Indices into the data arrays
     * \param c Cameras to assign
     * \param count Number of indices
     */
    void CameraComponent::set(int *i, std::shared_ptr<gl::Camera> *c, unsigned count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, i++, c++) {
            int index = *i;

            // Check the index is in range
            if (index < 0 || static_cast<unsigned>(index) >= data.n) {
                // Setting value of records out of range should not have any effect
                return;
            }

            // Change the pointer to share ownership of the new camera
            data.camera[*i] = *c;
        }
    }

    /**
     * \brief Destroy the record at the index
     *
     * Destroy the record at the index.
     * This can lead to reshuffling of the data arrays (to keep them tightly packed) and therefore invalidation of
     * previously looked up indices.
     *
     * \param i Index
     */
    void CameraComponent::destroy(int i) {
        // Check index is in range
        if (i < 0 || static_cast<unsigned>(i) >= data.n) {
            // Records out of range are considered already destroyed
            return;
        }

        // Move last record into i-th place
        int last_idx = data.n - 1;
        data.entity[i] = data.entity[last_idx];
        data.camera[i] = data.camera[last_idx];

        // Destroy the pointer at the last record (as it is now invalidated)
        data.camera[last_idx].~shared_ptr<gl::Camera>();

        // Update data counts and index map
        data.n--;
    }

    /**
     * \brief Collect garbage
     *
     * Destroy records for dead entities.
     * This is done by checking random records until a certain number of live entities in a row are found.
     * Therefore with few dead entities not much time is wasted iterating through the array, and with many dead entities
     * they are destroyed within couple calls.
     *
     * \param manager Entity manager to check entities against
     */
    void CameraComponent::gc(const EntityManager &manager) {
        // Randomly check records until 4 live entities in a row are found
        unsigned alive_in_row = 0;
        static std::random_device device;
        static std::mt19937_64 generator;
        std::uniform_int_distribution<int> distribution(0, data.n);
        while (data.n > 0 && alive_in_row < 4) {
            // Note: % data.n required because data.n can change and this keeps indices in valid range
            unsigned i = distribution(generator) % data.n;
            if (manager.alive(data.entity[i])) {
                alive_in_row++;
            } else {
                alive_in_row = 0;
                destroy(i);
            }
        }
    }

    /**
     * \brief Destroy the component manager, freeing up the used memory
     */
    CameraComponent::~CameraComponent() {
        // Destroy all pointers
        std::shared_ptr<gl::Camera> *c = data.camera;
        for (unsigned i = 0; i < data.n; i++, c++) {
            c->~shared_ptr<gl::Camera>();
        }

        // Deallocate the data buffer
        allocator.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * record_size);
    }

    /**
     * \brief Show ImGui debug information
     */
    void CameraComponent::show_debug() {
        ImGui::Text("Record size: %i bytes", record_size);
        ImGui::Text("Records (allocated): %i (%i)", data.n, data.allocated);
        ImGui::Text("Size data arrays (allocated): %i (%i) bytes", record_size * data.n, record_size * data.allocated);
        if (ImGui::Button("Query")) {
            ImGui::OpenPopup("Component Manager Query");
        }
        if (ImGui::BeginPopupModal("Component Manager Query", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            show_query();

            ImGui::Separator();
            if (ImGui::Button("Close")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }

    /**
     * \brief Show ImGui form for querying data from a manager of this component
     */
    void CameraComponent::show_query() {
        ImGui::TextUnformatted("Entity:");
        ImGui::InputInt2("index - generation", query_idx_gen);
        if (ImGui::Button("Refresh")) {
            query_cameras.clear();
            if (query_idx_gen[0] >= 0 && query_idx_gen[1] >= 0) {
                ecs::Entity q(static_cast<unsigned>(query_idx_gen[0]), static_cast<unsigned>(query_idx_gen[1]));
                ecs::Entity *e = data.entity;
                for (unsigned i = 0; i < data.n; i++, e++) {
                    if (*e == q) {
                        query_cameras.emplace_back(data.camera[i]);
                    }
                }
            }
        }
        ImGui::Separator();
        if (query_cameras.empty()) {
            ImGui::TextUnformatted("No record found");
        } else {
            ImGui::Text("Camera count: %i", static_cast<int>(query_cameras.size()));
            for (unsigned i = 0; i < query_cameras.size(); i++) {
                std::ostringstream label;
                label << "Camera #" << (i + 1);
                if (ImGui::CollapsingHeader(label.str().c_str())) {
                    query_cameras[i]->show_debug();
                }
            }
        }
    }
    //--- end CameraComponent implementation
}
