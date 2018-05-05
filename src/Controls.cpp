/*
 * Control component and system implementations
 */

#include <open-sea/Controls.h>
#include <open-sea/Delta.h>
#include <open-sea/Window.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace open_sea::controls {

    //--- start FreeControls implementation
    /**
     * \brief Transform the subject according to input
     */
    void FreeControls::transform() {
        // Update position
        glm::vec3 global_translate{};
        {
            int index = transformMgr->lookup(subject);
            glm::vec3 local{};

            // Gather input
            if (input::is_held(controls.left))
                local.x += -1;
            if (input::is_held(controls.right))
                local.x += 1;
            if (input::is_held(controls.forward))
                local.z += -1;
            if (input::is_held(controls.backward))
                local.z += 1;
            if (input::is_held(controls.up))
                local.y += 1;
            if (input::is_held(controls.down))
                local.y += -1;

            // Normalise
            float l = glm::length(local);
            if (l != 0.0f && l != 1.0f)
                local /= l;

            // Transform and apply
            global_translate = glm::rotate(transformMgr->data.orientation[index], local);
            global_translate.x *= controls.speed_x;
            global_translate.y *= controls.speed_y;
            global_translate.z *= controls.speed_z;
            global_translate *= time::get_delta();
            transformMgr->translate(&index, &global_translate, 1);
        }

        // Update camera guide rotation
        glm::quat rotation{};
        if (input::is_held(controls.turn)) {
            // Local axes of rotation
            constexpr glm::vec3 pitch_axis{1.0f, 0.0f, 0.0f};   // Positive x axis
            constexpr glm::vec3 yaw_axis{0.0f, 1.0f, 0.0f};     // Positive y axis
            constexpr glm::vec3 roll_axis{0.0f, 0.0f, 1.0f};    // Positive z axis

            // Ensure cursor is disabled
            if (glfwGetInputMode(window::window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
                glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- controls.turn_rate);

            // Positive yaw is left
            float yaw = cursor_delta.x * (- controls.turn_rate);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(controls.clockwise))
                roll += controls.roll_rate;
            if (input::is_held(controls.counter_clockwise))
                roll -= controls.roll_rate;
            roll *= time::get_delta();

            // Compute transformation quaternion and transform
            if (pitch != 0 || yaw != 0 || roll != 0) {
                // Transform axes of rotation
                glm::quat original = transformMgr->data.orientation[index];
                glm::vec3 tr_pitch_ax = glm::rotate(original,  pitch_axis);
                glm::vec3 tr_yaw_ax = glm::rotate(original, yaw_axis);
                glm::vec3 tr_roll_ax = glm::rotate(original, roll_axis);

                // Compute transformation
                glm::quat pitchQ = glm::angleAxis(glm::radians(pitch), tr_pitch_ax);
                glm::quat yawQ = glm::angleAxis(glm::radians(yaw), tr_yaw_ax);
                glm::quat rollQ = glm::angleAxis(glm::radians(roll), tr_roll_ax);
                rotation = rollQ * yawQ * pitchQ;

                // Apply
                transformMgr->rotate(&index, &rotation, 1);
            }
        } else {
            // Ensure cursor is not disabled
            if (glfwGetInputMode(window::window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
                glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void FreeControls::setSubject(ecs::Entity newSubject) {
        subject = newSubject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void FreeControls::showDebug() {
        //TODO
    }
    //--- end FreeControls implementation

}
