/*
 * Control component and system implementations
 */

#include <open-sea/Controls.h>
#include <open-sea/Delta.h>
#include <open-sea/Window.h>

#include <imgui.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace open_sea::controls {
    // Local axes of rotation
    constexpr glm::vec3 pitch_axis{1.0f, 0.0f, 0.0f};   // Positive x axis
    constexpr glm::vec3 yaw_axis{0.0f, 1.0f, 0.0f};     // Positive y axis
    constexpr glm::vec3 roll_axis{0.0f, 0.0f, 1.0f};    // Positive z axis

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

        // Update rotation
        glm::quat rotation{};
        if (input::is_held(controls.turn)) {
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
     * \brief Get current subject of this control
     *
     * \return Current subject
     */
    ecs::Entity FreeControls::getSubject() {
        return subject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void FreeControls::showDebug() {
        ImGui::Text("Subject: %s", subject.str().c_str());

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Forward: %s", controls.forward.str().c_str());
            ImGui::Text("Backward: %s", controls.backward.str().c_str());
            ImGui::Text("Left: %s", controls.left.str().c_str());
            ImGui::Text("Right: %s", controls.right.str().c_str());
            ImGui::Text("Up: %s", controls.up.str().c_str());
            ImGui::Text("Down: %s", controls.down.str().c_str());
            ImGui::Text("Clockwise roll: %s", controls.clockwise.str().c_str());
            ImGui::Text("Counter-clockwise roll: %s", controls.counter_clockwise.str().c_str());
            ImGui::Text("Turn: %s", controls.turn.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", controls.speed_x);
            ImGui::Text("Speed Y: %.3f units / second", controls.speed_y);
            ImGui::Text("Speed Z: %.3f units / second", controls.speed_z);
            ImGui::Text("Turn rate: %.3f degrees / screen unit", controls.turn_rate);
            ImGui::Text("Roll rate: %.3f degrees / second", controls.roll_rate);
        }
    }
    //--- end FreeControls implementation
    //--- start FPSControls implementation
    /**
     * \brief Transform the subject according to input
     */
    void FPSControls::transform() {
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

            // Transform
            global_translate = glm::rotate(transformMgr->data.orientation[index], local);

            // Lock to XZ plane
            global_translate.y = 0;

            // Normalise
            float l = glm::length(global_translate);
            if (l != 0.0f && l != 1.0f)
                global_translate /= l;

            // Scale by speed and delta time
            global_translate.x *= controls.speed_x;
            global_translate.z *= controls.speed_z;
            global_translate *= time::get_delta();

            // Apply
            transformMgr->translate(&index, &global_translate, 1);
        }

        // Update rotation
        glm::quat rotation{};
        {
            // Ensure cursor is disabled
            if (glfwGetInputMode(window::window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
                glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- controls.turn_rate);
            float newPitch = pitch + this->pitch;

            // Positive yaw is left
            float yaw = cursor_delta.x * (/*-*/ controls.turn_rate);

            // Compute transformation quaternion and transform
            if (pitch != 0 || yaw != 0) {
                // Clamp pitch
                if (newPitch > 90)
                    pitch -= newPitch - 90;
                else if (newPitch < -90)
                    pitch -= newPitch - (-90);

                // Transform axes of rotation
                glm::quat original = transformMgr->data.orientation[index];
                glm::vec3 tr_pitch_ax = glm::rotate(original,  pitch_axis);

                // Compute transformation
                glm::quat pitchQ = glm::angleAxis(glm::radians(pitch), tr_pitch_ax);
                glm::quat yawQ = glm::angleAxis(glm::radians(yaw), yaw_axis);
                rotation = yawQ * pitchQ;

                // Apply
                transformMgr->rotate(&index, &rotation, 1);

                // Update stored pitch
                this->pitch += pitch;
            }
        }
    }

    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void FPSControls::setSubject(ecs::Entity newSubject) {
        subject = newSubject;
        updatePitch();
    }

    /**
     * \brief Update pitch tracker with value from subject transformation
     */
    void FPSControls::updatePitch() {
        // Get the subject's forward vector
        int index = transformMgr->lookup(subject);
        glm::quat orientation = transformMgr->data.orientation[index];
        glm::vec3 forward = glm::rotate(orientation, glm::vec3{0.0f, 0.0f, -1.0f});

        // Project forward onto XZ plane
        glm::vec3 forwardXZ = forward;
        forwardXZ.y = 0;

        // Pitch is the angle from the projection to forward
        float l = glm::length(forward);
        if (l != 0.0f && l != 1.0f)
            forward /= l;
        l = glm::length(forwardXZ);
        if (l != 0.0f && l != 1.0f)
            forwardXZ /= l;
        pitch = glm::orientedAngle(forwardXZ, forward, glm::vec3{1.0f, 0.0f, 0.0f});
    }

    /**
     * \brief Get current subject of this control
     *
     * \return Current subject
     */
    ecs::Entity FPSControls::getSubject() {
        return subject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void FPSControls::showDebug() {
        ImGui::Text("Subject: %s", subject.str().c_str());
        ImGui::Text("Pitch: %.3f degrees", pitch);

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Forward: %s", controls.forward.str().c_str());
            ImGui::Text("Backward: %s", controls.backward.str().c_str());
            ImGui::Text("Left: %s", controls.left.str().c_str());
            ImGui::Text("Right: %s", controls.right.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", controls.speed_x);
            ImGui::Text("Speed Z: %.3f units / second", controls.speed_z);
            ImGui::Text("Turn rate: %.3f degrees / screen unit", controls.turn_rate);
        }
    }
    //--- end FreeControls implementation
    //--- start TopDownControls implementation
    /**
     * \brief Transform the subject according to input
     */
    void TopDownControls::transform() {
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
            if (input::is_held(controls.up))
                local.y += 1;
            if (input::is_held(controls.down))
                local.y += -1;

            // Transform
            global_translate = glm::rotate(transformMgr->data.orientation[index], local);

            // Lock to XY plane
            global_translate.z = 0;

            // Normalise
            float l = glm::length(global_translate);
            if (l != 0.0f && l != 1.0f)
                global_translate /= l;

            // Scale by speed and delta time
            global_translate.x *= controls.speed_x;
            global_translate.y *= controls.speed_y;
            global_translate *= time::get_delta();

            // Apply
            transformMgr->translate(&index, &global_translate, 1);
        }

        // Update rotation
        glm::quat rotation{};
        {
            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(controls.clockwise))
                roll += controls.roll_rate;
            if (input::is_held(controls.counter_clockwise))
                roll -= controls.roll_rate;
            roll *= time::get_delta();

            // Compute transformation quaternion and transform
            if (roll != 0) {
                // Transform axes of rotation
                glm::quat original = transformMgr->data.orientation[index];
                glm::vec3 tr_roll_ax = glm::rotate(original, roll_axis);

                // Compute transformation
                glm::quat rollQ = glm::angleAxis(glm::radians(roll), tr_roll_ax);

                // Apply
                transformMgr->rotate(&index, &rollQ, 1);
            }
        }
    }

    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void TopDownControls::setSubject(ecs::Entity newSubject) {
        subject = newSubject;
    }

    /**
     * \brief Get current subject of this control
     *
     * \return Current subject
     */
    ecs::Entity TopDownControls::getSubject() {
        return subject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void TopDownControls::showDebug() {
        ImGui::Text("Subject: %s", subject.str().c_str());

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Left: %s", controls.left.str().c_str());
            ImGui::Text("Right: %s", controls.right.str().c_str());
            ImGui::Text("Up: %s", controls.up.str().c_str());
            ImGui::Text("Down: %s", controls.down.str().c_str());
            ImGui::Text("Clockwise roll: %s", controls.clockwise.str().c_str());
            ImGui::Text("Counter-clockwise roll: %s", controls.counter_clockwise.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", controls.speed_x);
            ImGui::Text("Speed Y: %.3f units / second", controls.speed_y);
            ImGui::Text("Roll rate: %.3f degrees / second", controls.roll_rate);
        }
    }
    //--- end TopDownControls implementation
}
