/** \file ImGui.h
 * ImGUI integration
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_IMGUI_H
#define OPEN_SEA_IMGUI_H

namespace open_sea::imgui {
    void init();
    void new_frame();
    void render();
    void clean_up();

    void key_callback(int key, int scancode, int action, int mods);
    void mouse_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);
    void char_callback(unsigned int codepoint);
}

#endif //OPEN_SEA_IMGUI_H
