/** \file Input.cpp
 * Input implementation
 *
 * \author Filip Smola
 */
#include <open-sea/Input.h>
#include <open-sea/Log.h>
#include <open-sea/Window.h>
#include <open-sea/ImGui.h>
namespace w = open_sea::window;

#include <sstream>

namespace open_sea::input {
    //! Module logger
    log::severity_logger lg = log::get_logger("Input");

    // Signals
    //! Key signal
    std::unique_ptr<key_signal> keyboard;
    //! Cursor entrance signal
    std::unique_ptr<enter_signal> enter;
    //! Mouse button signal
    std::unique_ptr<mouse_signal> mouse;
    //! Scroll signal
    std::unique_ptr<scroll_signal> scroll;
    //! Character signal
    std::unique_ptr<character_signal> character;
    //! Unified input signal
    std::unique_ptr<unified_signal> unified;

    // Unified input
    //! Set of held down unified inputs
    std::set<UnifiedInput> unified_state;

    // Callbacks
    /**
     * \brief Send signal about a key event
     *
     * \param window Event window
     * \param key Relevant GLFW key code
     * \param scancode System-specific key code
     * \param action Relevant action
     * \param mods Bitfield of applied modifier keys
     */
    void key_callback(::GLFWwindow* window, int key, int scancode, int action, int mods) {
        // Skip if not the global window
        if (window != w::window) {
            return;
        }

        // Transform values
        state state = (action == GLFW_PRESS) ? press : (action == GLFW_REPEAT) ? repeat : release;

        // Update unified input state
        UnifiedInput ui{0, static_cast<unsigned int>(scancode)};
        if (action == GLFW_PRESS) {
            // Press -> insert
            unified_state.insert(ui);
            (*unified)(ui, press);
        } else if(action == GLFW_RELEASE) {
            // Release -> remove
            unified_state.erase(ui);
            (*unified)(ui, release);
        }

        // Fire a signal
        static bool imgui_waits_esc = false;        // Fixes #12: True when ImGui is waiting for ESC release signal
        static bool imgui_waits_ent = false;        // Fixes #14: True when ImGui is waiting for Enter release signal
        if (ImGui::GetIO().WantCaptureKeyboard) {
            // Let ImGui take all input when it wants it
            open_sea::imgui::key_callback(key, scancode, state, mods);

            if (key == GLFW_KEY_ESCAPE && state == press) {
                imgui_waits_esc = true;
            }
            if (key == GLFW_KEY_ENTER && state == press) {
                imgui_waits_ent = true;
            }
        } else {
            if (imgui_waits_esc && key == GLFW_KEY_ESCAPE && state == release) {
                open_sea::imgui::key_callback(key, scancode, state, mods);
                imgui_waits_esc = false;
            } else if(imgui_waits_ent && key == GLFW_KEY_ENTER && state == release){
                open_sea::imgui::key_callback(key, scancode, state, mods);
                imgui_waits_ent = false;
            } else {
                (*keyboard)(key, scancode, state, mods);
            }
        }
    }

    /**
     * \brief Send signal about the cursor entering/leaving the window
     *
     * \param window Event window
     * \param entered \c GLFW_TRUE when entered, \c GLFW_FALSE when left
     */
    void cursor_enter_callback(::GLFWwindow* window, int entered) {
        // Skip if not the global window
        if (window != w::window) {
            return;
        }

        // Fire the signal
        (*enter)(entered == GLFW_TRUE);
    }

    /**
     * \brief Send signal about a mouse button event
     *
     * \param window Event window
     * \param button Relevant GLFW button code
     * \param action Relevan action
     * \param mods Bitfield of applied modifier keys
     */
    void mouse_button_callback(::GLFWwindow* window, int button, int action, int mods) {
        // Skip if not the global window
        if (window != w::window) {
            return;
        }

        // Transform values
        state state = (action == GLFW_PRESS) ? press : (action == GLFW_REPEAT) ? repeat : release;

        // Update unified input state
        UnifiedInput ui{1, static_cast<unsigned int>(button)};
        if (action == GLFW_PRESS) {
            // Press -> insert
            unified_state.insert(ui);
            (*unified)(ui, press);
        } else if(action == GLFW_RELEASE) {
            // Release -> remove
            unified_state.erase(ui);
            (*unified)(ui, release);
        }

        // Fire a signal
        if (ImGui::GetIO().WantCaptureMouse) {
            open_sea::imgui::mouse_callback(button, state, mods);
        } else {
            (*mouse)(button, state, mods);
        }
    }

    /**
     * \brief Send signal about a scroll event
     *
     * \param window Event window
     * \param xoffset Horizontal scroll offset
     * \param yoffset Vertical scroll offset
     */
    void scroll_callback(::GLFWwindow* window, double xoffset, double yoffset) {
        // Skip if not the global window
        if (window != w::window) {
            return;
        }

        // Fire a signal
        if (ImGui::GetIO().WantCaptureMouse) {
            open_sea::imgui::scroll_callback(xoffset, yoffset);
        } else {
            (*scroll)(xoffset, yoffset);
        }
    }

    /**
     * \brief Send signal about a character event
     *
     * \param window Event window
     * \param codepoint Unicode codepoint of the character
     */
    void character_callback(::GLFWwindow* window, unsigned int codepoint) {
        // Skip if not the global window
        if (window != w::window) {
            return;
        }

        // Fire a signal
        if (ImGui::GetIO().WantCaptureKeyboard) {
            open_sea::imgui::char_callback(codepoint);
        } else {
            (*character)(codepoint);
        }
    }

    /**
     * \brief Whether the unified input is held down
     *
     * \param input Unified input to check
     * \return Whether the input is held down
     */
    bool is_held(UnifiedInput input) {
        return unified_state.count(input) != 0;
    }

    /**
     * \brief Reattach the signals to the current global window
     */
    void reattach() {
        if (w::window) {
            log::log(lg, log::info, "Reattaching input...");

            ::glfwSetKeyCallback(w::window, key_callback);
            ::glfwSetCursorEnterCallback(w::window, cursor_enter_callback);
            ::glfwSetMouseButtonCallback(w::window, mouse_button_callback);
            ::glfwSetScrollCallback(w::window, scroll_callback);
            ::glfwSetCharCallback(w::window, character_callback);

            log::log(lg, log::info, "Reattached input");
        }
    }

    /**
     * \brief Get string representation of the unified input as "(device) code"
     *
     * \return String representation
     */
    std::string UnifiedInput::str() const {
        std::string device_str;
        switch (device) {
            case 0:
                device_str = "Keyboard";
                break;
            case 1:
                device_str = "Mouse";
                break;
            default:
                device_str = "Unknown - ";
                device_str.append(std::to_string(device));
        }

        std::ostringstream result;
        result << "(" << device_str << ") " << code;
        return result.str();
    }

    /**
     * \brief Initialize input
     *
     * Instantiate the various signals and attach them to the current global window.
     */
    void init() {
        log::log(lg, log::info, "Input initializing...");

        // Instantiate the signals
        keyboard = std::make_unique<key_signal>();
        enter = std::make_unique<enter_signal>();
        mouse = std::make_unique<mouse_signal>();
        scroll = std::make_unique<scroll_signal>();
        character = std::make_unique<character_signal>();
        unified = std::make_unique<unified_signal>();

        // Attach the callbacks
        reattach();

        log::log(lg, log::info, "Input initialized");
    }

    /**
     * \brief Get cursor position
     *
     * Get position of the cursor in screen coordinates relative to origin of the window (top left).
     *
     * \return Cursor position
     */
    ::glm::dvec2 cursor_position() {
        double x, y;
        ::glfwGetCursorPos(w::window, &x, &y);
        return glm::dvec2(x, y);
    }

    //! Latest cursor delta
    ::glm::dvec2 cursor_d{};

    /**
     * \brief Get latest cursor delta
     *
     * \return Latest cursor delta
     */
    ::glm::dvec2 cursor_delta() {
        return cursor_d;
    }

    //! Cursor position during last cursor delta update
    ::glm::dvec2 last_cursor_pos{};

    /**
     * \brief Update cursor delta
     *
     * Compute new cursor delta based on current and last cursor position
     */
    void update_cursor_delta() {
        glm::dvec2 pos = cursor_position();
        cursor_d = pos - last_cursor_pos;
        last_cursor_pos = pos;
    }

    /**
     * \brief Get key state
     *
     * Get state of a key. Only returns \c press or \c release (due to GLFW).
     *
     * \param key GLFW key name
     * \return \c press if pressed, \c release otherwise
     */
    state key_state(int key) {
        return (::glfwGetKey(w::window, key) == GLFW_PRESS) ? press : release;
    }

    /**
     * \brief Get mouse button state
     *
     * Get the state of a mouse button. Only returns \c press or \c release (due to GLFW).
     *
     * \param button GLFW mouse button name
     * \return \c press if pressed, \c release otherwise
     */
    state mouse_state(int button) {
        return (::glfwGetMouseButton(w::window, button) == GLFW_PRESS) ? press : release;
    }

    /**
     * \brief Get key name
     *
     * Get the name of the supplied key.
     * If \c key is \c unknown_key then \c scancode is used, otherwise it is ignored.
     * Returns "undefined" when no name is found.
     *
     * Note: unsupported by GLFW on Wayland, returns "undefined" for all keys
     *
     * \param key GLFW key name
     * \param scancode System-specific key code
     * \return Name of the key
     */
    std::string key_name(int key, int scancode) {
        const char* cs = ::glfwGetKeyName(key, scancode);
        if (cs == nullptr) {
            return std::string("undefined");
        }

        return std::string(cs);
    }

    //! Current cursor mode (only reliable if \c cursor_mode_true is \c true)
    cursor_mode::mode current_cursor_mode = cursor_mode::normal;
    //! Flag that is \c false until the first time \c get_cursor_mode() is called
    bool cursor_mode_true = false;

    /**
     * \brief Set cursor mode
     *
     * \param mode New value
     */
    void set_cursor_mode(cursor_mode::mode mode) {
        if (get_cursor_mode() != mode) {
            glfwSetInputMode(window::window, GLFW_CURSOR,
                             (mode == cursor_mode::normal) ?
                                 GLFW_CURSOR_NORMAL :
                                 (mode == cursor_mode::hidden) ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
            current_cursor_mode = mode;
        }
    }

    /**
     * \brief Get cursor mode
     *
     * \return Cursor mode
     */
    cursor_mode::mode get_cursor_mode() {
        if (!cursor_mode_true) {
            auto glfw_mode = glfwGetInputMode(window::window, GLFW_CURSOR);
            switch (glfw_mode) {
                case GLFW_CURSOR_NORMAL: current_cursor_mode = cursor_mode::normal; break;
                case GLFW_CURSOR_HIDDEN: current_cursor_mode = cursor_mode::hidden; break;
                case GLFW_CURSOR_DISABLED: current_cursor_mode = cursor_mode::disabled; break;
                default: current_cursor_mode = cursor_mode::normal;
            }
            cursor_mode_true = true;
        }

        return current_cursor_mode;
    }

    /**
     * \brief Get clipboard content
     *
     * \return Clipboard content
     */
    const char* get_clipboard() {
        return ::glfwGetClipboardString(window::window);
    }

    /**
     * \brief Set clipboard content
     *
     * \param in New content
     */
    void set_clipboard(const char* in) {
        ::glfwSetClipboardString(window::window, in);
    }

    /**
     * \brief Connect a slot to the key signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_key(const key_signal::slot_type& slot) {
        return keyboard->connect(slot);
    }

    /**
     * \brief Connect a slot to the cursor entrance signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_enter(const enter_signal::slot_type& slot) {
        return enter->connect(slot);
    }

    /**
     * \brief Connect a slot to the mouse button signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_mouse(const mouse_signal::slot_type& slot) {
        return mouse->connect(slot);
    }

    /**
     * \brief Connect a slot to the scroll signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_scroll(const scroll_signal::slot_type& slot) {
        return scroll->connect(slot);
    }

    /**
     * \brief Connect a slot to the character signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_character(const character_signal::slot_type &slot) {
        return character->connect(slot);
    }

    /**
     * \brief Connect a slot to the unified input signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_unified(const unified_signal::slot_type &slot) {
        return unified->connect(slot);
    }

    /**
     * \brief Show the ImGui debug window
     *
     * \param open Pointer to window's open flag for the close widget
     */
    void debug_window(bool *open) {
        if (ImGui::Begin("Input", open)) {
            glm::dvec2 cur_pos = cursor_position();
            ImGui::Text("Cursor position: %.2f, %.2f", cur_pos.x, cur_pos.y);
            ImGui::Text("Number of key slots: %d", static_cast<int>(keyboard->num_slots()));
            ImGui::Text("Number of enter slots: %d", static_cast<int>(enter->num_slots()));
            ImGui::Text("Number of mouse slots: %d", static_cast<int>(mouse->num_slots()));
            ImGui::Text("Number of scroll slots: %d", static_cast<int>(scroll->num_slots()));
            ImGui::Text("Number of character slots: %d", static_cast<int>(character->num_slots()));
            ImGui::Text("Number of unified input slots: %d", static_cast<int>(unified->num_slots()));
            ImGui::Spacing();

            ImGui::Text("Cursor delta: %.2f, %.2f", cursor_d.x, cursor_d.y);
            ImGui::Spacing();

            ImGui::Text("ImGui wants mouse: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");
            ImGui::Text("ImGui wants keyboard: %s", ImGui::GetIO().WantCaptureKeyboard ? "true" : "false");
        }
        ImGui::End();
    }
}
