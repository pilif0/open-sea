/** \file Window.h
 * Window module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_WINDOW_H
#define OPEN_SEA_WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <memory>

namespace open_sea::window {
    //! Game window
    extern ::GLFWwindow* window;

    //! Possible window states
    enum window_state {
        windowed,   //! Windowed
        borderless, //! Borderless window
        fullscreen  //! Fullscreen
    };

    //! Default property values
    namespace defaults {
        constexpr int16_t width = 1280;
        constexpr int16_t height = 720;
        constexpr int16_t resolutionH = 1280;
        constexpr int16_t resolutionV = 720;
        constexpr const char* title = "Game";
        constexpr ::GLFWmonitor* monitor = nullptr;
        constexpr window_state state = windowed;
        constexpr bool vSync = false;
    }

    //! Set of window properties
    struct window_properties {
        int16_t width = defaults::width;            //! Width
        int16_t height = defaults::height;          //! Height
        int16_t resolutionH = defaults::resolutionH;//! Horizontal resolution
        int16_t resolutionV = defaults::resolutionV;//! Vertical resolution
        std::string title = defaults::title;        //! Title
        ::GLFWmonitor* monitor = defaults::monitor; //! Monitor or \c nullptr if windowed
        window_state state = defaults::state;       //! State
        bool vSync = defaults::vSync;               //! Whether vSync is on
    };

    //! Current window properties (empty until the window is created)
    extern std::unique_ptr<window_properties> current;

    bool init_glfw();
    void create_window(uint16_t width, uint16_t height, const std::string& title = nullptr, ::GLFWmonitor* monitor = nullptr);
    bool should_close();
    void set_state(window_state newState);
    void set_size(uint16_t width, uint16_t height);
    void set_resolution(uint16_t horizontal, uint16_t vertical);
    void enableVSync();
    void clean_up();
}

#endif //OPEN_SEA_WINDOW_H
