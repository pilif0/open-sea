/** \file Controls.cpp
 * Control component and system implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Controls.h>
#include <open-sea/Components.h>
#include <open-sea/Delta.h>
#include <open-sea/Window.h>
#include <open-sea/Debug.h>

#include <imgui.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace open_sea::controls {
    //! Local pitch axis - positive x axis
    glm::vec3 pitch_axis{1.0f, 0.0f, 0.0f};
    //! Local yaw axis - ositive y axis
    glm::vec3 yaw_axis{0.0f, 1.0f, 0.0f};
    //! Local roll axis - ositive z axis
    glm::vec3 roll_axis{0.0f, 0.0f, 1.0f};

    //--- start Controls implementation
    /**
     * \brief Set a new subject for this control
     *
     * \param new_subject New subject
     */
    void Controls::set_subject(ecs::Entity new_subject) {
        subject = new_subject;
    }

    /**
     * \brief Get current subject of this control
     *
     * \return Current subject
     */
    ecs::Entity Controls::get_subject() const {
        return subject;
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void Controls::show_debug() {
        ImGui::Text("Subject: %s", subject.str().c_str());
        ImGui::Text("Last translate: %.3f, %.3f, %.3f", last_translate.x, last_translate.y, last_translate.z);
        ImGui::TextUnformatted("Last rotate:");
        ImGui::SameLine();
        debug::show_quat(last_rotate);
    }
    //--- end Controls implementation
    //--- start Free implementation

    void Free::transform() {
        // Update position
        {
            // Gather input
            glm::vec3 local{};
            if (input::is_held(config.left)) {
                local.x += -1;
            }
            if (input::is_held(config.right)) {
                local.x += 1;
            }
            if (input::is_held(config.forward)) {
                local.z += -1;
            }
            if (input::is_held(config.backward)) {
                local.z += 1;
            }
            if (input::is_held(config.up)) {
                local.y += 1;
            }
            if (input::is_held(config.down)) {
                local.y += -1;
            }

            // Only translate if needed
            float l = glm::length(local);
            if (l != 0) {
                int index = transform_mgr->lookup(subject);

                // Normalise
                if (l != 0.0f && l != 1.0f) {
                    local /= l;
                }

                // Transform and apply
                glm::vec3 global_translate = glm::rotate(transform_mgr->data.orientation[index], local);
                global_translate.x *= config.speed_x;
                global_translate.y *= config.speed_y;
                global_translate.z *= config.speed_z;
                global_translate *= time::get_delta();
                transform_mgr->translate(&index, &global_translate, 1);
                last_translate = global_translate;
            } else {
                last_translate = {};
            }
        }

        // Update rotation
        {
            // Ensure cursor is disabled
            if (input::get_cursor_mode() != input::cursor_mode::disabled) {
                input::set_cursor_mode(input::cursor_mode::disabled);
            }

            // Look up subject transformation
            int index = transform_mgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- config.turn_rate);

            // Positive yaw is left
            float yaw = cursor_delta.x * (- config.turn_rate);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(config.clockwise)) {
                roll += config.roll_rate;
            }
            if (input::is_held(config.counter_clockwise)) {
                roll -= config.roll_rate;
            }
            roll *= time::get_delta();

            // Compute transformation quaternion and transform
            if (pitch != 0 || yaw != 0 || roll != 0) {
                // Transform axes of rotation
                glm::quat original = transform_mgr->data.orientation[index];
                glm::vec3 tr_pitch_ax = glm::rotate(original,  pitch_axis);
                glm::vec3 tr_yaw_ax = glm::rotate(original, yaw_axis);
                glm::vec3 tr_roll_ax = glm::rotate(original, roll_axis);

                // Compute transformation
                glm::quat pitch_q = glm::angleAxis(glm::radians(pitch), tr_pitch_ax);
                glm::quat yaw_q = glm::angleAxis(glm::radians(yaw), tr_yaw_ax);
                glm::quat roll_q = glm::angleAxis(glm::radians(roll), tr_roll_ax);
                glm::quat rotation = roll_q * yaw_q * pitch_q;

                // Apply
                transform_mgr->rotate(&index, &rotation, 1);
                last_rotate = rotation;
            } else {
                last_rotate = glm::quat();
            }
        }
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void Free::show_debug() {
        Controls::show_debug();

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

    void FPS::transform() {
        // Update position
        {
            // Gather input
            glm::vec3 local{};
            if (input::is_held(config.left)) {
                local.x += -1;
            }
            if (input::is_held(config.right)) {
                local.x += 1;
            }
            if (input::is_held(config.forward)) {
                local.z += -1;
            }
            if (input::is_held(config.backward)) {
                local.z += 1;
            }

            // Only translate if actually needed
            if (glm::length(local) != 0) {
                int index = transform_mgr->lookup(subject);

                // Transform
                glm::vec3 global_translate = glm::rotate(transform_mgr->data.orientation[index], local);

                // Lock to XZ plane
                global_translate.y = 0;

                // Only translate if needed
                float l = glm::length(global_translate);
                if (l != 0) {
                    // Normalise
                    if (l != 1.0f) {
                        global_translate /= l;
                    }

                    // Scale by speed and delta time
                    global_translate.x *= config.speed_x;
                    global_translate.z *= config.speed_z;
                    global_translate *= time::get_delta();

                    // Apply
                    transform_mgr->translate(&index, &global_translate, 1);
                    last_translate = global_translate;
                }
            } else {
                last_translate = {};
            }
        }

        // Update rotation
        {
            // Ensure cursor is disabled
            if (input::get_cursor_mode() != input::cursor_mode::disabled) {
                input::set_cursor_mode(input::cursor_mode::disabled);
            }

            // Look up subject transformation
            int index = transform_mgr->lookup(subject);

            // Retrieve delta
            glm::vec2 cursor_delta = input::cursor_delta();

            // Positive pitch is up
            float pitch = cursor_delta.y * (- config.turn_rate);
            float new_pitch = pitch + this->pitch;

            // Positive yaw is left
            float yaw = cursor_delta.x * (/*-*/ config.turn_rate);

            // Compute transformation quaternion and transform
            if (pitch != 0 || yaw != 0) {
                // Clamp pitch
                if (new_pitch > 90) {
                    pitch -= new_pitch - 90;
                } else if (new_pitch < -90) {
                    pitch -= new_pitch - (-90);
                }

                // Transform axes of rotation
                glm::quat original = transform_mgr->data.orientation[index];
                glm::vec3 tr_pitch_ax = glm::rotate(original,  pitch_axis);

                // Compute transformation
                glm::quat pitch_q = glm::angleAxis(glm::radians(pitch), tr_pitch_ax);
                glm::quat yaw_q = glm::angleAxis(glm::radians(yaw), yaw_axis);
                glm::quat rotation = yaw_q * pitch_q;

                // Apply
                transform_mgr->rotate(&index, &rotation, 1);

                // Update stored pitch
                this->pitch += pitch;

                last_rotate = rotation;
            } else {
                last_rotate = glm::quat();
            }
        }
    }

    /**
     * \brief Set a new subject for this control
     *
     * \param newSubject New subject
     */
    void FPS::set_subject(ecs::Entity newSubject) {
        Controls::set_subject(newSubject);
        update_pitch();
    }

    /**
     * \brief Update pitch tracker with value from subject transformation
     */
    void FPS::update_pitch() {
        // Get the subject's forward vector
        int index = transform_mgr->lookup(subject);
        glm::quat orientation = transform_mgr->data.orientation[index];
        glm::vec3 forward = glm::rotate(orientation, glm::vec3{0.0f, 0.0f, -1.0f});

        // Project forward onto XZ plane
        glm::vec3 forward_xz = forward;
        forward_xz.y = 0;

        // Pitch is the angle from the projection to forward
        float l = glm::length(forward);
        if (l != 0.0f && l != 1.0f) {
            forward /= l;
        }
        l = glm::length(forward_xz);
        if (l != 0.0f && l != 1.0f) {
            forward_xz /= l;
        }
        pitch = glm::orientedAngle(forward_xz, forward, glm::vec3{1.0f, 0.0f, 0.0f});
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void FPS::show_debug() {
        Controls::show_debug();
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
            if (input::is_held(config.left)) {
                local.x += -1;
            }
            if (input::is_held(config.right)) {
                local.x += 1;
            }
            if (input::is_held(config.up)) {
                local.y += 1;
            }
            if (input::is_held(config.down)) {
                local.y += -1;
            }

            // Translate only if needed
            if (glm::length(local) != 0) {
                int index = transform_mgr->lookup(subject);

                // Transform
                glm::vec3 global_translate = glm::rotate(transform_mgr->data.orientation[index], local);

                // Lock to XY plane
                global_translate.z = 0;

                // Translate only if needed
                float l = glm::length(global_translate);
                if (l != 0) {
                    // Normalise
                    if (l != 1.0f) {
                        global_translate /= l;
                    }

                    // Scale by speed and delta time
                    global_translate.x *= config.speed_x;
                    global_translate.y *= config.speed_y;
                    global_translate *= time::get_delta();

                    // Apply
                    transform_mgr->translate(&index, &global_translate, 1);
                    last_translate = global_translate;
                } else {
                    last_translate = {};
                }
            } else {
                last_translate = {};
            }
        }

        // Update rotation
        glm::quat rotation{};
        {
            // Look up subject transformation
            int index = transform_mgr->lookup(subject);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::is_held(config.clockwise)) {
                roll += config.roll_rate;
            }
            if (input::is_held(config.counter_clockwise)) {
                roll -= config.roll_rate;
            }
            roll *= time::get_delta();

            // Compute transformation quaternion and transform
            if (roll != 0) {
                // Transform axes of rotation
                glm::quat original = transform_mgr->data.orientation[index];
                glm::vec3 tr_roll_ax = glm::rotate(original, roll_axis);

                // Compute transformation
                glm::quat roll_q = glm::angleAxis(glm::radians(roll), tr_roll_ax);

                // Apply
                transform_mgr->rotate(&index, &roll_q, 1);
                last_rotate = rotation;
            } else {
                last_rotate = glm::quat();
            }
        }
    }

    /**
     * \brief Show ImGui debug information for this control
     */
    void TopDown::show_debug() {
        Controls::show_debug();

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
