/** \file CameraMove.cpp
 * Camera movement implementations
 *
 * \author Filip Smola
 */

#include <open-sea/CameraMove.h>
#include <open-sea/ImGui.h>
#include <open-sea/Components.h>
#include <open-sea/GL.h>

namespace open_sea::camera {

    //--- start AtEntity implementation
    /**
     * \brief Transform all cameras with assigned entities to those entities
     */
    void AtEntity::transform() {
        // Get reference for entity's transformation
        auto ref = transform_mgr->table->get_reference(entity);

        // Transform camera in the same way, defaulting to identity if no transformation
        if (ref.matrix) {
            camera->set_transformation(*ref.matrix);
        } else {
            camera->set_transformation(glm::mat4(1.0f));
        }
    }

    /**
     * \brief Show ImGui debug information
     */
    void AtEntity::show_debug() {
        ImGui::Text("Following Entity: %s", entity.str().c_str());

        if (ImGui::CollapsingHeader("Camera")) {
            ImGui::PushID("camera");
            camera->show_debug();
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Transformation Component Manager")) {
            ImGui::PushID("transform_mgr");
            transform_mgr->show_debug();
            ImGui::PopID();
        }
    }
    //--- end AtEntity implementation
}
