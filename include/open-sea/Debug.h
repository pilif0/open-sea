/** \file Debug.h
 * Debug GUI
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_DEBUG_H
#define OPEN_SEA_DEBUG_H

#include <open-sea/ImGui.h>

namespace open_sea::debug {
    //! Standard width for debug windows
    // This is to provide some unified appearance and easy arrangement into columns
    constexpr float STANDARD_WIDTH = 350.0f;
    void set_standard_width();
    void main_menu();

    /**
     * \brief Interface for any object that has a debug window
     */
    class Debuggable {
        public:
            //! Show debug information (called within an ImGui window)
            virtual void showDebug() = 0;
    };
    typedef std::tuple<std::shared_ptr<Debuggable>, std::string, bool> menu_record;

    void add_entity_manager(std::shared_ptr<Debuggable> em, std::string label);
    void remove_entity_manager(std::shared_ptr<Debuggable> em);

    void add_component_manager(std::shared_ptr<Debuggable> com, std::string label);
    void remove_component_manager(std::shared_ptr<Debuggable> com);

    void add_system(std::shared_ptr<Debuggable> sys, std::string label);
    void remove_system(std::shared_ptr<Debuggable> sys);
}

#endif //OPEN_SEA_DEBUG_H
