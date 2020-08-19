/** \file CameraMove.h
 * Camera Movement Algorithms
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_CAMERAMOVE_H
#define OPEN_SEA_CAMERAMOVE_H

#include <open-sea/Debuggable.h>
#include <open-sea/Entity.h>

#include <memory>

// Forward declarations
namespace open_sea {
    namespace ecs {
        class TransformationTable;
    }

    namespace gl {
        class Camera;
    }
}

//TODO move cameras to the same namespace
namespace open_sea::camera {
    /**
     * \addtogroup Camera Movement Algorithms
     * \brief Algorithms that move cameras
     *
     * Algorithms used to move cameras, for example to follow an entity.
     *
     * @{
     */

    /** \class AtEntity
     * \brief Keeps camera at the position of an entity
     */
    //Note: if the assigned entity doesn't have the transformation component, identity transformation will be assumed
    class AtEntity : public debug::Debuggable {
        private:
            //! Entity to follow
            ecs::Entity entity;
            //! Camera to move
            std::shared_ptr<gl::Camera> camera;
        public:
            //! Transformation component
            std::shared_ptr<ecs::TransformationTable> transform_mgr{};

            //! Wrap a camera to follow an entity given the relevant transformation component manager
            AtEntity(std::shared_ptr<ecs::TransformationTable> t, ecs::Entity e, std::shared_ptr<gl::Camera> c)
                    : entity(e), camera(std::move(c)), transform_mgr(std::move(t)) {}

            void transform();

            void show_debug() override;
    };

    /**
     * @}
     */
}

#endif //OPEN_SEA_CAMERAMOVE_H
