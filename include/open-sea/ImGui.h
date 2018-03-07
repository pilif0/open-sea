/** \file ImGUI.h
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

    void connect_listeners();
    void disconnect_listeners();
}

#endif //OPEN_SEA_IMGUI_H
