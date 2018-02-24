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
    // General Assumptions:
    //  - Only one window at all times
    //  - The window is not resizable

    //! Possible window states
    enum window_state {
        windowed,   //!< Windowed
        borderless, //!< Borderless window
        fullscreen  //!< Fullscreen
    };

    //! Default property values
    namespace defaults {
        constexpr int16_t width = 1280;
        constexpr int16_t height = 720;
        constexpr int16_t fbWidth = 1280;
        constexpr int16_t fbHeight = 720;
        constexpr const char* title = "Game";
        constexpr ::GLFWmonitor* monitor = nullptr;
        constexpr window_state state = windowed;
        constexpr bool vSync = false;
    }

    //! Set of window properties
    struct window_properties {
        int width = defaults::width;                //!< Width
        int height = defaults::height;              //!< Height
        int fbWidth = defaults::fbWidth;            //!< Frame buffer width (horizontal resolution)
        int fbHeight = defaults::fbHeight;          //!< Frame buffer height (vertical resolution)
        std::string title = defaults::title;        //!< Title
        ::GLFWmonitor* monitor = defaults::monitor; //!< Monitor or \c nullptr if windowed
        window_state state = defaults::state;       //!< State
        bool vSync = defaults::vSync;               //!< Whether vSync is on
    };

    extern ::GLFWwindow* window;

    bool init();

    bool make_windowed(int width, int height);
    bool make_borderless(::GLFWmonitor* monitor);
    bool make_fullscreen(int width, int height, ::GLFWmonitor* monitor);

    void set_title(const std::string& title);
    void set_size(int width, int height);
    void enable_vSync();
    void center();
    void show();
    void hide();
    void close();
    window_properties current_properties();

    void set_size_callback(::GLFWwindowsizefun cbfun);
    void set_focus_callback(::GLFWwindowfocusfun cbfun);
    void set_close_callback(::GLFWwindowclosefun cbfun);

    void update();

    void clean_up();
    void terminate();
    bool should_close();
}

#endif //OPEN_SEA_WINDOW_H
