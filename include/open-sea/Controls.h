/** \file Controls.h
 * Components and systems related to entity controls
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_CONTROLS_H
#define OPEN_SEA_CONTROLS_H

#include <open-sea/Entity.h>
#include <open-sea/Log.h>
#include <open-sea/Input.h>
#include <open-sea/Debuggable.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Forward declarations
namespace open_sea::ecs {
    class TransformationComponent;
}

//! %Controls that allow for transforming of an entity based on user input
namespace open_sea::controls {
    /**
     * \addtogroup Controls
     * \brief Entity transformation controls
     *
     * Controls that allow for transforming (translating and rotating) of an entity based on user input.
     * Each control scheme has a single entity as the subject.
     * More entities can be moved by taking advantage of the parent-child relationship in the transformation component.
     *
     * @{
     */

    /** \class Controls
     * \brief Abstract base class for all controls
     */
    class Controls : public debug::Debuggable {
        protected:
            //! Entity being controlled
            ecs::Entity subject;
            //! Last applied translation
            glm::vec3 last_translate{};
            //! Last applied rotation
            glm::quat last_rotate;

        public:
            explicit Controls(ecs::Entity s) : subject(s), last_rotate(glm::quat()) {}
            //! Transform the subject according to input
            virtual void transform() = 0;
            virtual void set_subject(ecs::Entity new_subject);
            virtual ecs::Entity get_subject() const;

            void show_debug() override;

            virtual ~Controls() {}
    };

    /** \class Free
     * \brief %Controls an entity using free controls
     *
     * %Controls an entity using free controls.
     * No restrictions on orientation or position control.
     * Unified input control over X, Y, Z position and roll.
     * Mouse control over pitch and yaw.
     * All control is from the perspective of the entity.
     * Only one entity should be controlled at one time.
     */
    class Free : public Controls {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transform_mgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Negative Z
                input::UnifiedInput forward;
                //! Positive Z
                input::UnifiedInput backward;
                //! Negative X
                input::UnifiedInput left;
                //! Positive X
                input::UnifiedInput right;
                //! Positive Y
                input::UnifiedInput up;
                //! Negative Y
                input::UnifiedInput down;
                //! Negatvie around positive Z
                input::UnifiedInput clockwise;
                //! Positive around positive Z
                input::UnifiedInput counter_clockwise;

                // Factors
                //! Left-right (strafing) speed (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_z;
                //! Up-down speed (units / second)
                float speed_y;
                //! Turn rate (degree / screen unit)
                float turn_rate;
                //! Roll rate (degree / second)
                float roll_rate;
            } config;

            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and config
            Free(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Config c)
                    : Controls(s), transform_mgr(std::move(t)), config(c) {}

            void transform() override;

            void show_debug() override;
    };

    /** \class FPS
     * \brief %Controls an entity using %FPS controls
     *
     * %Controls an entity using FPS controls.
     * Position control in XZ plane (from the perspective of the entity) without restriction through unified input.
     * Rotation control of yaw around parent's Y axis and of pitch within [+90,-90] degrees through mouse input.
     * No control over roll and movement along the Y axis.
     * Orientation control always on (disables cursor).
     * Only one entity should be controlled at one time.
     */
    // Note: this still allows Y motion through external means, just not through this specific control
    class FPS : public Controls {
        private:
            //! Current subject's pitch (in degrees)
            float pitch = 0.0f;
            void update_pitch();

        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transform_mgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Negative Z
                input::UnifiedInput forward;
                //! Positive Z
                input::UnifiedInput backward;
                //! Negative X
                input::UnifiedInput left;
                //! Positive X
                input::UnifiedInput right;

                // Factors
                //! Left-right (strafing) speed (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_z;
                //! Turn rate (degree / screen unit)
                float turn_rate;
            } config;


            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and config
            FPS(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Config c)
                    : Controls(s), transform_mgr(std::move(t)), config(c) { update_pitch(); }

            void transform() override;
            void set_subject(ecs::Entity newSubject) override;

            void show_debug() override;
    };

    /** \class TopDown
     * \brief %Controls an entity using top down controls
     *
     * %Controls an entity using top down controls.
     * Position control in XY plane (from the perspective of the entity) without restriction through unified input.
     * Rotation control of roll around entity's Z axis through unified input.
     * No control over yaw, pitch and movement along the Z axis.
     * Only one entity should be controlled at one time.
     */
    class TopDown : public Controls {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transform_mgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Positive Y
                input::UnifiedInput up;
                //! Negative Y
                input::UnifiedInput down;
                //! Negative X
                input::UnifiedInput left;
                //! Positive X
                input::UnifiedInput right;
                //! Negative around positive Z
                input::UnifiedInput clockwise;
                //! Positive around positive Z
                input::UnifiedInput counter_clockwise;

                // Factors
                //! Left-right (strafing) speed (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_y;
                //! Roll rate (degree / second)
                float roll_rate;
            } config;


            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and config
            TopDown(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Config c)
                    : Controls(s), transform_mgr(std::move(t)), config(c) {}

            void transform() override;

            void show_debug() override;
    };

    /**
     * @}
     */

};

#endif //OPEN_SEA_CONTROLS_H
