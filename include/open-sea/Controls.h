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
        public:
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Entity being controlled
            ecs::Entity subject;
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

            void showDebug();
    };

};

#endif //OPEN_SEA_CONTROLS_H
