/** \file Components.h
 * Entity Component System module - Component definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_COMPONENTS_H
#define OPEN_SEA_COMPONENTS_H

#include <open-sea/Entity.h>
#include <open-sea/Model.h>
#include <open-sea/Log.h>
#include <open-sea/GL.h>
#include <open-sea/Debuggable.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>

namespace open_sea::ecs {
    // Notes:
    //  - Index -1 means that the component for that entity was not found
    //  - Index can be represented by int, because it will always fit into non-negative int range (as long as at least
    //      one bit of the entity handles is used for generation)

    //! Default starting size of component managers
    constexpr unsigned DEFAULT_SIZE = 1;

    /** \class ModelComponent
     * \brief Associates an entity with a model
     */
    class ModelComponent : public debug::Debuggable {
        private:
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Model Component Mgr");
            //! Models used by components in this manager
            std::vector<std::shared_ptr<model::Model>> models;
        public:
            ModelComponent() : ModelComponent(DEFAULT_SIZE) {}
            explicit ModelComponent(unsigned size);

            // Data
            //! Data of component instances
            struct InstanceData {
                //! Number of used instances
                unsigned n = 0;
                //! Number of allocated instances
                unsigned allocated = 0;
                //! Buffer with data
                void *buffer = nullptr;

                //! Associated entity
                Entity *entity = nullptr;
                //! Index of associated model
                int *model = nullptr;
            };
            InstanceData data{};
            void allocate(unsigned size);
            //! Size of one record in bytes
            static constexpr unsigned RECORD_SIZE = sizeof(Entity) + sizeof(int);
            //! Allocator used by the manager
            std::allocator<unsigned char> ALLOCATOR;
            int modelToIndex(std::shared_ptr<model::Model> model);
            std::shared_ptr<model::Model> getModel(int i);

            // Access
            //! Map of entities to data indices
            std::unordered_map<Entity, int> map;
            int lookup(Entity e) const;
            void lookup(Entity *e, int *dest, unsigned count) const;
            void add(Entity *e, std::shared_ptr<model::Model> *m, unsigned count);
            void add(Entity *e, int *m, unsigned count);
            void set(int *i, std::shared_ptr<model::Model> *m, unsigned count);
            void set(int *i, int *m, unsigned count);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~ModelComponent();

            void showDebug() override;
            int queryIdxGen[2] {0, 0};
            int queryModelId = -1;
            std::shared_ptr<model::Model> queryModel{};
            void showQuery();
    };

    /** \class TransformationComponent
     * \breif Associates an entity with a transformation relative to some parent
     * Associates an entity with a local transformation relative to some parent.
     * If the entity has no parent (index -1) then it is considered a root and its transformation is relative to identity.
     * Multiple root entities are not considered siblings.
     */
    class TransformationComponent : public debug::Debuggable {
        private:
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Transformation Component Mgr");
            void swap(int i, int j);

        public:
            TransformationComponent() : TransformationComponent(DEFAULT_SIZE) {}
            TransformationComponent(unsigned size);

            // Data
            //! Data of component instances
            struct InstanceData {
                //! Number of used instances
                unsigned n = 0;
                //! Number of allocated instances
                unsigned allocated = 0;
                //! Buffer with data
                void *buffer = nullptr;

                //! Associated entity
                Entity *entity = nullptr;
                //! Local position
                glm::vec3 *position = nullptr;
                //! Local orientation
                glm::quat *orientation = nullptr;
                //! Local scale
                glm::vec3 *scale = nullptr;
                //! World matrix
                glm::mat4 *matrix = nullptr;
                //! Parent index (-1 if root)
                int *parent = nullptr;
                //! Index of first child (-1 if none)
                int *firstChild = nullptr;
                //! Index of next sibling (-1 if none)
                int *nextSibling = nullptr;
                //! Index of previous sibling (-1 if none)
                int *prevSibling = nullptr;
            };
            InstanceData data{};
            void allocate(unsigned size);
            //! Size of one record in bytes
            static constexpr unsigned RECORD_SIZE = sizeof(Entity) + 2 * sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(glm::mat4) + 4 * sizeof(int);
            //! Allocator used by the manager
            std::allocator<unsigned char> ALLOCATOR;

            // Access
            //! Map of entities to data indices
            std::unordered_map<Entity, int> map;
            int lookup(Entity e) const;
            void lookup(Entity *e, int *dest, unsigned count) const;
            void add(Entity *e, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count, int parent = -1);
            void set(int *i, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count);
            void adopt(int i, int parent = -1);
            void updateMatrix(int i);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~TransformationComponent();

            // Transformation
            // Note: these each update both the relevant value and the matrix
            void translate(int *i, glm::vec3 *delta, unsigned count);
            void rotate(int *i, glm::quat *delta, unsigned count);
            void scale(int *i, glm::vec3 *delta, unsigned count);
            void setPosition(int *i,  glm::vec3 *position, unsigned count);
            void setOrientation(int *i, glm::quat *orientation, unsigned count);
            void setScale(int *i, glm::vec3 *scale, unsigned count);

            void showDebug() override;
    };

    /** \class CameraComponent
     * \brief Associates an entity with a camera
     * Associates an entity with a camera.
     * Each camera should have at most one entity associated with it (otherwise following it might be unpredictable).
     * One entity can have multiple associated cameras.
     */
    //TODO: maybe add a method to retrieve all indices of an entity instead of just first
    class CameraComponent : public debug::Debuggable {
        private:
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Camera Component Mgr");
        public:
            CameraComponent() : CameraComponent(DEFAULT_SIZE) {}
            explicit CameraComponent(unsigned size);

            // Data
            //! Data of component instances
            struct InstanceData {
                //! Number of used instances
                unsigned n = 0;
                //! Number of allocated instances
                unsigned allocated = 0;
                //! Buffer with data
                void *buffer = nullptr;

                //! Associated entity
                Entity *entity = nullptr;
                //! Shared pointer to camera
                std::shared_ptr<gl::Camera> *camera = nullptr;
            };
            InstanceData data{};
            void allocate(unsigned size);
            //! Size of one record in bytes
            static constexpr unsigned RECORD_SIZE = sizeof(Entity) + sizeof(std::shared_ptr<gl::Camera>);
            //! Allocator used by the manager
            std::allocator<unsigned char> ALLOCATOR;

            // Access
            int lookup(Entity e) const;
            void lookup(Entity *e, int *dest, unsigned count) const;
            void add(Entity *e, std::shared_ptr<gl::Camera> *c, unsigned count);
            void set(int *i, std::shared_ptr<gl::Camera> *c, unsigned count);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~CameraComponent();

            void showDebug() override;
    };
}

#endif //OPEN_SEA_COMPONENTS_H
