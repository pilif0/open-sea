/** \file Entity.h
 * Entity Component System module - Entity definition and utilities
 *
 * \author Filip Smola
 */
// Heavily inspired by niklasfrykholm's articles at https://github.com/niklasfrykholm/blog/tree/master/2014
#ifndef OPEN_SEA_ENTITY_H
#define OPEN_SEA_ENTITY_H

#include <open-sea/Log.h>
#include <open-sea/Debuggable.h>

#include <vector>
#include <cstdint>
#include <queue>

//! %Entity Component System namespace
namespace open_sea::ecs {
    /**
     * \addtogroup Entity
     * \brief %Entity representation
     *
     * %Entity Component System core, the Entity representation.
     * A handle is an unsigned 32 bit integer split into index (22 bits) and generation (10 bits).
     * Supports up to 2^22 concurrent entities.
     * Each index can be reused 2^10 times until the handle repeats.
     * An entity is considered alive iff its generation is equal to the generation at its index in the manager.
     * Handle layout is first generation and then index.
     *
     * @{
     */

    //! Handle type alias
    typedef uint32_t handle;

    //! Number of bits in entity index
    constexpr unsigned entity_index_bits = 22;
    //! Mask for extracting entity index from handle
    constexpr handle entity_index_mask = (1 << entity_index_bits) - 1;    // 22 1s, padded to left with 0s
    //! Number of bits in entity generation
    constexpr unsigned entity_generation_bits = 10;
    //! Mask for extracting entity generation from handle (after shifting away the index)
    constexpr handle entity_generation_mask = (1 << entity_generation_bits) - 1;  // 10 1s, padded to left with 0s

    //! Entity represented by its handle
    struct Entity {
        //! Handle of this entity
        handle id;

        Entity() {}

        //! Construct an entity from index and generation
        Entity(unsigned index, unsigned generation) { id = (generation << entity_index_bits) + index; }

        //! Get index of this entity
        unsigned index() const { return id & entity_index_mask; }
        //! Get generation of this entity
        unsigned generation() const { return (id >> entity_index_bits) & entity_generation_mask; }

        bool operator==(const Entity &rhs) const { return id == rhs.id; }
        bool operator!=(const Entity &rhs) const { return !(rhs == *this); }

        std::string str() const;
    };

    /** \class EntityManager
     * \brief Manages which entities are considered alive
     *
     * Manages which entities are alive by keeping a record of the generation at each index.
     * Only reuses indices (incrementing the generation) when there is a certain number available, thus spreading the
     *  index use more evenly and making full handle reuse less likely.
     */
    class EntityManager : public debug::Debuggable {
        private:
            //! Record of currently living (or the next one to live if none alive) generation in an index
            std::vector<uint16_t> generation;
            //! Queue of indices with latest generation dead
            std::queue<unsigned> freeIndices;
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Entity Manager");
        public:
            // Debug info
            //! Number of entities alive
            unsigned living_entities = 0;
            //! Maximum current generation
            uint16_t  max_generation = 0;
            //! Maximum current index
            unsigned max_index = 0;

            //! Minimum number of free indices in the queue before reusing from the queue
            // This means reuse of indices will be much more spread out and IDs will reappear much more rarely
            static constexpr unsigned minimum_free_indices = 1024;

            Entity create();
            void create(Entity* dest, unsigned count);
            bool alive(Entity e) const;
            void kill(Entity e);

            void show_debug() override;
    };

    /**
     * @}
     */
}

// Hash function for entities
template<>
struct std::hash<open_sea::ecs::Entity> {
    std::size_t operator()(const open_sea::ecs::Entity& e) const;
};

#endif //OPEN_SEA_ENTITY_H
