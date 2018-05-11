/** \file ImGui.h
 * ImGUI integration
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_IMGUI_H
#define OPEN_SEA_IMGUI_H

#include <imgui.h>

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
}

#endif //OPEN_SEA_IMGUI_H
