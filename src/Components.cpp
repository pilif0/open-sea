/*
 * Components implementation
 */

#include <open-sea/Components.h>

#include <stdexcept>
#include <random>
#include <sstream>

namespace open_sea::ecs {
    // Notes:
    //  - I am fairly certain that after calling reset on a shared pointer, I can just deallocate the memory without
    //      having to explicitly call its destructor (as reset will take care of the use count)

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

        std::ostringstream message;
        message << "Allocating " << byteCount << " bytes for " << size << " records";
        log::log(lg, log::debug, message.str());

        newData.buffer = ALLOCATOR.allocate(byteCount);
        newData.n = data.n;
        newData.allocated = size;

        // Compute pointers to data arrays
        newData.entity = (Entity*) newData.buffer;                                  // Entity is at the start
        newData.model = (std::shared_ptr<model::Model>*) (newData.entity + size);     // followed by model pointers

        // Copy data into new space
        if (data.n > 0) {
            std::memcpy(newData.entity, data.entity, data.n * sizeof(Entity));
            std::memcpy(newData.model, data.model, data.n * sizeof(std::shared_ptr<model::Model>));


            // Deallocate old data
            ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
        }

        // Set the data
        data = newData;
        std::ostringstream message2;
        message2 << "Allocated addresses from " << data.buffer;
        log::log(lg, log::debug, message2.str());
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
     * Look up indices of the entities in the data arrays and writes them into the destination.
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
     *
     * \param e Entities
     * \param m Models to tie entities with (in the same order)
     * \param count Number of entities
     */
    void ModelComponent::add(Entity *e, std::shared_ptr<model::Model> *m, unsigned count) {
        // Check data has enough space
        if (data.allocated < (data.n + count)) {
            // Too small -> reallocate
            //TODO is there a better increment?
            allocate(data.n + count);
        }

        // For every entity, add a new record
        Entity *destE = data.entity + data.n;
        std::shared_ptr<model::Model> *destM = data.model + data.n;
        for (unsigned i = 0; i < count; i++, e++, m++, destE++, destM++) {
            //TODO: check that the entity doesn't yet have this component?

            std::ostringstream message;
            message << "Adding record entity at " << destE << "(" << data.entity << " + " << data.n << "*"<< sizeof(Entity)
                    <<") and model at " << destM << "("<<data.model<<" + "<<data.n<<"*"<<sizeof(std::shared_ptr<model::Model>) << ")";
            log::log(lg, log::debug, message.str());

            // Write the data into the buffer
            *destE = *e;
            ::new (destM) std::shared_ptr<model::Model>(*m);

            // Increment data count and add map entry for the new record
            data.n++;
            map[*e] = data.n - 1;
        }
    }

    /**
     * \brief Set the models at the indices
     * Set the models at the indices.
     * If an index is out of range (greater than or equal to \c data.n), nothing is set (points either to destroyed record
     * or out of allocated range for that data array)
     *
     * \param i Indices into the data arrays
     * \param m Models to assign
     * \param count Number of indices
     */
    void ModelComponent::set(int *i, std::shared_ptr<model::Model> *m, unsigned count) {
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

        // Reset shared pointer
        data.model[lastIdx].reset();

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
        // Reset shared pointers to models
        std::shared_ptr<model::Model> *m = data.model;
        for (int i = 0; i < data.n; i++, m++) {
            m->reset();
        }

        // Deallocate the data buffer
        ALLOCATOR.deallocate(static_cast<unsigned char *>(data.buffer), data.allocated * RECORD_SIZE);
    }
    //--- end ModelComponent implementation
    //--- start TransformationComponent implementation

    //--- end TransformationComponent implementation
}
