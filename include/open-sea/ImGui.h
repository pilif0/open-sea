/** \file ImGui.h
 * ImGui integration
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

//! Dear ImGui integration
namespace open_sea::imgui {
    /**
     * \addtogroup ImGui
     * \brief Dear ImGui integration
     *
     * General Dear ImGui integration.
     * Takes care of setup, updating, rendering, clean up and input.
     *
     * @{
     */

    void init();
    void new_frame();
    void render();
    void clean_up();

    void key_callback(int key, int scancode, int action, int mods);
    void mouse_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);
    void char_callback(unsigned int codepoint);

    /**
     * @}
     */
}

#endif //OPEN_SEA_IMGUI_H
