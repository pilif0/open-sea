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

#include <glm/glm.hpp>

#include <stdexcept>
#include <random>
#include <algorithm>

namespace open_sea::ecs {
    //! Number of live entities that need to be seen in row before garbage collection gives up
    constexpr size_t live_in_row = 4;

    //--- Start ModelTable implementation
    /**
     * \brief Construct a model component manager
     *
     * Construct a model component manager and pre-allocate space for the given number of components.
     *
     * \param size Number of components
     */
    ModelTable::ModelTable(unsigned size) {
        this->table = std::make_unique<data::TableSoA<Entity, Data>>(size);
    }

    /**
     * \brief Get index to a model
     *
     * Get index to a model, adding the model to the manager's storage if necessary.
     *
     * \param model Pointer to model
     * \return Index into the \c models storage of this manager
     */
    size_t ModelTable::model_to_index(const std::shared_ptr<model::Model> &model) {
        size_t model_idx;

        // Try to find the model in storage
        auto model_pos = std::find(models.begin(), models.end(), model);
        if (model_pos == models.end()) {
            // Not found -> add the pointer
            model_idx = static_cast<int>(models.size());
            models.emplace_back(model); //TODO make sure this works
        } else {
            // Found -> use found index
            model_idx = static_cast<int>(model_pos - models.begin());
        }

        return model_idx;
    }

    /**
     * \brief Get the model at the provided index
     *
     * Get the model at the provided index from the model store
     *
     * \param i Index
     * \return Model pointer
     */
    std::shared_ptr<model::Model> ModelTable::get_model(size_t i) const {
        return std::shared_ptr<model::Model>(models[i]);
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
    void ModelTable::gc(const EntityManager &manager) {
        unsigned seen_live_in_row = 0;
        static std::random_device device;
        static std::mt19937_64 generator(table->size());

        std::vector<Entity> entities = table->keys();
        std::uniform_int_distribution<int> distribution(0, entities.size());

        // Keep trying while there are entities and haven't seen too many living
        while (!entities.empty() && seen_live_in_row < live_in_row) {
            // Note: modulo required because number of entities can change and this keeps indices in valid range
            size_t i = distribution(generator) % entities.size();

            if (manager.alive(entities[i])) {
                // Increment live counter
                seen_live_in_row++;
            } else {
                // Reset live counter and remove record from table and entity list
                seen_live_in_row = 0;
                table->remove(entities[i]);
                entities.erase(entities.begin() + i);
            }
        }
    }

    /**
     * \brief Destroy the component manager, freeing up the used memory
     */
    //TODO this is not necessary, model store destructor takes care of this
    ModelTable::~ModelTable() {
        // Reset model pointers
        for (auto &model : models) {
            model.reset();
        }
        models.clear();
    }

    /**
     * \brief Show ImGui debug information
     */
    void ModelTable::show_debug() {
        ImGui::Text("Table type: %s", table->type_name());
        ImGui::Text("Record size: %lu bytes", sizeof(Data));
        ImGui::Text("Records: %lu (%lu bytes)", table->size(), sizeof(Data) * table->size());
        ImGui::Text("Allocated: %lu (%lu bytes)", table->allocated(), sizeof(Data) * table->allocated());
        ImGui::Text("Pages allocated: %lu", table->pages());
        ImGui::Text("Stored models: %i", static_cast<int>(models.size()));
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
    void ModelTable::show_query() {
        ImGui::TextUnformatted("Entity:");
        ImGui::InputInt2("index - generation", query_idx_gen);
        if (ImGui::Button("Refresh")) {
            if (query_idx_gen[0] >= 0 && query_idx_gen[1] >= 0) {
                try {
                    Entity entity(static_cast<unsigned>(query_idx_gen[0]), static_cast<unsigned>(query_idx_gen[1]));
                    query_ref = table->get_reference(entity);
                } catch (std::out_of_range&) {
                    query_ref = {nullptr};
                }
            } else {
                query_ref = {nullptr};
            }
        }
        ImGui::Separator();
        if (query_ref.model) {
            size_t model_id = *(query_ref.model);
            ImGui::Text("Model index: %zu", model_id);
            ImGui::TextUnformatted("Model information:");
            ImGui::Indent();
            get_model(model_id)->show_debug();
            ImGui::Unindent();
        } else {
            ImGui::TextUnformatted("No record found");
        }
    }
    //--- End ModelTable implementation

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

    //--- Start TransformationTable implementation
    /**
     * \brief Construct a transformation component manager
     *
     * Construct a transformation component manager and pre-allocate space for the given number of components.
     *
     * \param size Number of components
     */
    TransformationTable::TransformationTable(unsigned size) {
        this->table = std::make_unique<data::TableSoA<Entity, Data>>(size);
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
    void TransformationTable::gc(const EntityManager &manager) {
        unsigned seen_live_in_row = 0;
        static std::random_device device;
        static std::mt19937_64 generator(table->size());

        std::vector<Entity> entities = table->keys();
        std::uniform_int_distribution<int> distribution(0, entities.size());

        // Keep trying while there are entities and haven't seen too many living
        while (!entities.empty() && seen_live_in_row < live_in_row) {
            // Note: modulo required because number of entities can change and this keeps indices in valid range
            size_t i = distribution(generator) % entities.size();

            if (manager.alive(entities[i])) {
                // Increment live counter
                seen_live_in_row++;
            } else {
                // Reset live counter and remove record from table and entity list
                seen_live_in_row = 0;
                remove(entities[i]);
                entities.erase(entities.begin() + i);
            }
        }
    }

    /**
     * \brief Add the component to the entity
     *
     * Each records is added to the start of the parent's children list.
     *
     * \param key Entity
     * \param position Position
     * \param orientation Orientation
     * \param scale Scale
     * \param parent Parent (-1 if root)
     * \return `true` iff the structure was modified
     */
    bool TransformationTable::add(const Entity &key,  const glm::vec3 &position, const glm::quat &orientation, const glm::vec3 &scale, const data::opt_index parent) {
        // Get parent reference
        Data::Ptr par_ref{};
        if (parent.is_set()) {
            try {
                par_ref = table->get_reference(parent);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Adding record to parent that can't be found.");
            }
        }

        // Construct data
        Data data = {
                position,
                orientation,
                scale,
                transformation(position, orientation, scale),
                parent,
                {},
                (parent.is_set()) ? *par_ref.first_child : data::opt_index(),
                {}
        };

        // Add it
        auto idx = table->add(key, data);

        // Check for failure
        if (!idx.is_set()) {
            return false;
        }

        // Link with sibling and parent
        if (parent.is_set()) {
            // Set as previous first child's sibling
            auto sibling = *par_ref.first_child;
            if (sibling.is_set()) {
                // Get reference
                Data::Ptr sib_ref{};
                try {
                    sib_ref = table->get_reference(sibling);
                } catch (std::out_of_range &e) {
                    // Not found -> Invalid sibling provided
                    throw std::invalid_argument("Parent references first child that can't be found.");
                }

                // Set previous sibling
                sib_ref.prev_sibling->set(idx);
            }

            // Set as parent's first child
            par_ref.first_child->set(idx);
        }

        // Update parent's matrix to propagate it to new children
        if (parent.is_set()) {
            update_matrix(parent);
        }

        return idx.is_set();
    }

    /**
     * \brief Add the component to the entities
     *
     * Each records is added to the start of the parent's children list.
     * Any record that fails to be added is skipped.
     *
     * \param key Entities
     * \param position Positions
     * \param orientation Orientations
     * \param scale Scales
     * \param parent Parent (-1 if root)
     * \return `true` iff the structure was modified
     */
    // Note: This can't be done in SOA style, because each inserted record needs to know where the previous was inserted
    //          (for prev_sibling, and to set its next_sibling).
    bool TransformationTable::add(const Entity *keys, const glm::vec3 *position, const glm::quat *orientation, const glm::vec3 *scale, const data::opt_index parent, size_t count) {
        // Get parent reference
        Data::Ptr par_ref{};
        if (parent.is_set()) {
            try {
                par_ref = table->get_reference(parent);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Adding records to parent that can't be found.");
            }
        }

        // Perform for each record in sequence
        bool result = false;
        const Entity *k = keys;
        const glm::vec3 *p = position;
        const glm::quat *o = orientation;
        const glm::vec3 *s = scale;
        data::opt_index last_added = parent.is_set() ? *par_ref.first_child : data::opt_index();
        for (size_t i = 0; i < count; i++, k++, p++, o++, s++) {
            // Construct data
            Data data = {
                    *p,
                    *o,
                    *s,
                    transformation(*p, *o, *s),
                    parent,
                    data::opt_index(),
                    parent.is_set() ? last_added : data::opt_index(),
                    data::opt_index()
            };

            // Add it
            //TODO What if this fails? e.g. key already present
            data::opt_index just_added = table->add(*k, data);
            result = just_added.is_set() || result;

            // Set as last added's previous sibling if added non-root
            if (last_added.is_set() && just_added.is_set()) {
                // Get reference
                Data::Ptr last_ref{};
                try {
                    last_ref = table->get_reference(last_added);
                } catch (std::out_of_range &e) {
                    // Not found -> just added it, something is VERY wrong
                    throw std::invalid_argument(i == 0 || !result ? // i.e. if first success
                        "Parent references first child that can't be found." :
                        "Record that was just added can't be found.");
                }

                // Set previous sibling
                last_ref.prev_sibling->set(just_added);
            }

            // Update last added index if non-root, skipping failures
            if (just_added.is_set() && parent.is_set()) {
                last_added.set(just_added);
            }
        }

        // Set parent's first child to the last added record
        if (parent.is_set()) {
            par_ref.first_child->set(last_added);
        }

        // Update parent's matrix to propagate it to new children
        if (parent.is_set()) {
            update_matrix(parent);
        }

        return result;
    }

    /**
     * \brief Change the subject's parent
     *
     * The subject is added to the start of the parent's children list
     *
     * \param e Subject entity
     * \param parent Parent index (unset for root)
     */
     // Note: Uses opt_index instead of Entity because it needs to be able to express an empty parent (root)
    void TransformationTable::adopt(const Entity &e, const data::opt_index parent) {
        // Get the subject reference
        Data::Ptr ref{};
        auto idx = table->lookup(e);
        try {
            ref = table->get_reference(e);
        } catch (std::out_of_range &e) {
            // Not found -> nothing to update
            return;
        }

        // Remove from original tree
        auto original_parent = *ref.parent;
        if (original_parent.is_set()) {
            // Get original parent reference
            Data::Ptr par_ref{};
            try {
                par_ref = table->get_reference(original_parent);
            } catch (std::out_of_range &e) {
                // Not found -> table structure broken, something is VERY wrong
                throw std::invalid_argument("Record references parent that can't be found.");
            }

            // Make parent point to next sibling if necessary
            if (par_ref.first_child->is_set()) {
                par_ref.first_child->set(*ref.next_sibling);
            }

            // Connect neighbouring siblings
            auto prev_sib = *ref.prev_sibling;
            auto next_sib = *ref.next_sibling;
            if (prev_sib.is_set()) {
                Data::Ptr prev_ref{};
                try {
                    prev_ref = table->get_reference(prev_sib);
                } catch (std::out_of_range &e) {
                    // Not found -> table structure broken, something is VERY wrong
                    throw std::invalid_argument("Record references previous sibling that can't be found.");
                }
                prev_ref.next_sibling->set(next_sib);
            }
            if (next_sib.is_set()) {
                Data::Ptr next_ref{};
                try {
                    next_ref = table->get_reference(next_sib);
                } catch (std::out_of_range &e) {
                    // Not found -> table structure broken, something is VERY wrong
                    throw std::invalid_argument("Record references next sibling that can't be found.");
                }
                next_ref.prev_sibling->set(prev_sib);
            }
        }

        // Add to destination tree
        ref.parent->set(parent);
        if (!parent.is_set()) {
            // Make root
            ref.prev_sibling->unset();
            ref.next_sibling->unset();
        } else {
            // Get new parent reference
            Data::Ptr par_ref{};
            try {
                par_ref = table->get_reference(parent);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Tried to set parent to one that can't be found.");
            }

            // Make first child
            auto sibling = *par_ref.first_child;
            par_ref.first_child->set(idx);

            // Link with new next sibling
            Data::Ptr sib_ref{};
            try {
                sib_ref = table->get_reference(sibling);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Record references first child that can't be found.");
            }
            sib_ref.prev_sibling->set(idx);
            ref.next_sibling->set(sibling);
        }
    }

    /**
     * \brief Update world transformation matrix of a record at the provided index
     *
     * \param idx Record index
     * \param siblings Whether to also update following siblings
     */
    // Note: siblings flag is used to simplify recursion
    void TransformationTable::update_matrix(data::opt_index idx, bool siblings) {
        // Check the index is set
        if (!idx.is_set()) {
            // Nothing to update
            return;
        }

        // Get data
        Data::Ptr data{};
        try {
            data = table->get_reference(idx);
        } catch (std::out_of_range &e) {
            // Index out of range -> nothing to update
            return;
        }

        // Retrieve parent matrix
        glm::mat4 parent;
        if (data.parent->is_set()) {
            // Get parent reference
            Data::Ptr par_ref{};
            try {
                par_ref = table->get_reference(*data.parent);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Record references parent that can't be found.");
            }

            parent = *par_ref.matrix;
        } else {
            parent = glm::mat4(1.0f);
        }

        // Update own matrix
        *data.matrix = parent * transformation(*data.position, *data.orientation, *data.scale);

        // Update the children
        if (data.first_child->is_set()) {
            update_matrix(*data.first_child, true);
        }

        // Update siblings if asked to
        if (siblings && data.next_sibling->is_set()) {
            update_matrix(*data.next_sibling, true);
        }
    }

    /**
     * \brief Update world transformation matrix of entity
     *
     * \param e Entity
     * \param siblings Whether to also update following siblings
     */
    // Note: siblings flag is used to simplify recursion
    void TransformationTable::update_matrix(Entity e, bool siblings) {
        // Look up record and pass to index-based overload
        update_matrix(table->lookup(e), siblings);
    }

    /**
     * \brief Remove record at the provided index and its children
     *
     * \param idx Record index
     * \return `true` iff the structure was modified
     */
    bool TransformationTable::remove(data::opt_index idx) {
        // Check the index is set
        if (!idx.is_set()) {
            // Nothing to remove
            return false;
        }

        // Get the data
        Data::Ptr data{};
        try {
            data = table->get_reference(idx);
        } catch (std::out_of_range &e) {
            return false;
        }

        // Recursively destroy children
        while (data.first_child->is_set()) {
            // Remove first child
            auto first_child = *data.first_child;
            bool removed = remove(first_child);

            // Adjust index and data if we got moved by this removal
            if (removed && idx.get() == table->size()) {
                // Removal happened and index is now outside the table -> got moved to first_child
                idx = first_child;
                try {
                    data = table->get_reference(idx);
                } catch (std::out_of_range &e) {
                    // Previously valid first child index is now outside -> it was last but by above so were we
                    //  -> somehow we were our first child -> fatal error
                    throw std::exception();
                }
            } else if (!removed) {
                // Child wasn't removed -> should never happen - break potential infinite loop and note the error
                //TODO there probably is a better exception for this
                throw std::invalid_argument("Record child couldn't be removed.");
            }
        }

        // Remove references to this record
        if (data.parent->is_set()) {
            // Get the reference to parent
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*data.parent);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid parent provided
                throw std::invalid_argument("Record references parent that can't be found.");
            }

            // Check we are removing the first child
            if ((*ref.first_child) == idx) {
                // Replace with next sibling
                ref.first_child->set(*data.next_sibling);
            }
        }
        if (data.next_sibling->is_set()) {
            // Get the reference next sibling
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*data.next_sibling);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid sibling provided
                throw std::invalid_argument("Record references next sibling that can't be found.");
            }

            // Set next sibling's previous sibling to mine (or lack thereof)
            ref.prev_sibling->set(*data.prev_sibling);
        }
        if (data.prev_sibling->is_set()) {
            // Get the reference to previous sibling
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*data.prev_sibling);
            } catch (std::out_of_range &e) {
                // Not found -> Invalid sibling provided
                throw std::invalid_argument("Record references previous sibling that can't be found.");
            }

            // Set previous sibling's next sibling to mine (or lack thereof)
            ref.next_sibling->set(*data.next_sibling);
        }

        // Remove the record
        return table->remove(idx).is_set();
    }

    /**
     * \brief Remove entity's record and those of its children
     *
     * \param key Entity
     * \return `true` iff the structure was modified
     */
    bool TransformationTable::remove(const Entity &key) {
        // Look up record and pass to index-based overload
        return remove(table->lookup(key));
    }

    /**
     * \brief Translate entities
     *
     * Translate given entities.
     * Updates both the position and the matrix.
     *
     * \param e Entities
     * \param delta Translation vectors
     * \param count Number of entities
     */
    void TransformationTable::translate(Entity *e, glm::vec3 *delta, size_t count) {
        // For every entity, set the value
        for (unsigned j = 0; j < count; j++, e++, delta++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.position += *delta;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Rotate entities
     *
     * Rotate given entities.
     * Updates both the orientation and the matrix.
     *
     * \param e Entities
     * \param delta Rotation quaternions
     * \param count Number of entities
     */
    void TransformationTable::rotate(Entity *e, glm::quat *delta, size_t count) {
        // For every entity, set the value
        for (unsigned j = 0; j < count; j++, e++, delta++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.orientation = *delta * *ref.orientation;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Scale entities
     *
     * Scale (multiplicatively) given entities.
     * Updates both the scale and the matrix.
     * Scaling is done by component-wise multiplying current scale by the vector.
     *
     * \param e Entities
     * \param delta Scale vectors
     * \param count Number of entities
     */
    void TransformationTable::scale(Entity *e, glm::vec3 *delta, size_t count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, e++, delta++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.scale *= *delta;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Set position entities
     *
     * Set position of givne entities.
     * Updates both the position and the matrix.
     *
     * \param e Entities
     * \param position New position vectors
     * \param count Number of entities
     */
    void TransformationTable::set_position(Entity *e, glm::vec3 *position, size_t count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, e++, position++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.position = *position;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Set orientation of entities
     *
     * Set orientation of given entities.
     * Updates both the orientation and the matrix.
     *
     * \param e Entities
     * \param orientation New orientation quaternions
     * \param count Number of entities
     */
    void TransformationTable::set_orientation(Entity *e, glm::quat *orientation, size_t count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, e++, orientation++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.orientation = *orientation;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Set scale of entities
     *
     * Set scale of givne entities.
     * Updates both the scale and the matrix.
     *
     * \param e Entities
     * \param scale New scale vectors
     * \param count Number of entities
     */
    void TransformationTable::set_scale(Entity *e, glm::vec3 *scale, size_t count) {
        // For every index, set the value
        for (unsigned j = 0; j < count; j++, e++, scale++) {
            // Get reference
            Data::Ptr ref{};
            try {
                ref = table->get_reference(*e);
            } catch (std::out_of_range &e) {
                // Not found -> skip
                continue;
            }

            // Set the value
            *ref.scale = *scale;

            // Update matrix
            update_matrix(*e);
        }
    }

    /**
     * \brief Show ImGui debug information
     */
    void TransformationTable::show_debug() {
        ImGui::Text("Table type: %s", table->type_name());
        ImGui::Text("Record size: %lu bytes", sizeof(Data));
        ImGui::Text("Records: %lu (%lu bytes)", table->size(), sizeof(Data) * table->size());
        ImGui::Text("Allocated: %lu (%lu bytes)", table->allocated(), sizeof(Data) * table->allocated());
        ImGui::Text("Pages allocated: %lu", table->pages());
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
    void TransformationTable::show_query() {
        ImGui::TextUnformatted("Entity:");
        ImGui::InputInt2("index - generation", query_idx_gen);
        if (ImGui::Button("Refresh")) {
            if (query_idx_gen[0] >= 0 && query_idx_gen[1] >= 0) {
                try {
                    Entity entity(static_cast<unsigned>(query_idx_gen[0]), static_cast<unsigned>(query_idx_gen[1]));
                    query_entity = entity;
                    query_buffer = table->get_copy(entity);
                    query_success = true;
                } catch (std::out_of_range&) {
                    query_buffer = {};
                    query_success = false;
                }
            } else {
                query_buffer = {};
                query_success = false;
            }
        }
        ImGui::Separator();
        if (query_success) {
            ImGui::Text("Position: %.3f, %.3f, %.3f", query_buffer.position.x, query_buffer.position.y, query_buffer.position.z);
            ImGui::TextUnformatted("Orientation:");
            ImGui::SameLine();
            debug::show_quat(query_buffer.orientation);
            ImGui::Text("Scale: %.3f, %.3f, %.3f", query_buffer.scale.x, query_buffer.scale.y, query_buffer.scale.z);
            debug::show_matrix(query_buffer.matrix);
            ImGui::Text("Parent: %s", (!query_buffer.parent.is_set()) ? "none" : table->lookup(query_buffer.parent).str().c_str());
            ImGui::Text("First child: %s", (!query_buffer.first_child.is_set()) ? "none" : table->lookup(query_buffer.first_child).str().c_str());
            ImGui::Text("Next sibling: %s", (!query_buffer.next_sibling.is_set()) ? "none" : table->lookup(query_buffer.next_sibling).str().c_str());
            ImGui::Text("Previous sibling: %s", (!query_buffer.prev_sibling.is_set()) ? "none" : table->lookup(query_buffer.prev_sibling).str().c_str());
            ImGui::Spacing();
            if (ImGui::Button("Set Position")) {
                query_pos = query_buffer.position;
                ImGui::OpenPopup("set position");
            } ImGui::SameLine();
            if (ImGui::Button("Set Orientation")) {
                query_ori = query_buffer.orientation;
                ImGui::OpenPopup("set orientation");
            } ImGui::SameLine();
            if (ImGui::Button("Set Scale")) {
                query_sca = query_buffer.scale;
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
                set_position(&query_entity, &query_pos, 1);
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
                set_orientation(&query_entity, &query_ori, 1);
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
                set_scale(&query_entity, &query_sca, 1);
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
                translate(&query_entity, &query_pos_delta, 1);
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
                rotate(&query_entity, &query_ori_delta, 1);
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
                scale(&query_entity, &query_sca_fac, 1);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    //--- End TransformationTable implementation
}
