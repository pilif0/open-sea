/** \file Components.h
 * Entity Component System module - Component definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_COMPONENTS_H
#define OPEN_SEA_COMPONENTS_H

#include <open-sea/Entity.h>
#include <open-sea/Model.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>

namespace open_sea::ecs {
    // Notes:
    //  - Index -1 means that the component for that entity was not found
    //  - Index can be represented by int, because it will always fit into non-negative int range (as long as at least
    //      one bit is used for generation)
    //  - If length of parameters for a transformation doesn't match count, loop back to start (i.e. act similarly to
    //      Matlab's bsxfun)

    //TODO: find out whether using maps to keep the indices is worth the space they use in the speedup they produce

    /** \class ModelComponent
     * \brief Associates an entity with a model
     */
    class ModelComponent {
        public:
            ModelComponent();
            ModelComponent(unsigned size);

            // Data
            //! Data of component instances
            struct InstanceData {
                //! Number of used instances
                unsigned n;
                //! Number of allocated instances
                unsigned allocated;
                //! Buffer with data
                void *buffer;

                //! Associated entity
                Entity *entity;
                //! Pointer to associated model
                std::shared_ptr<model::Model> *model;
            };
            InstanceData data;
            void allocate(unsigned size);

            // Access
            //! Map of entities to data indices
            std::unordered_map<Entity, int> map;
            int lookup(Entity e) const;
            int* lookup(Entity *e, unsigned count) const;
            int* add(Entity *e, std::shared_ptr<model::Model> *m, unsigned count);
            void set(int *i, std::shared_ptr<model::Model> *m, unsigned count);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~ModelComponent();
    };

    /** \class TransformationComponent
     * \breif Associates an entity with a world transformation
     */
    //TODO: add tree capabilities (parents, children, siblings, local x world transformation)
    class TransformationComponent {
        public:
            TransformationComponent();
            TransformationComponent(unsigned size);

            // Data
            //! Data of component instances
            struct InstanceData {
                //! Number of used instances
                unsigned n;
                //! Number of allocated instances
                unsigned allocated;
                //! Buffer with data
                void *buffer;

                //! Associated entity
                Entity *entity;
                //! World position
                glm::vec3 *position;
                //! World orientation
                glm::quat *orientation;
                //! World scale
                glm::vec3 *scale;
                //! World matrix
                glm::mat4 *matrix;
            };
            InstanceData data;
            void allocate(unsigned size);

            // Access
            //! Map of entities to data indices
            std::unordered_map<Entity, int> map;
            int lookup(Entity e) const;
            int* lookup(Entity *e, unsigned count) const;
            int* add(Entity *e, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count);
            void set(int *i, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~TransformationComponent();

            // Transformation
            // Note: these each update both the relevant value and the matrix
            void translate(int *i, glm::vec3 *delta, unsigned count);
            void rotate(int *i, glm::quat *dela, unsigned count);
            void scale(int *i, glm::vec3 *delta, unsigned count);
            void setPosition(int *i,  glm::vec3 *position, unsigned count);
            void setOrientation(int *i, glm::quat *orientation, unsigned count);
            void setScale(int *i, glm::vec3 *scale, unsigned count);
    };
}

#endif //OPEN_SEA_COMPONENTS_H
