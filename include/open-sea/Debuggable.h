/** \file Debuggable.h
 * Interface for debuggable objects
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_DEBUGGABLE_H
#define OPEN_SEA_DEBUGGABLE_H

namespace open_sea::debug {
    /**
     * \addtogroup Debug
     *
     * @{
     */

    /**
     * \brief Interface for any object that has a debug window
     */
    class Debuggable {
        public:
            //! Show debug information (called within an ImGui window)
            virtual void showDebug() = 0;
    };

    /**
     * @}
     */
}

#endif //OPEN_SEA_DEBUGGABLE_H
