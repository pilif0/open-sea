/** \file Systems.cpp
 * General system implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Systems.h>
#include <open-sea/ImGui.h>

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

        // Apply transformation for each entity-camera pair
        std::shared_ptr<gl::Camera> *c = cameraMgr->data.camera;
        int *i = indices.data();
        for (int j = 0; j < N; j++, i++, c++) {
            if (*i == -1) {
                (*c)->setTransformation(glm::mat4(1.0f));
            } else {
                (*c)->setTransformation(transformMgr->data.matrix[*i]);
            }
        }
    }

    /**
     * \brief Transform the cameras to their assigned entities
     *
     * \param cameras Cameras to transform
     * \param count Number of cameras
     */
    void CameraFollow::transform(std::shared_ptr<gl::Camera> *cameras, unsigned count) {
        // Gather relevant entities
        std::vector<ecs::Entity> entities(count);
        std::shared_ptr<gl::Camera> *cLoc = cameras;
        for (int j = 0; j < count; j++, cLoc++) {
            std::shared_ptr<gl::Camera> *cCom = cameraMgr->data.camera;
            for (int i = 0; i < cameraMgr->data.n; i++, cCom++) {
                if (*cLoc == *cCom)
                    entities.push_back(cameraMgr->data.entity[i]);
            }
        }

        // Look each entity up in transformation manager
        std::vector<int> indices(count);
        transformMgr->lookup(entities.data(), indices.data(), count);

        // Apply transformation for each entity-camera pair
        std::shared_ptr<gl::Camera> *c = cameraMgr->data.camera;
        int *i = indices.data();
        for (int j = 0; j < count; j++, i++, c++) {
            if (*i == -1) {
                (*c)->setTransformation(glm::mat4(1.0f));
            } else {
                (*c)->setTransformation(transformMgr->data.matrix[*i]);
            }
        }
    }

    /**
     * \brief Show ImGui debug information
     */
    void CameraFollow::showDebug() {
        if (ImGui::CollapsingHeader("Transformation Component Manager")) {
            ImGui::PushID("transformMgr");
            transformMgr->showDebug();
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Camera Component Manager")) {
            ImGui::PushID("cameraMgr");
            cameraMgr->showDebug();
            ImGui::PopID();
        }
    }
    //--- endCameraFollow implementation
}
