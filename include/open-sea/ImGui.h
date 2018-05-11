/** \file ImGui.h
 * ImGUI integration
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_IMGUI_H
#define OPEN_SEA_IMGUI_H

#include <imgui.h>

#include <utility>
#include <vector>
#include <string>
#include <memory>

namespace open_sea::imgui {
    void init();
    void new_frame();
    void render();
    void clean_up();

    void key_callback(int key, int scancode, int action, int mods);
    void mouse_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);
    void char_callback(unsigned int codepoint);

    //! Standard width for debug windows
    // This is to provide some sort of unified appearance and easy arrangement into columns
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

#endif //OPEN_SEA_IMGUI_H
