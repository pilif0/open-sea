/** \file Debug.h
 * Debug GUI
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_DEBUG_H
#define OPEN_SEA_DEBUG_H

#include <open-sea/ImGui.h>
#include <glm/glm.hpp>

#include <functional>

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
    //! Type of menu items that toggle display of a window for the debuggable
    typedef std::tuple<std::shared_ptr<Debuggable>, std::string, bool> menu_item;

    void add_entity_manager(const std::shared_ptr<Debuggable> &em, const std::string &label);
    void remove_entity_manager(const std::shared_ptr<Debuggable> &em);

    void add_component_manager(const std::shared_ptr<Debuggable> &com, const std::string &label);
    void remove_component_manager(const std::shared_ptr<Debuggable> &com);

    void add_system(const std::shared_ptr<Debuggable> &sys, const std::string &label);
    void remove_system(const std::shared_ptr<Debuggable> &sys);

    void add_controls(const std::shared_ptr<Debuggable> &con, const std::string &label);
    void remove_controls(const std::shared_ptr<Debuggable> &con);

    //! Function that defines the contents of a menu (called between \c ImGui::BeginMenu and \c ImGui::EndMenu)
    typedef std::function<void()> menu_func;
    //! Type of menus (contents defined in the function)
    typedef std::tuple<menu_func, std::string> menu;

    unsigned add_menu(const menu_func &f, const std::string &label);
    void remove_menu(unsigned id);

    void show_matrix(const glm::mat4 &m);

    void clean_up();
}

#endif //OPEN_SEA_DEBUG_H
