/** \file Systems.cpp
 * General system implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Systems.h>
#include <open-sea/ImGui.h>
#include <open-sea/Components.h>
#include <open-sea/GL.h>

#include <vector>

namespace open_sea::ecs {

    //--- start CameraFollow implementation
    /**
     * \brief Transform all cameras with assigned entities to those entities
     */
    void CameraFollow::transform() {
        unsigned n = camera_mgr->data.n;

        // Get reference for each entity in transformation manager
        std::vector<TransformationTable::Data::Ptr> refs(n);
        transform_mgr->table->get_reference(camera_mgr->data.entity, refs.data(), n);

        // Apply transformation for each entity-camera pair
        std::shared_ptr<gl::Camera> *c = camera_mgr->data.camera;
        auto r = refs.begin();
        for (unsigned j = 0; j < n; j++, r++, c++) {
            if (r->matrix == nullptr) {
                (*c)->set_transformation(glm::mat4(1.0f));
            } else {
                (*c)->set_transformation(*r->matrix);
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
        std::shared_ptr<gl::Camera> *c_loc = cameras;
        for (unsigned j = 0; j < count; j++, c_loc++) {
            std::shared_ptr<gl::Camera> *c_com = camera_mgr->data.camera;
            for (unsigned i = 0; i < camera_mgr->data.n; i++, c_com++) {
                if (*c_loc == *c_com) {
                    entities.push_back(camera_mgr->data.entity[i]);
                }
            }
        }

        // Look each entity up in transformation manager
        std::vector<TransformationTable::Data::Ptr> refs(count);
        transform_mgr->table->get_reference(camera_mgr->data.entity, refs.data(), count);

        // Apply transformation for each entity-camera pair
        std::shared_ptr<gl::Camera> *c = camera_mgr->data.camera;
        auto r = refs.begin();
        for (unsigned j = 0; j < count; j++, r++, c++) {
            if (r->matrix == nullptr) {
                (*c)->set_transformation(glm::mat4(1.0f));
            } else {
                (*c)->set_transformation(*r->matrix);
            }
        }
    }

    /**
     * \brief Show ImGui debug information
     */
    void CameraFollow::show_debug() {
        if (ImGui::CollapsingHeader("Transformation Component Manager")) {
            ImGui::PushID("transform_mgr");
            transform_mgr->show_debug();
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Camera Component Manager")) {
            ImGui::PushID("camera_mgr");
            camera_mgr->show_debug();
            ImGui::PopID();
        }
    }
    //--- endCameraFollow implementation
}
