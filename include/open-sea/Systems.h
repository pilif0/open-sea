/** \file Systems.h
 * Entity Component System module - General system definitions
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_SYSTEMS_H
#define OPEN_SEA_SYSTEMS_H

#include <open-sea/Components.h>
#include <open-sea/GL.h>
#include <open-sea/Debuggable.h>

namespace open_sea::ecs {
    /**
     * \addtogroup Systems
     * \brief General systems of the ECS
     *
     * General systems of the ECS.
     * These are systems that do not fit into any other module.
     *
     * @{
     */

    /** \class CameraFollow
     * \brief System that makes GL cameras follow entities they have assigned
     */
    //Note: if the assigned entity doesn't have the transformation component, identity transformation will be assumed
    class CameraFollow : public debug::Debuggable {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transform_mgr{};
            //! Camera component manager
            std::shared_ptr<ecs::CameraComponent> camera_mgr{};

            //! Construct the system assigning it pointers to relevant component managers
            CameraFollow(std::shared_ptr<ecs::TransformationComponent> t, std::shared_ptr<ecs::CameraComponent> c)
                : transform_mgr(std::move(t)), camera_mgr(std::move(c)) {}

            void transform();
            void transform(std::shared_ptr<gl::Camera> *cameras, unsigned count);

            void show_debug() override;
    };

    /**
     * @}
     */
}

#endif //OPEN_SEA_SYSTEMS_H
