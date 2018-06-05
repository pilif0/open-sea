/*
 * Entity implementation
 */

#include <open-sea/Entity.h>
#include <open-sea/ImGui.h>

#include <stdexcept>

namespace open_sea::ecs {

    //--- start Entity implementation
    /**
     * \brief Get string representation of the entity as "index|generation"
     *
     * \return String representation
     */
    std::string Entity::str() const {
        std::ostringstream result;
        result << index() << "|" << generation();
        return result.str();
    }
    //--- end Entity implementation
    //--- start EntityManager implementation
    /**
     * \brief Create a new entity
     *
     * \return Created entity
     * \throw \c std::runtime_error when there is no available index for new entity
     */
    Entity EntityManager::create() {
        unsigned index;

        // Find appropriate index
        if (freeIndices.size() > MINIMUM_FREE_INDICES) {
            // Reuse a freed index
            index = freeIndices.front();
            freeIndices.pop();
        } else {
            // Check there is at least one more available index
            if (generation.size() < (1 << ENTITY_INDEX_BITS)) {
                // Index available -> use the next one
                generation.push_back(0);
                index = generation.size() - 1;
            } else {
                // Index not available -> attempt to reuse some
                if (!freeIndices.empty()) {
                    // Free index available -> reuse it
                    index = freeIndices.front();
                    freeIndices.pop();
                } else {
                    // No index available at all -> unable to create a new entity
                    log::log(lg, log::error, "No available index for new entity");
                    throw std::runtime_error("No available index for new entity");
                }
            }
        }

        // Update debug information
        livingEntities++;
        maxIndex = (index > maxIndex) ? index : maxIndex;
        auto gen = generation[index];
        maxGeneration = (gen > maxGeneration) ? gen : maxGeneration;

        return Entity(index, gen);
    }

    /**
     * \brief Create multiple new entities
     *
     * \param dest Destination for created entities
     * \param count Number of entities to create
     */
    void EntityManager::create(Entity *dest, unsigned count) {
        for (int i = 0; i < count; i++, dest++) {
            *dest = create();
        }
    }

    /**
     * \brief Get whether the entity is alive
     * Get whether the entity is alive.
     * An entity is considered alive iff its generation matches the generation at its index.
     *
     * \param e Entity to check
     * \return \c true when alive, \c false otherwise
     */
    bool EntityManager::alive(Entity e) const {
        return generation[e.index()] == e.generation();
    }

    /**
     * \brief Kill (destroy) an entity
     *
     * \param e Entity to kill
     */
    void EntityManager::kill(Entity e) {
        // Increase generation and add to free indices
        const unsigned index = e.index();
        generation[index]++;
        freeIndices.push(index);
        livingEntities--;
    }

    /**
     * \brief Show ImGui debug information
     */
    void EntityManager::showDebug() {
        ImGui::Text("Living entities: %i", livingEntities);
        ImGui::Text("Maximum generation: %i", maxGeneration);
        ImGui::Text("Maximum index: %i", maxIndex);
        ImGui::Text("Free indices: %i", freeIndices.size());
    }
    //--- end EntityManager implementation
}

// Entity hash function
std::size_t std::hash<open_sea::ecs::Entity>::operator()(const open_sea::ecs::Entity &e) const {
    // Hash the id
    return std::hash<int>()(e.id);
}
