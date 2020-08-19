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
namespace open_sea::model {
    class Model;
}

namespace open_sea::ecs {
    /**
     * \addtogroup Components
     * \brief Component managers of the ECS
     *
     * @{
     */

    //! Default starting size of component managers
    constexpr unsigned default_size = 1;

    /** \class ModelTable
     * \brief Associates a model with the entity
     *
     * Takes shared ownership of the models.
     * Entities are associated with a private identifier for the model, which can then be used to look up the relevant
     *  model pointer instance.
     */
    class ModelTable : public debug::Debuggable {
        public:
            /** \struct Data
             * Model component data is just the index into the model store
             */
            struct Data {
                static constexpr size_t count = 1;
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
            std::shared_ptr<model::Model> get_model(Entity e) const;
            bool remove_model(size_t i);

            void gc(const EntityManager &manager);

            void show_debug() override;
            int query_idx_gen[2] {0, 0};
            Data::Ptr query_ref {nullptr};
            void show_query();

            ~ModelTable() override;
    };


    /** \class TransformationTable
     * \brief Associates an entity with a transformation relative to some parent
     *
     * The transformation is composed of translation, rotation and scaling, taken relative to parent.
     * The parent is optional, with empty parent referring to the global origin.
     * Doubly linked between siblings and parent with first child.
     * Singly linked from non-first child to parent.
     * Caches the transformation matrix relative to global origin.
     * Any transformation recomputes the matrix of the transformed entity and recursively all its descendants.
     */
    // Note: only time when indices can change is on removal of a record (last item gets moved), so using opt_index for
    //          the tree structure fields is safe as long as the affected ones get adjusted on removal
    //TODO a lot of the structure-preserving algorithms could probably be done better
    class TransformationTable : public debug::Debuggable {
        public:
            /** \struct Data
             * Transformation component data has the three transformations, their matrix, and tree-related attributes
             */
            struct Data {
                static constexpr size_t count = 8;
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
