/*
 * Input implementation
 */
#include <imgui.h>

#include <open-sea/Input.h>
#include <open-sea/Log.h>
#include <open-sea/Window.h>
#include <open-sea/ImGui.h>
namespace w = open_sea::window;

#include <sstream>

namespace open_sea::input {
    // Logger for this module
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
        if (window != w::window)
            return;

        // Transform values
        state state = (action == GLFW_PRESS) ? press : (action == GLFW_REPEAT) ? repeat : release;

        // Fire a signal
        static bool imgui_waits_esc = false;        // Fixes #12: True when ImGui is waiting for ESC release signal
        static bool imgui_waits_ent = false;        // Fixes #14: True when ImGui is waiting for Enter release signal
        if (ImGui::GetIO().WantCaptureKeyboard) {
            // Let ImGui take all input when it wants it
            open_sea::imgui::key_callback(key, scancode, state, mods);

            if (key == GLFW_KEY_ESCAPE && state == press) imgui_waits_esc = true;
            if (key == GLFW_KEY_ENTER && state == press) imgui_waits_ent = true;
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
        if (window != w::window)
            return;

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
        if (window != w::window)
            return;

        // Transform values
        state state = (action == GLFW_PRESS) ? press : (action == GLFW_REPEAT) ? repeat : release;

        // Fire a signal
        if (ImGui::GetIO().WantCaptureMouse)
            open_sea::imgui::mouse_callback(button, state, mods);
        else
            (*mouse)(button, state, mods);
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
        if (window != w::window)
            return;

        // Fire a signal
        if (ImGui::GetIO().WantCaptureMouse)
            open_sea::imgui::scroll_callback(xoffset, yoffset);
        else
            (*scroll)(xoffset, yoffset);
    }

    /**
     * \brief Send signal about a character event
     *
     * \param window Event window
     * \param codepoint Unicode codepoint of the character
     */
    void character_callback(::GLFWwindow* window, unsigned int codepoint) {
        // Skip if not the global window
        if (window != w::window)
            return;

        // Fire a signal
        if (ImGui::GetIO().WantCaptureKeyboard)
            open_sea::imgui::char_callback(codepoint);
        else
            (*character)(codepoint);
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
     * \brief Initialize input
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

        // Attach the callbacks
        reattach();

        log::log(lg, log::info, "Input initialized");
    }

    /**
     * \brief Get cursor position
     * Get position of the cursor in screen coordinates relative to origin of the window (top left).
     *
     * \return Cursor position
     */
    ::glm::dvec2 cursor_position() {
        double x, y;
        ::glfwGetCursorPos(w::window, &x, &y);
        return glm::dvec2(x, y);
    }

    /**
     * \brief Get key state
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
     * Get the name of the supplied key.
     * If \c key is \c unknown_key then \scancode is used, otherwise it is ignored.
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
        if (cs == nullptr)
            return std::string("undefined");

        return std::string(cs);
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
     * \brief Show the ImGui debug window
     */
    void show_debug() {
        ImGui::Begin("Input");

        glm::dvec2 cur_pos = cursor_position();
        ImGui::Text("Cursor position: (%.2f,%.2f)", cur_pos.x, cur_pos.y);
        ImGui::Text("Number of key slots: %d", keyboard->num_slots());
        ImGui::Text("Number of enter slots: %d", enter->num_slots());
        ImGui::Text("Number of mouse slots: %d", mouse->num_slots());
        ImGui::Text("Number of scroll slots: %d", scroll->num_slots());
        ImGui::Text("Number of character slots: %d", character->num_slots());
        ImGui::Spacing();

        ImGui::Text("ImGui wants mouse: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");
        ImGui::Text("ImGui wants keyboard: %s", ImGui::GetIO().WantCaptureKeyboard ? "true" : "false");

        ImGui::End();
    }
}
