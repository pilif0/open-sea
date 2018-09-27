/** \file Input.h
 * Input module
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_INPUT_H
#define OPEN_SEA_INPUT_H

#include <glm/vec2.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <boost/signals2.hpp>
namespace signals = boost::signals2;

#include <string>
#include <set>

namespace open_sea::input {
    /**
     * \addtogroup Input
     * \brief All input related code
     *
     * @{
     */

    //! GLFW name for the unknown key
    constexpr int unknown_key = GLFW_KEY_UNKNOWN;

    //! Possible states of a key
    enum state {
        press,  //!< Pressed
        repeat, //!< Held down
        release //!< Released
    };

    //! Alias for the Signals2 connection type
    typedef signals::connection connection;

    // Signal types
    //! Key signal type
    typedef signals::signal<void (int, int, state, int)> key_signal;
    //! Cursor entrance signal type
    typedef signals::signal<void (bool)> enter_signal;
    //! Mouse button signal type
    typedef signals::signal<void (int, state, int)> mouse_signal;
    //! Scroll signal type
    typedef signals::signal<void (double, double)> scroll_signal;
    //! Character signal type
    typedef signals::signal<void (unsigned int)> character_signal;

    //! Unified input key
    struct UnifiedInput {
        //! Device code
        // 0 -> KB, 1 -> Mouse
        unsigned int device : 4;
        //! Device-specific input code
        // KB -> scancode, Mouse -> button no. (0 based)
        unsigned int code : 28;

        friend bool operator<(const open_sea::input::UnifiedInput &lhs, const open_sea::input::UnifiedInput &rhs) {
            return ((lhs.device << 28) + lhs.code) < ((rhs.device << 28) + rhs.code);
        }

        friend bool operator==(const UnifiedInput &lhs, const UnifiedInput &rhs) {
            return lhs.device == rhs.device && lhs.code == rhs.code;
        }

        //! Create unified input from a GLFW keyboard key
        static UnifiedInput keyboard(int key) { return UnifiedInput{.device = 0, .code = static_cast<unsigned int>(::glfwGetKeyScancode(key))}; }
        //! Create unified input from a GLFW mouse button
        static UnifiedInput mouse(int button) { return UnifiedInput{.device = 1, .code = static_cast<unsigned int>(button)}; }

        std::string str() const;
    };
    //! Unified input signal
    typedef signals::signal<void (UnifiedInput, state)> unified_signal;
    bool is_held(UnifiedInput input);

    void init();
    void reattach();

    connection connect_key(const key_signal::slot_type& slot);
    connection connect_enter(const enter_signal::slot_type& slot);
    connection connect_mouse(const mouse_signal::slot_type& slot);
    connection connect_scroll(const scroll_signal::slot_type& slot);
    connection connect_character(const character_signal::slot_type& slot);
    connection connect_unified(const unified_signal::slot_type &slot);

    ::glm::dvec2 cursor_position();
    glm::dvec2 cursor_delta();
    void update_cursor_delta();
    state key_state(int key);
    state mouse_state(int button);
    std::string key_name(int key, int scancode);

    namespace cursor_mode {
        //! Possible cursor modes
        enum mode { normal, hidden, disabled };
    }
    void set_cursor_mode(cursor_mode::mode mode);
    cursor_mode::mode get_cursor_mode();

    const char* get_clipboard();
    void set_clipboard(const char* in);

    void debug_window(bool *open);

    /**
     * @}
     */
}

#endif //OPEN_SEA_INPUT_H
