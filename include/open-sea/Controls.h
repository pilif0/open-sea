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

    /** \class FreeControls
     * \brief Controls an entity using free controls
     * Controls an entity using free controls.
     * No restrictions on orientation or position control.
     * Unified input control over X, Y, Z position and roll.
     * Mouse control over pitch and yaw.
     * Orientation control enabled by a key (as it requires the cursor to be disabled) - this includes roll control.
     * Only one entity should be controlled at one time.
     */
    class FreeControls {
        private:
            //! Entity being controlled
            ecs::Entity subject;

        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Controls {
                // Key bindings
                input::unified_input forward;
                input::unified_input backward;
                input::unified_input left;
                input::unified_input right;
                input::unified_input up;
                input::unified_input down;
                //! Clockwise roll key
                input::unified_input clockwise;
                //! Counter clockwise roll key
                input::unified_input counter_clockwise;
                //! Key that turns orientation control on when held
                input::unified_input turn;

                // Factors
                //! Left-right (strafing) speed in (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_z;
                //! Up-down speed (units / second)
                float speed_y;
                //! Degrees turned per screen unit of mouse movement
                float turn_rate;    //TODO: should probably be independent of the screen size (deg per screen fraction?)
                //! Degrees per second
                float roll_rate;
            } controls;

            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and controls.
            FreeControls(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Controls c)
                    : transformMgr(std::move(t)), subject(s), controls(c) {}

            void transform();
            void setSubject(ecs::Entity newSubject);
            ecs::Entity getSubject();

            void showDebug();
    };

    /** \class FPSControls
     * \brief Controls an entity using FPS controls
     * Controls an entity using FPS controls.
     * Position control in XZ plane without restriction through unified input.
     * Rotation control of yaw around global Y axis and of pitch within [+90,-90] degrees through mouse input.
     * No control over roll and movement along the Y axis.
     * Orientation control always on (disables cursor).
     * Only one entity should be controlled at one time.
     */
    // Note: this still allows Y motion through external means, just not through this specific control
    class FPSControls {
        private:
            //! Current subject's pitch (in degrees)
            float pitch;
            //! Entity being controlled
            ecs::Entity subject;
            void updatePitch();

        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Controls {
                // Key bindings
                input::unified_input forward;
                input::unified_input backward;
                input::unified_input left;
                input::unified_input right;

                // Factors
                //! Left-right (strafing) speed in (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_z;
                //! Degrees turned per screen unit of mouse movement
                float turn_rate;    //TODO: should probably be independent of the screen size (deg per screen fraction?)
            } controls;


            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and controls.
            FPSControls(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Controls c)
                    : transformMgr(std::move(t)), subject(s), controls(c), pitch(0.0f) { updatePitch(); }

            void transform();
            void setSubject(ecs::Entity newSubject);
            ecs::Entity getSubject();

            void showDebug();
    };

    /** \class TopDownControls
     * \brief Controls an entity using top down controls
     * Controls an entity using top down controls.
     * Position control in XY plane without restriction through unified input.
     * Rotation control of roll around global Z axis through unified input.
     * No control over yaw, pitch and movement along the Z axis.
     * Only one entity should be controlled at one time.
     */
    class TopDownControls {
        private:
            //! Entity being controlled
            ecs::Entity subject;

        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Key bindings and factors
            struct Controls {
                // Key bindings
                input::unified_input up;
                input::unified_input down;
                input::unified_input left;
                input::unified_input right;
                input::unified_input clockwise;
                input::unified_input counter_clockwise;

                // Factors
                //! Left-right (strafing) speed in (units / second)
                float speed_x;
                //! Forward-backward speed (units / second)
                float speed_y;
                //! Degrees per second
                float roll_rate;
            } controls;


            //! Constuct the controls assigning it a pointer to the relevant transformation component manager, subject and controls.
            TopDownControls(std::shared_ptr<ecs::TransformationComponent> t, ecs::Entity s, Controls c)
                    : transformMgr(std::move(t)), subject(s), controls(c) {}

            void transform();
            void setSubject(ecs::Entity newSubject);
            ecs::Entity getSubject();

            void showDebug();
    };

};

#endif //OPEN_SEA_CONTROLS_H
