/*
 * System implementations
 */

#include <open-sea/Systems.h>

#include <vector>

namespace open_sea::ecs {

    //--- start CameraFollow implementation
    /**
     * \brief Transform all cameras with assigned entities to those entities
     */
    void CameraFollow::transform() {
        unsigned N = cameraMgr->data.n;

        // Look each entity up in transformation manager
        std::vector<int> indices(N);
        transformMgr->lookup(cameraMgr->data.entity, indices.data(), N);

        // Construct transformation data for each entity
        std::vector<EntityData> transformData(N);
        int *i = indices.data();
        EntityData *d = transformData.data();
        for (int j = 0; j < N; j++, i++, d++) {
            if (*i == -1) {
                *d = EntityData{
                        .position = glm::vec3{},
                        .orientation = glm::quat()
                };
            } else {
                *d = EntityData{
                        .position = transformMgr->data.position[*i],
                        .orientation = transformMgr->data.orientation[*i]
                };
            }
        }

        // Apply the transformations
        std::shared_ptr<gl::Camera> *c = cameraMgr->data.camera;
        d = transformData.data();
        for (int j = 0; j < N; j++, c++, d++) {
            //Note: assumes the entity is root
            (*c)->setPosition(d->position);
            (*c)->setRotation(d->orientation);
        }
    }

    /**
     * \brief Transform the cameras to their assigned entities
     *
     * \param cameras Cameras to transform
     * \param count Number of cameras
     */
    void CameraFollow::transform(std::shared_ptr<gl::Camera> *cameras, unsigned count) {

    }
    //--- endCameraFollow implementation
}
