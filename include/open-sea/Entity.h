/** \file Entity.h
 * Entity Component System module - Entity definition and utilities
 *
 * \author Filip Smola
 */
// Heavily inspired by niklasfrykholm's articles at https://github.com/niklasfrykholm/blog/tree/master/2014
#ifndef OPEN_SEA_ENTITY_H
#define OPEN_SEA_ENTITY_H

#include <vector>
#include <cstdint>
#include <queue>

namespace open_sea::ecs {
    // Notes:
    //  - Supports ~4 million concurrent entities
    //  - Supports 1024 generations before a previously used handle reappears
    //  - There should never be two entities alive with the same index and generation
    //  - Of two entities with equal index and distinct generation, the one with greater generation is considered alive
    //  - Entity ID layout: (10 bits of generation)(22 bits of index)

    //! Handle type alias
    typedef uint32_t handle;

    //! Number of bits in entity index
    constexpr unsigned ENTITY_INDEX_BITS = 22;
    //! Mask for extracting entity index from handle
    constexpr handle ENTITY_INDEX_MASK = (1 << ENTITY_INDEX_BITS) - 1;    // 22 1s, padded to left with 0s
    //! Number of bits in entity generation
    constexpr unsigned ENTITY_GENERATION_BITS = 10;
    //! Mask for extracting entity generation from handle (after shifting away the index)
    constexpr handle ENTITY_GENERATION_MASK = (1 << ENTITY_GENERATION_BITS) - 1;  // 10 1s, padded to left with 0s

    struct Entity {
        //! ID of this entity
        handle id;

        //! Get index of this entity
        unsigned index() const { return id & ENTITY_INDEX_MASK; }
        //! Get generation of this entity
        unsigned generation() const { return (id >> ENTITY_INDEX_BITS) & ENTITY_GENERATION_MASK; }
    };

    class EntityManager {
        private:
            //! Record of currently living (or the next one to live if none alive) generation in an index
            std::vector<uint16_t> generation;
            //! Queue of indices with latest generation dead
            std::queue<unsigned> freeIndices;
        public:
            // Debug info
            //! Number of entities alive
            unsigned livingEntities = 0;
            //! Maximum current generation
            uint16_t  maxGeneration = 0;
            //! Maximum current index
            unsigned maxIndex = 0;

            //! Minimum number of free indices in the queue before reusing from the queue
            // This means reuse of indices will be much more spread out and IDs will reappear much more rarely
            static constexpr unsigned MINIMUM_FREE_INDICES = 1024;

            Entity create();
            bool alive(Entity e) const;
            void kill(Entity e);

            void showDebug() const;
    };
}

#endif //OPEN_SEA_ENTITY_H
