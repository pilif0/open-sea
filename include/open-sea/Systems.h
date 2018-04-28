/** \file Systems.h
 * Entity Component System module - General system definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_SYSTEMS_H
#define OPEN_SEA_SYSTEMS_H

#include <open-sea/Components.h>
#include <open-sea/GL.h>

namespace open_sea::ecs {

    /** \class CameraFollow
     * \brief System that makes GL cameras follow entities they have assigned
     */
    //Note: if the assigned entity doesn't have the transformation component, identity transformation will be assumed
    class CameraFollow {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Camera component manager
            std::shared_ptr<ecs::CameraComponent> cameraMgr{};

            //! Transformation from one entity
            struct EntityData {
                glm::vec3 position;
                glm::quat orientation;
            };

            //! Construct the system assigning it pointers to relevant component managers
            CameraFollow(std::shared_ptr<ecs::TransformationComponent> t, std::shared_ptr<ecs::CameraComponent> c)
                : transformMgr(std::move(t)), cameraMgr(std::move(c)) {}

            void transform();
            void transform(std::shared_ptr<gl::Camera> *cameras, unsigned count);
    };
}

#endif //OPEN_SEA_SYSTEMS_H
