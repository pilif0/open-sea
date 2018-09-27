/** \file Window.h
 * Window module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_WINDOW_H
#define OPEN_SEA_WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <boost/signals2.hpp>
namespace signals = boost::signals2;

#include <string>
#include <memory>

//! All window state and related functions
namespace open_sea::window {
    /**
     * \addtogroup Window
     * \brief All window state and related functions
     *
     * All window properties, state and related functions.
     * There is only one window at all times.
     * The window is not resizable.
     *
     * @{
     */

    //! Possible window states
    enum window_state {
        windowed,   //!< Windowed
        borderless, //!< Borderless window
        fullscreen  //!< Fullscreen
    };

    //! Default property values
    namespace defaults {
        constexpr int16_t width = 1280;             //!< Width
        constexpr int16_t height = 720;             //!< Height
        constexpr int16_t fb_width = 1280;          //!< Frame buffer width
        constexpr int16_t fb_height = 720;          //!< Frame buffer height
        constexpr const char* title = "Game";       //!< Title
        constexpr ::GLFWmonitor* monitor = nullptr; //!< Monitor
        constexpr window_state state = windowed;    //!< State
        constexpr bool v_sync = false;              //!< vSync
    }

    //! Set of window properties
    struct WindowProperties {
        int width = defaults::width;                //!< Width
        int height = defaults::height;              //!< Height
        int fb_width = defaults::fb_width;          //!< Frame buffer width (horizontal resolution)
        int fb_height = defaults::fb_height;        //!< Frame buffer height (vertical resolution)
        std::string title = defaults::title;        //!< Title
        ::GLFWmonitor* monitor = defaults::monitor; //!< Monitor or \c nullptr if windowed
        window_state state = defaults::state;       //!< State
        bool v_sync = defaults::v_sync;             //!< Whether vSync is on
    };

    //! Alias for the Signals2 connection type
    typedef signals::connection connection;

    // Signal types
    //! Size signal type
    typedef signals::signal<void (int, int)> size_signal_t;
    //! Focus signal type
    typedef signals::signal<void (bool)> focus_signal_t;
    //! Close signal type
    typedef signals::signal<void ()> close_signal_t;

    extern ::GLFWwindow* window;

    bool init();

    bool make_windowed(int width, int height);
    bool make_borderless(::GLFWmonitor* monitor);
    bool make_fullscreen(int width, int height, ::GLFWmonitor* monitor);

    void set_title(const std::string& title);
    void set_size(int width, int height);
    void enable_v_sync();
    void center();
    void show();
    void hide();
    void close();
    WindowProperties current_properties();
    bool is_focused();

    connection connect_size(const size_signal_t::slot_type& slot);
    connection connect_focus(const focus_signal_t::slot_type& slot);
    connection connect_close(const close_signal_t::slot_type& slot);

    void update();

    void clean_up();
    void terminate();
    bool should_close();

    void debug_window(bool *open);
    void show_modify();

    /**
     * @}
     */
}

#endif //OPEN_SEA_WINDOW_H
