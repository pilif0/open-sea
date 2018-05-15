/*
 * Debug gui implementation
 */

#include <open-sea/Debug.h>
#include <open-sea/Window.h>
#include <open-sea/Delta.h>
#include <open-sea/Input.h>
#include <open-sea/GL.h>

namespace open_sea::debug {
    //! List of registered entity managers
    std::vector<menu_record> em_list{};
    //! List of registered component managers
    std::vector<menu_record> com_list{};
    //! List of registered systems
    std::vector<menu_record> sys_list{};

    /**
     * \brief Set next window width to the standard width
     */
    void set_standard_width() {
        ImGui::SetNextWindowSize(ImVec2(STANDARD_WIDTH, 0.0f), ImGuiCond_Once);
    }

    /**
     * \brief Add a main menu bar item for the entity manager
     *
     * \param em Entity Manager
     * \param label Label
     */
    void add_entity_manager(std::shared_ptr<Debuggable> em, std::string label) {
        em_list.push_back({em, label, false});
    }

    /**
     * \brief Remove all main menu bar items for the entity manager
     *
     * \param em Entity Manager
     */
    void remove_entity_manager(std::shared_ptr<Debuggable> em) {
        for (auto i = em_list.begin(); i < em_list.end(); i++){
            if (std::get<0>(*i) == em)
                em_list.erase(i);
        }
    }

    /**
     * \biref Add a main menu bar item for the component manager
     *
     * \param com Component Manager
     * \param label Label
     */
    void add_component_manager(std::shared_ptr<Debuggable> com, std::string label) {
        com_list.push_back({com, label, false});
    }

    /**
     * \brief Remove all main menu bar items for the component manager
     *
     * \param com Component Manager
     */
    void remove_component_manager(std::shared_ptr<Debuggable> com) {
        for (auto i = com_list.begin(); i < com_list.end(); i++){
            if (std::get<0>(*i) == com)
                com_list.erase(i);
        }
    }

    /**
     * \brief Add a main menu bar item for the system
     *
     * \param sys System
     * \param label Label
     */
    void add_system(std::shared_ptr<Debuggable> sys, std::string label) {
        sys_list.push_back({sys, label, false});
    }

    /**
     * \brief Remove all main menu bar items for the system
     *
     * \param sys System
     */
    void remove_system(std::shared_ptr<Debuggable> sys) {
        for (auto i = sys_list.begin(); i < sys_list.end(); i++){
            if (std::get<0>(*i) == sys)
                sys_list.erase(i);
        }
    }

    /**
     * \brief Show the main menu and all windows controlled by it
     */
    void main_menu() {
        // Window open flags
        static bool time = false;
        static bool window = false;
        static bool input = false;
        static bool opengl = false;
        static bool imgui_demo = false;

        if (ImGui::BeginMainMenuBar()) {
            // System menu
            if (ImGui::BeginMenu("System")) {
                if (ImGui::MenuItem("Time", nullptr, &time)) {}
                if (ImGui::MenuItem("Window", nullptr, &window)) {}
                if (ImGui::MenuItem("Input", nullptr, &input)) {}
                if (ImGui::MenuItem("OpenGL", nullptr, &opengl)) {}

                ImGui::Separator();

                if (ImGui::MenuItem("Exit")) { window::close(); }

                ImGui::EndMenu();
            }

            // ECS
            if (ImGui::BeginMenu("ECS")) {
                if (ImGui::BeginMenu("Entity Managers:")) {
                    // Print a menu item for each registered entity manager
                    for (auto i = em_list.begin(); i < em_list.end(); i++) {
                        if (ImGui::MenuItem(std::get<1>(*i).c_str(), nullptr, &std::get<2>(*i))) {}
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Component Managers:")) {
                    // Print a menu item for each registered component manager
                    for (auto i = com_list.begin(); i < com_list.end(); i++) {
                        if (ImGui::MenuItem(std::get<1>(*i).c_str(), nullptr, &std::get<2>(*i))) {}
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Systems:")) {
                    // Print a menu item for each registered system
                    for (auto i = sys_list.begin(); i < sys_list.end(); i++) {
                        if (ImGui::MenuItem(std::get<1>(*i).c_str(), nullptr, &std::get<2>(*i))) {}
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            // Demos
            if (ImGui::BeginMenu("Demos")) {
                if (ImGui::MenuItem("Dear ImGui", nullptr, &imgui_demo)) {}

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // System windows
        if (time) time::debug_window(&time);
        if (window) window::debug_window(&window);
        if (input) input::debug_window(&input);
        if (opengl) gl::debug_window(&opengl);

        // Demo windows
        if (imgui_demo) ImGui::ShowDemoWindow(&imgui_demo);

        // Entity managers
        for (auto i = em_list.begin(); i < em_list.end(); i++) {
            if (std::get<2>(*i)) {
                set_standard_width();

                if (ImGui::Begin(std::string("Entity Manager - ").append(std::get<1>(*i)).c_str(), &std::get<2>(*i))) {
                    (std::get<0>(*i))->showDebug();
                }
                ImGui::End();
            }
        }

        // Entity managers
        for (auto i = com_list.begin(); i < com_list.end(); i++) {
            if (std::get<2>(*i)) {
                set_standard_width();

                if (ImGui::Begin(std::string("Component Manager - ").append(std::get<1>(*i)).c_str(), &std::get<2>(*i))) {
                    (std::get<0>(*i))->showDebug();
                }
                ImGui::End();
            }
        }

        // Entity managers
        for (auto i = sys_list.begin(); i < sys_list.end(); i++) {
            if (std::get<2>(*i)) {
                set_standard_width();

                if (ImGui::Begin(std::string("System - ").append(std::get<1>(*i)).c_str(), &std::get<2>(*i))) {
                    (std::get<0>(*i))->showDebug();
                }
                ImGui::End();
            }
        }
    }
}
