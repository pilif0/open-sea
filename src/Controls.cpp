/*
 * Control component and system implementations
 */

#include <open-sea/Controls.h>
#include <open-sea/Delta.h>
#include <open-sea/Window.h>
#include <open-sea/Debug.h>

#include <imgui.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace open_sea::controls {
    // Local axes of rotation
    glm::vec3 pitch_axis{1.0f, 0.0f, 0.0f};   // Positive x axis
    glm::vec3 yaw_axis{0.0f, 1.0f, 0.0f};     // Positive y axis
    glm::vec3 roll_axis{0.0f, 0.0f, 1.0f};    // Positive z axis

    //--- start Controls implementation
    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void Controls::setSubject(ecs::Entity newSubject) {
        subject = newSubject;
    }

    /**
     * \brief Get current subject of this control
     *
     * \return Current subject
     */
    ecs::Entity Controls::getSubject() const {
        return subject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void Controls::showDebug() {
        ImGui::Text("Subject: %s", subject.str().c_str());
        ImGui::Text("Last translate: %.3f, %.3f, %.3f", lastTranslate.x, lastTranslate.y, lastTranslate.z);
        ImGui::TextUnformatted("Last rotate:");
        ImGui::SameLine();
        debug::show_quat(lastRotate);
    }
    //--- end Controls implementation
    //--- start Free implementation
    /**
     * \brief Transform the subject according to input
     */
    void Free::transform() {
        // Update position
        {
            // Gather input
            glm::vec3 local{};
            if (input::is_held(config.left))
                local.x += -1;
            if (input::is_held(config.right))
                local.x += 1;
            if (input::is_held(config.forward))
                local.z += -1;
            if (input::is_held(config.backward))
                local.z += 1;
            if (input::is_held(config.up))
                local.y += 1;
            if (input::is_held(config.down))
                local.y += -1;

            // Only translate if needed
            float l = glm::length(local);
            if (l != 0) {
                int index = transformMgr->lookup(subject);

                // Normalise
                if (l != 0.0f && l != 1.0f)
                    local /= l;

                // Transform and apply
                glm::vec3 global_translate = glm::rotate(transformMgr->data.orientation[index], local);
                global_translate.x *= config.speed_x;
                global_translate.y *= config.speed_y;
                global_translate.z *= config.speed_z;
                global_translate *= time::get_delta();
                transformMgr->translate(&index, &global_translate, 1);
                lastTranslate = global_translate;
            } else {
                lastTranslate = {};
            }
        }

        // Update rotation
        {
            // Ensure cursor is disabled
            if (input::get_cursor_mode() != input::cursor_mode::disabled)
                input::set_cursor_mode(input::cursor_mode::disabled);

            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- config.turn_rate);

            // Positive yaw is left
            float yaw = cursor_delta.x * (- config.turn_rate);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(config.clockwise))
                roll += config.roll_rate;
            if (input::is_held(config.counter_clockwise))
                roll -= config.roll_rate;
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
                glm::quat rotation = rollQ * yawQ * pitchQ;

                // Apply
                transformMgr->rotate(&index, &rotation, 1);
                lastRotate = rotation;
            } else {
                lastRotate = glm::quat();
            }
        }
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void Free::showDebug() {
        Controls::showDebug();

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Forward: %s", config.forward.str().c_str());
            ImGui::Text("Backward: %s", config.backward.str().c_str());
            ImGui::Text("Left: %s", config.left.str().c_str());
            ImGui::Text("Right: %s", config.right.str().c_str());
            ImGui::Text("Up: %s", config.up.str().c_str());
            ImGui::Text("Down: %s", config.down.str().c_str());
            ImGui::Text("Clockwise roll: %s", config.clockwise.str().c_str());
            ImGui::Text("Counter-clockwise roll: %s", config.counter_clockwise.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", config.speed_x);
            ImGui::Text("Speed Y: %.3f units / second", config.speed_y);
            ImGui::Text("Speed Z: %.3f units / second", config.speed_z);
            ImGui::Text("Turn rate: %.3f degrees / screen unit", config.turn_rate);
            ImGui::Text("Roll rate: %.3f degrees / second", config.roll_rate);
        }
    }
    //--- end Free implementation
    //--- start FPS implementation
    /**
     * \brief Transform the subject according to input
     */
    void FPS::transform() {
        // Update position
        {
            // Gather input
            glm::vec3 local{};
            if (input::is_held(config.left))
                local.x += -1;
            if (input::is_held(config.right))
                local.x += 1;
            if (input::is_held(config.forward))
                local.z += -1;
            if (input::is_held(config.backward))
                local.z += 1;

            // Only translate if actually needed
            if (glm::length(local) != 0) {
                int index = transformMgr->lookup(subject);

                // Transform
                glm::vec3 global_translate = glm::rotate(transformMgr->data.orientation[index], local);

                // Lock to XZ plane
                global_translate.y = 0;

                // Only translate if needed
                float l = glm::length(global_translate);
                if (l != 0) {
                    // Normalise
                    if (l != 1.0f)
                        global_translate /= l;

                    // Scale by speed and delta time
                    global_translate.x *= config.speed_x;
                    global_translate.z *= config.speed_z;
                    global_translate *= time::get_delta();

                    // Apply
                    transformMgr->translate(&index, &global_translate, 1);
                    lastTranslate = global_translate;
                }
            } else {
                lastTranslate = {};
            }
        }

        // Update rotation
        {
            // Ensure cursor is disabled
            if (input::get_cursor_mode() != input::cursor_mode::disabled)
                input::set_cursor_mode(input::cursor_mode::disabled);

            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- config.turn_rate);
            float newPitch = pitch + this->pitch;

            // Positive yaw is left
            float yaw = cursor_delta.x * (/*-*/ config.turn_rate);

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
                glm::quat rotation = yawQ * pitchQ;

                // Apply
                transformMgr->rotate(&index, &rotation, 1);

                // Update stored pitch
                this->pitch += pitch;

                lastRotate = rotation;
            } else {
                lastRotate = glm::quat();
            }
        }
    }

    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void FPS::setSubject(ecs::Entity newSubject) {
        Controls::setSubject(newSubject);
        updatePitch();
    }

    /**
     * \brief Update pitch tracker with value from subject transformation
     */
    void FPS::updatePitch() {
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
     * \brief Show ImGui debug information for this control
     */
    void FPS::showDebug() {
        Controls::showDebug();
        ImGui::Text("Pitch: %.3f degrees", pitch);

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Forward: %s", config.forward.str().c_str());
            ImGui::Text("Backward: %s", config.backward.str().c_str());
            ImGui::Text("Left: %s", config.left.str().c_str());
            ImGui::Text("Right: %s", config.right.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", config.speed_x);
            ImGui::Text("Speed Z: %.3f units / second", config.speed_z);
            ImGui::Text("Turn rate: %.3f degrees / screen unit", config.turn_rate);
        }
    }
    //--- end FPS implementation
    //--- start TopDown implementation
    /**
     * \brief Transform the subject according to input
     */
    void TopDown::transform() {
        // Update position
        {
            // Gather input
            glm::vec3 local{};
            if (input::is_held(config.left))
                local.x += -1;
            if (input::is_held(config.right))
                local.x += 1;
            if (input::is_held(config.up))
                local.y += 1;
            if (input::is_held(config.down))
                local.y += -1;

            // Translate only if needed
            if (glm::length(local) != 0) {
                int index = transformMgr->lookup(subject);

                // Transform
                glm::vec3 global_translate = glm::rotate(transformMgr->data.orientation[index], local);

                // Lock to XY plane
                global_translate.z = 0;

                // Translate only if needed
                float l = glm::length(global_translate);
                if (l != 0) {
                    // Normalise
                    if (l != 1.0f)
                        global_translate /= l;

                    // Scale by speed and delta time
                    global_translate.x *= config.speed_x;
                    global_translate.y *= config.speed_y;
                    global_translate *= time::get_delta();

                    // Apply
                    transformMgr->translate(&index, &global_translate, 1);
                    lastTranslate = global_translate;
                } else {
                    lastTranslate = {};
                }
            } else {
                lastTranslate = {};
            }
        }

        // Update rotation
        glm::quat rotation{};
        {
            // Look up subject transformation
            int index = transformMgr->lookup(subject);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(config.clockwise))
                roll += config.roll_rate;
            if (input::is_held(config.counter_clockwise))
                roll -= config.roll_rate;
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
                lastRotate = rotation;
            } else {
                lastRotate = glm::quat();
            }
        }
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void TopDown::showDebug() {
        Controls::showDebug();

        if (ImGui::CollapsingHeader("Bindings")) {
            ImGui::Text("Left: %s", config.left.str().c_str());
            ImGui::Text("Right: %s", config.right.str().c_str());
            ImGui::Text("Up: %s", config.up.str().c_str());
            ImGui::Text("Down: %s", config.down.str().c_str());
            ImGui::Text("Clockwise roll: %s", config.clockwise.str().c_str());
            ImGui::Text("Counter-clockwise roll: %s", config.counter_clockwise.str().c_str());
        }

        if (ImGui::CollapsingHeader("Factors")) {
            ImGui::Text("Speed X: %.3f units / second", config.speed_x);
            ImGui::Text("Speed Y: %.3f units / second", config.speed_y);
            ImGui::Text("Roll rate: %.3f degrees / second", config.roll_rate);
        }
    }
    //--- end TopDown implementation
}
