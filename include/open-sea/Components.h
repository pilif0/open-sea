/** \file Components.h
 * Entity Component System module - Component definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_COMPONENTS_H
#define OPEN_SEA_COMPONENTS_H

#include <open-sea/Entity.h>
#include <open-sea/Log.h>
#include <open-sea/Debuggable.h>
#include <open-sea/Table.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>

// Forward declarations
namespace open_sea {
    namespace model {
        class Model;
    }

    namespace gl {
        class Camera;
    }
}

namespace open_sea::ecs {
    /**
     * \addtogroup Components
     * \brief Component managers of the ECS
     *
     * Component managers of the ECS.
     * Index can be \c int, because as long as at least one bit of the entity handles is used for generation it will fit.
     * Index \c -1 means that the record was not found.
     *
     * @{
     */
    // The -1 index is used instead of exceptions (contrary to code standard) to support actions on many entities at
    //  once.

    //! Default starting size of component managers
    constexpr unsigned default_size = 1;

    /** \class ModelTable
     * \brief Table-based model component manager
     */
     //TODO docs
    class ModelTable : public debug::Debuggable {
        public:
            /** \struct Data
             * Model component data is just the index into the model store
             */
            struct Data {
                static constexpr unsigned int count = 1;
                struct Ptr {
                    size_t *model;
                };

                size_t model;
            };
        private:
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Model Component Manager (Table)");
            //! Models used by components in this manager
            std::vector<std::shared_ptr<model::Model>> models;
        public:
            //! Table holding the components
            std::unique_ptr<data::Table<Entity, Data>> table;

            ModelTable() : ModelTable(default_size) {}
            explicit ModelTable(unsigned size);

            size_t model_to_index(const std::shared_ptr<model::Model>& model);
            std::shared_ptr<model::Model> get_model(size_t i) const;

            void gc(const EntityManager &manager);

            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            Data::Ptr query_ref {nullptr};
            void show_query();

            ~ModelTable() override;
    };

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
            ModelComponent() : ModelComponent(default_size) {}
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
            static constexpr unsigned record_size = sizeof(Entity) + sizeof(int);
            //! Allocator used by the manager
            std::allocator<unsigned char> allocator;
            int model_to_index(std::shared_ptr<model::Model> model);
            std::shared_ptr<model::Model> get_model(int i) const;

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

            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            int query_idx = -1;
            void show_query();
    };

    /** \class TransformationTable
     * \brief Table-based transformation component manager
     */
    // Note: only time when indices can change is on removal of a record (last item gets moved), so using opt_index for
    //          the tree structure fields is safe as long as the affected ones get adjusted on removal
    //TODO docs
    //TODO a lot of the structure-preserving algorithms could probably be done better
    class TransformationTable : public debug::Debuggable {
        public:
            /** \struct Data
             * Transformation component data has the three transformations, their matrix, and tree-related attributes
             */
            struct Data {
                static constexpr unsigned int count = 8;
                struct Ptr {
                    glm::vec3 *position = nullptr;
                    glm::quat *orientation = nullptr;
                    glm::vec3 *scale = nullptr;
                    glm::mat4 *matrix = nullptr;
                    data::opt_index *parent = nullptr;
                    data::opt_index *first_child = nullptr;
                    data::opt_index *next_sibling = nullptr;
                    data::opt_index *prev_sibling = nullptr;
                };

                //! Local position
                glm::vec3 position;
                //! Local orientation
                glm::quat orientation;
                //! Local scale
                glm::vec3 scale;
                //! World matrix
                glm::mat4 matrix;
                //! Parent index (unset if root)
                data::opt_index parent;
                //! Index of first child (unset if none)
                data::opt_index first_child;
                //! Index of next sibling (unset if none)
                data::opt_index next_sibling;
                //! Index of previous sibling (unset if none)
                data::opt_index prev_sibling;
            };
        private:
            //! Logger for this manager
            log::severity_logger lg = log::get_logger("Transformation Component Manager (Table)");
        public:
            //! Table holding the components
            std::unique_ptr<data::Table<Entity, Data>> table;

            TransformationTable() : TransformationTable(default_size) {}
            explicit TransformationTable(unsigned size);


            // Structure preserving table modifiers
            bool add(const Entity &key, const glm::vec3 &position, const glm::quat &orientation, const glm::vec3 &scale, data::opt_index parent);
            bool add(const Entity *keys, const glm::vec3 *position, const glm::quat *orientation, const glm::vec3 *scale, data::opt_index parent, size_t count);
            bool remove(data::opt_index idx);
            bool remove(const Entity &key);
            void adopt(const Entity &e, data::opt_index parent = {});
            void update_matrix(data::opt_index idx, bool siblings = false);
            void update_matrix(Entity e, bool siblings = false);

            // Transformations
            // Note: these each update both the relevant value and the matrix
            void translate(Entity *e, glm::vec3 *delta, size_t count);
            void rotate(Entity *e, glm::quat *delta, size_t count);
            void scale(Entity *e, glm::vec3 *delta, size_t count);
            void set_position(Entity *e, glm::vec3 *position, size_t count);
            void set_orientation(Entity *e, glm::quat *orientation, size_t count);
            void set_scale(Entity *e, glm::vec3 *scale, size_t count);

            // Debug
            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            bool query_success = false;
            Entity query_entity{};
            Data query_buffer{};
            glm::vec3 query_pos{};
            glm::quat query_ori{};
            glm::vec3 query_sca{};
            glm::vec3 query_pos_delta{};
            glm::quat query_ori_delta{};
            glm::vec3 query_sca_fac{};
            void show_query();

            void gc(const EntityManager &manager);
            ~TransformationTable() override = default;
    };

    /** \class TransformationComponent
     * \brief Associates an entity with a transformation relative to some parent
     *
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
            TransformationComponent() : TransformationComponent(default_size) {}
            explicit TransformationComponent(unsigned size);

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
                int *first_child = nullptr;
                //! Index of next sibling (-1 if none)
                int *next_sibling = nullptr;
                //! Index of previous sibling (-1 if none)
                int *prev_sibling = nullptr;
            };
            InstanceData data{};
            void allocate(unsigned size);
            //! Size of one record in bytes
            static constexpr unsigned record_size = sizeof(Entity) + 2 * sizeof(glm::vec3) + sizeof(glm::quat) + sizeof(glm::mat4) + 4 * sizeof(int);
            //! Allocator used by the manager
            std::allocator<unsigned char> allocator;

            // Access
            //! Map of entities to data indices
            std::unordered_map<Entity, int> map;
            int lookup(Entity e) const;
            void lookup(Entity *e, int *dest, unsigned count) const;
            void add(Entity *e, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count, int parent = -1);
            void set(int *i, glm::vec3 *position, glm::quat *orientation, glm::vec3 *scale, unsigned count);
            void adopt(int i, int parent = -1);
            void update_matrix(int i);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~TransformationComponent();

            // Transformation
            // Note: these each update both the relevant value and the matrix
            void translate(int *i, glm::vec3 *delta, unsigned count);
            void rotate(int *i, glm::quat *delta, unsigned count);
            void scale(int *i, glm::vec3 *delta, unsigned count);
            void set_position(int *i, glm::vec3 *position, unsigned count);
            void set_orientation(int *i, glm::quat *orientation, unsigned count);
            void set_scale(int *i, glm::vec3 *scale, unsigned count);

            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            int query_idx = -1;
            glm::vec3 query_pos{};
            glm::quat query_ori;
            glm::vec3 query_sca;
            glm::vec3 query_pos_delta{};
            glm::quat query_ori_delta;
            glm::vec3 query_sca_fac{};
            void show_query();
    };

    // Note: There are issues with using the tables to store the camera component. These highlighted that camera isn't
    //  really a component of an entity. I am instead making them a thing that the game state holds and uses to render
    //  through. The method of following entities will instead wrap around a camera and be directly udated, instead of
    //  being an ECS system.

    /** \class CameraComponent
     * \brief Associates an entity with a camera
     *
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
            CameraComponent() : CameraComponent(default_size) {}
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
            static constexpr unsigned record_size = sizeof(Entity) + sizeof(std::shared_ptr<gl::Camera>);
            //! Allocator used by the manager
            std::allocator<unsigned char> allocator;

            // Access
            int lookup(Entity e) const;
            void lookup(Entity *e, int *dest, unsigned count) const;
            void add(Entity *e, std::shared_ptr<gl::Camera> *c, unsigned count);
            void set(int *i, std::shared_ptr<gl::Camera> *c, unsigned count);

            // Maintenance
            void destroy(int i);
            void gc(const EntityManager &manager);
            ~CameraComponent();

            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            std::vector<std::shared_ptr<gl::Camera>> query_cameras;
            void show_query();
    };

    /**
     * @}
     */
}

// Table-related member macros
SOA_MEMBER(open_sea::ecs::ModelTable::Data, 0, size_t, model)

SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 0, glm::vec3, position)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 1, glm::quat, orientation)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 2, glm::vec3, scale)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 3, glm::mat4, matrix)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 4, open_sea::data::opt_index, parent)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 5, open_sea::data::opt_index, first_child)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 6, open_sea::data::opt_index, next_sibling)
SOA_MEMBER(open_sea::ecs::TransformationTable::Data, 7, open_sea::data::opt_index, prev_sibling)

#endif //OPEN_SEA_COMPONENTS_H
