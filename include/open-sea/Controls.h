/** \file Controls.h
 * Components and systems related to entity controls
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_CONTROLS_H
#define OPEN_SEA_CONTROLS_H

#include <open-sea/Controls.h>
#include <open-sea/Log.h>
#include <open-sea/Entity.h>
#include <open-sea/Components.h>
#include <open-sea/Input.h>

namespace open_sea::controls {

    /** \class Controls
     * \brief Abstract base class for all controls
     */
    class Controls {
        protected:
            //! Entity being controlled
            ecs::Entity subject;
            //! Last applied translation
            glm::vec3 lastTranslate{};
            //! Last applied rotation
            glm::quat lastRotate;

        public:
            Controls(ecs::Entity s) : subject(s), lastRotate(glm::quat()) {}
            virtual void transform() = 0;
            virtual void setSubject(ecs::Entity newSubject);
            virtual ecs::Entity getSubject() const;

            virtual void showDebug();

            virtual ~Controls() {}
    };

    /** \class FreeControls
     * \brief Controls an entity using free controls
     * Controls an entity using free controls.
     * No restrictions on orientation or position control.
     * Unified input control over X, Y, Z position and roll.
     * Mouse control over pitch and yaw.
     * All control is from the perspective of the entity.
     * Only one entity should be controlled at one time.
     */
    class Free : public Controls {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Negative Z
                input::unified_input forward;
                //! Positive Z
                input::unified_input backward;
                //! Negative X
                input::unified_input left;
                //! Positive X
                input::unified_input right;
                //! Positive Y
                input::unified_input up;
                //! Negative Y
                input::unified_input down;
                //! Negatvie around positive Z
                input::unified_input clockwise;
                //! Positive around positive Z
                input::unified_input counter_clockwise;

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
                    : transformMgr(std::move(t)), config(c), Controls(s) {}

            void transform() override;

            void showDebug() override;
    };

    /** \class FPS
     * \brief Controls an entity using FPS controls
     * Controls an entity using FPS controls.
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
            float pitch;
            void updatePitch();

        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Negative Z
                input::unified_input forward;
                //! Positive Z
                input::unified_input backward;
                //! Negative X
                input::unified_input left;
                //! Positive X
                input::unified_input right;

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
                    : transformMgr(std::move(t)), config(c), pitch(0.0f), Controls(s) { updatePitch(); }

            void transform() override;
            void setSubject(ecs::Entity newSubject) override;

            void showDebug() override;
    };

    /** \class TopDown
     * \brief Controls an entity using top down controls
     * Controls an entity using top down controls.
     * Position control in XY plane (from the perspective of the entity) without restriction through unified input.
     * Rotation control of roll around entity's Z axis through unified input.
     * No control over yaw, pitch and movement along the Z axis.
     * Only one entity should be controlled at one time.
     */
    class TopDown : public Controls {
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Config {
                // Key bindings
                //! Positive Y
                input::unified_input up;
                //! Negative Y
                input::unified_input down;
                //! Negative X
                input::unified_input left;
                //! Positive X
                input::unified_input right;
                //! Negative around positive Z
                input::unified_input clockwise;
                //! Positive around positive Z
                input::unified_input counter_clockwise;

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
                    : transformMgr(std::move(t)), config(c), Controls(s) {}

            void transform() override;

            void showDebug() override;
    };

};

#endif //OPEN_SEA_CONTROLS_H
