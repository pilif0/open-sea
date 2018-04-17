/*
 * Components implementation
 */

#include <open-sea/Components.h>

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
     * Allocate space for the given number of components
     *
     * \param size Number of components
     */
    void ModelComponent::allocate(unsigned size) {
        // Make sure data will fit into new space
        assert(size > data.n);

        // Allocate new space
        InstanceData newData;
        unsigned byteCount = size * RECORD_SIZE;

        newData.buffer = ALLOCATOR.allocate(byteCount);
        newData.n = data.n;
        newData.allocated = size;

        // Compute pointers to data arrays
        newData.entity = (Entity*) newData.buffer;          // Entity is at the start
        newData.model = (int*) (newData.entity + size);     // followed by model indices

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(newData.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(newData.model, data.model, data.n * sizeof(int));


            // Deallocate old data
            ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
        }

        // Set the data
        data = newData;
    }

    /**
     * \brief Get index to a model
     * Get index to a model, adding the model to the manager's storage if necessary.
     *
     * \param model Pointer to model
     * \return Index into the \c models storage of this manager
     */
    int ModelComponent::modelToIndex(std::shared_ptr<model::Model> model) {
        int modelIdx;

        // Try to find the model in storage
        auto modelPos = std::find(models.begin(), models.end(), model);
        if (modelPos == models.end()) {
            // Not found -> add the pointer (passed by value, is a copy)
            modelIdx = static_cast<int>(models.size());
            models.push_back(model);
        } else {
            // Found -> use found index
            modelIdx = static_cast<int>(modelPos - models.begin());
        }

        return modelIdx;
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
     * \brief Add the component to the entities
     * Add the component to the entities.
     * The model pointers get converted to indices using \c modelToIndex .
     * The number of entities and models must match.
     *
     * \param e Entities
     * \param m Models to assign
     * \param count Number of entities
     */
    void ModelComponent::add(Entity *e, std::shared_ptr<model::Model> *m, unsigned count) {
        // Convert pointers to indices
        int indices[count];
        for (int *i = indices; i - indices < count; i++, m++) {
            *i = modelToIndex(*m);
        }

        return add(e, indices, count);
    }

    /**
     * \brief Add the component to the entities
     * Add the component to the entities.
     * The indices are passed naively and are not checked against the actual \c models storage.
     * The number of entities and indices must match.
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
        Entity *destE = data.entity + data.n;
        int *destM = data.model + data.n;
        for (unsigned i = 0; i < count; i++, e++, m++, destE++, destM++) {
            //TODO: check that the entity doesn't yet have this component?

            // Write the data into the buffer
            *destE = *e;
            *destM = *m;

            // Increment data count and add map entry for the new record
            data.n++;
            map[*e] = data.n - 1;
        }
    }

    /**
     * \brief Set the models at the indices
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
            *j = modelToIndex(*m);
        }

        set(i, indices, count);
    }

    /**
     * \brief Set the models at the indices
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
        for (int j = 0; j < count; j++, i++, m++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the value
            data.model[*i] = *m;
        }
    }

    /**
     * \brief Destroy the record at the index
     * Destroy the record at the index.
     * This can lead to reshuffling of the data arrays (to keep them tightly packed) and therefore invalidation of
     * previously looked up indices.
     *
     * \param i Index
     */
    void ModelComponent::destroy(int i) {
        // Check index is in range
        if (i >= data.n) {
            // Records out of range are considered already destroyed
            return;
        }

        // Move last record into i-th place
        int lastIdx = data.n - 1;
        Entity e = data.entity[i];
        Entity last = data.entity[lastIdx];
        data.entity[i] = data.entity[lastIdx];
        data.model[i] = data.model[lastIdx];

        // Update data counts and index map
        map.erase(e);
        map[last] = i;
        data.n--;
    }

    /**
     * \brief Collect garbage
     * Destroy records for dead entities.
     * This is done by checking random records until a certain number of live entities in a row are found.
     * Therefore with few dead entities not much time is wasted iterating through the array, and with many dead entities
     * they are destroyed within couple calls.
     *
     * \param manager Entity manager to check entities against
     */
    void ModelComponent::gc(const EntityManager &manager) {
        // Randomly check records until 4 live entities in a row are found
        unsigned aliveInRow = 0;
        static std::random_device device;
        static std::mt19937_64 generator;
        std::uniform_int_distribution<int> distribution(0, data.n);
        while (data.n > 0 && aliveInRow < 4) {
            // Note: % data.n required because data.n can change and this keeps indices in valid range
            unsigned i = distribution(generator) % data.n;
            if (manager.alive(data.entity[i])) {
                aliveInRow++;
            } else {
                aliveInRow = 0;
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
        ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
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
     * Allocate space for the given number of components
     *
     * \param size Number of components
     */
    void TransformationComponent::allocate(unsigned size) {
        // Make sure data will fit into new space
        assert(size > data.n);

        // Allocate new space
        InstanceData newData;
        unsigned byteCount = size * RECORD_SIZE;

        newData.buffer = ALLOCATOR.allocate(byteCount);
        newData.n = data.n;
        newData.allocated = size;

        // Compute pointers to data arrays
        newData.entity = (Entity*) newData.buffer;          // Entity is at the start
        newData.position = (glm::vec3*) (newData.entity + size);     // followed by position
        newData.orientation = (glm::quat*) (newData.position + size);     // followed by orientation
        newData.scale = (glm::vec3*) (newData.orientation + size);     // followed by scale
        newData.matrix = (glm::mat4*) (newData.scale + size);     // followed by matrix

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(newData.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(newData.position, data.position, data.n * sizeof(glm::vec3));
            std::memcpy(newData.orientation, data.orientation, data.n * sizeof(glm::quat));
            std::memcpy(newData.scale, data.scale, data.n * sizeof(glm::vec3));
            std::memcpy(newData.matrix, data.matrix, data.n * sizeof(glm::mat4));


            // Deallocate old data
            ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
        }

        // Set the data
        data = newData;
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
     * Add the component to the entities.
     * The number of each argument must match.
     *
     * \param e Entities
     * \param position Positions
     * \param orientation Orientations
     * \param scale Scales
     * \param count Number of entities
     */
    void TransformationComponent::add(Entity *e, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale,
                                      unsigned count) {
        // Check data has enough space
        if (data.allocated < (data.n + count)) {
            // Too small -> reallocate
            //TODO is there a better increment?
            allocate(data.n + count);
        }

        // For every entity, add a new record
        Entity *destE = data.entity + data.n;
        glm::vec3 *destP = data.position + data.n;
        glm::quat *destO = data.orientation + data.n;
        glm::vec3 *destS = data.scale + data.n;
        glm::mat4 *destM = data.matrix + data.n;
        for (unsigned i = 0; i < count; i++, e++, position++, orientation++, scale++,
                destE++, destP++, destO++, destS++, destM++) {
            //TODO: check that the entity doesn't yet have this component?

            // Write the data into the buffer
            *destE = *e;
            *destP = *position;
            *destO = *orientation;
            *destS = *scale;
            *destM = transformation(*position, *orientation, *scale);

            // Increment data count and add map entry for the new record
            data.n++;
            map[*e] = data.n - 1;
        }
    }

    /**
     * \brief Set the transformation at the indices
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
        for (int j = 0; j < count; j++, i++, position++, orientation++, scale++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.position[*i] = *position;
            data.orientation[*i] = *orientation;
            data.scale[*i] = *scale;
            data.matrix[*i] = transformation(*position, *orientation, *scale);
        }
    }

    /**
     * \brief Destroy the record at the index
     * Destroy the record at the index.
     * This can lead to reshuffling of the data arrays (to keep them tightly packed) and therefore invalidation of
     * previously looked up indices.
     *
     * \param i Index
     */
    void TransformationComponent::destroy(int i) {
        // Check index is in range
        if (i >= data.n) {
            // Records out of range are considered already destroyed
            return;
        }

        // Move last record into i-th place
        int lastIdx = data.n - 1;
        Entity e = data.entity[i];
        Entity last = data.entity[lastIdx];
        data.entity[i] = data.entity[lastIdx];
        data.position[i] = data.position[lastIdx];
        data.orientation[i] = data.orientation[lastIdx];
        data.scale[i] = data.scale[lastIdx];
        data.matrix[i] = data.matrix[lastIdx];

        // Update data counts and index map
        map.erase(e);
        map[last] = i;
        data.n--;
    }

    /**
     * \brief Collect garbage
     * Destroy records for dead entities.
     * This is done by checking random records until a certain number of live entities in a row are found.
     * Therefore with few dead entities not much time is wasted iterating through the array, and with many dead entities
     * they are destroyed within couple calls.
     *
     * \param manager Entity manager to check entities against
     */
    void TransformationComponent::gc(const EntityManager &manager) {
        // Randomly check records until 4 live entities in a row are found
        unsigned aliveInRow = 0;
        static std::random_device device;
        static std::mt19937_64 generator;
        std::uniform_int_distribution<int> distribution(0, data.n);
        while (data.n > 0 && aliveInRow < 4) {
            // Note: % data.n required because data.n can change and this keeps indices in valid range
            unsigned i = distribution(generator) % data.n;
            if (manager.alive(data.entity[i])) {
                aliveInRow++;
            } else {
                aliveInRow = 0;
                destroy(i);
            }
        }
    }

    /**
     * \brief Destroy the component manager, freeing up the used memory
     */
    TransformationComponent::~TransformationComponent() {
        // Deallocate the data buffer
        ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
    }

    /**
     * \brief Translate entities at indices
     * Translate entities at given indices.
     * Updates both the position and the matrix.
     *
     * \param i Indices
     * \param delta Translation vectors
     * \param count Number of indices
     */
    void TransformationComponent::translate(int *i, glm::vec3 *delta, unsigned count) {
        // For every index, set the value
        for (int j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.position[*i] += *delta;
            data.matrix[*i] = transformation(data.position[*i], data.orientation[*i], data.scale[*i]);
        }
    }

    /**
     * \brief Rotate entities at indices
     * Rotate entities at given indices.
     * Updates both the orientation and the matrix.
     *
     * \param i Indices
     * \param dela Rotation quaternions
     * \param count Number of indices
     */
    void TransformationComponent::rotate(int *i, glm::quat *delta, unsigned count) {
        // For every index, set the value
        for (int j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.orientation[*i] = *delta * data.orientation[*i];
            data.matrix[*i] = transformation(data.position[*i], data.orientation[*i], data.scale[*i]);
        }
    }

    /**
     * \brief Scale entities at indices
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
        for (int j = 0; j < count; j++, i++, delta++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.scale[*i] *= *delta;
            data.matrix[*i] = transformation(data.position[*i], data.orientation[*i], data.scale[*i]);
        }
    }

    /**
     * \brief Set position of entities at indices
     * Set position of entities at given indices.
     * Updates both the position and the matrix.
     *
     * \param i Indices
     * \param position New position vectors
     * \param count Number of indices
     */
    void TransformationComponent::setPosition(int *i, glm::vec3 *position, unsigned count) {
        // For every index, set the value
        for (int j = 0; j < count; j++, i++, position++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.position[*i] = *position;
            data.matrix[*i] = transformation(*position, data.orientation[*i], data.scale[*i]);
        }
    }

    /**
     * \brief Set orientation of entities at indices
     * Set orientation of entities at given indices.
     * Updates both the orientation and the matrix.
     *
     * \param i Indices
     * \param orientation New orientation quaternions
     * \param count Number of indices
     */
    void TransformationComponent::setOrientation(int *i, glm::quat *orientation, unsigned count) {
        // For every index, set the value
        for (int j = 0; j < count; j++, i++, orientation++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.orientation[*i] = *orientation;
            data.matrix[*i] = transformation(data.position[*i], *orientation, data.scale[*i]);
        }
    }

    /**
     * \brief Set scale of entities at indices
     * Set scale of entities at given indices.
     * Updates both the scale and the matrix.
     *
     * \param i Indices
     * \param scale New scale vectors
     * \param count Number of indices
     */
    void TransformationComponent::setScale(int *i, glm::vec3 *scale, unsigned count) {
        // For every index, set the value
        for (int j = 0; j < count; j++, i++, scale++) {
            int index = *i;

            // Check the index is in range
            if (index >= data.n) {
                // Setting value of destroyed records should not have any effect
                return;
            }

            // Set the values
            data.scale[*i] = *scale;
            data.matrix[*i] = transformation(data.position[*i], data.orientation[*i], *scale);
        }
    }
    //--- end TransformationComponent implementation
}
