/** \file Window.cpp
 * Window implementation
 *
 * \author Filip Smola
 */
#include <open-sea/Window.h>
#include <open-sea/config.h>
#include <open-sea/Log.h>
#include <open-sea/ImGui.h>

#include <glm/glm.hpp>

#include <sstream>

namespace open_sea::window {
    //! Module logger
    log::severity_logger lg = log::get_logger("Window");

    //! Size signal
    std::unique_ptr<size_signal_t> size_signal;
    //! Focus signal
    std::unique_ptr<focus_signal_t> focus_signal;
    //! Close signal
    std::unique_ptr<close_signal_t> close_signal;

    //! Current window properties (default values until a window is created)
    std::unique_ptr<WindowProperties> current = std::make_unique<WindowProperties>();

    //! Game window
    ::GLFWwindow* window = nullptr;

    /**
     * \brief Log GLFW error
     *
     * \param error Error number
     * \param description Description
     */
    void error_callback(int error, const char* description) {
        static log::severity_logger lg = log::get_logger("GLFW");
        std::ostringstream string_stream;
        string_stream << "GLFW error " << error << ": " << description;
        log::log(lg, log::error, string_stream.str());
    }

    /**
     * \brief Initialize GLFW
     *
     * Initialize GLFW and signals, and log the actions
     *
     * \return \c false on failure, \c true otherwise
     */
    bool init() {
        log::log(lg, log::info, "Initializing Window module...");

        // Set the error callback
        ::glfwSetErrorCallback(error_callback);

        // Initialize GLFW
        if (!::glfwInit()) {
            log::log(lg, log::fatal, "GLFW initialization failed");
            return false;
        }
        log::log(lg, log::info, "GLFW initialized");

        // Instantiate signals
        size_signal = std::make_unique<size_signal_t>();
        focus_signal = std::make_unique<focus_signal_t>();
        close_signal = std::make_unique<close_signal_t>();

        // Add a slot to update viewport dimensions on each size change
        size_signal->connect([](float w, float h){ ::glViewport(0, 0, current->fb_width, current->fb_height); });

        log::log(lg, log::info, "Window module initialized");
        return true;
    }

    /**
     * \brief Prepare the title in the current window properties for printing
     *
     * Append engine information to the title in the current window propeties.
     *
     * \return Processed title
     */
    std::string process_title() {
        std::ostringstream processed_title;
        processed_title << current->title                                    // Write the title
                       << " (Open Sea v" << open_sea::version_full << ")";   // Append engine information
        return processed_title.str();
    }

    /**
     * \brief Set the window title
     *
     * Set the window title and log the action
     *
     * \param title New value
     */
    void set_title(const std::string &title) {
        // Write the new title
        current->title = title;

        // Modify the window
        if (window) {
            ::glfwSetWindowTitle(window, process_title().c_str());
        }

        // Log the action
        log::log(lg, log::info, (std::string("Title set to: ")).append(title));
    }

    /**
     * \brief Set size of the windowed window
     *
     * Set size of the window if in windowed or fullscreen state.
     * Do nothing if the window is in borderless state (the size there is governed by the monitor), the window doesn't
     * exist, or for non-positive parameters.
     *
     * \param width New width
     * \param height New height
     */
    void set_size(int width, int height) {
        // Skip if either parameter is non-positive
        if (width < 1 || height < 1) {
            return;
        }

        // Skip if the window doesn't exist (size is always set on creation)
        if (!window) {
            return;
        }

        // Skip when not windowed
        if (current->state == borderless) {
            return;
        }

        // Skip when no adjustment necessary
        if (current->width == width && current->height == height) {
            return;
        }

        // Modify the window
        ::glfwSetWindowSize(window, width, height);

        // Write the new sizes
        ::glfwGetWindowSize(window, &current->width, &current->height);
        ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);

        // Log the action
        std::ostringstream message;
        message << "Size set to [" << width << "," << height << "]";
        log::log(lg, log::info, message.str());
    }

    /**
     * \brief Enable vSync
     *
     * Enable vertical synchronisation (swap interval of 1).
     * Once enabled, cannot be disabled.
     */
    void enable_v_sync() {
        // Skip if already enabled
        if (current->v_sync) {
            return;
        }

        // Enable vSync
        ::glfwSwapInterval(1);

        // Write the property
        current->v_sync = true;
    }

    /**
     * \brief Center the window
     *
     * Center the window on the primary monitor.
     * Do nothing for non-windowed windows.
     */
    void center() {
        // Skip if not windowed
        if (current->state != windowed) {
            return;
        }

        // Retrieve primary monitor position and size
        int m_pos_x, m_pos_y;
        ::glfwGetMonitorPos(::glfwGetPrimaryMonitor(), &m_pos_x, &m_pos_y);
        const ::GLFWvidmode* videomode = ::glfwGetVideoMode(::glfwGetPrimaryMonitor());
        int m_width = videomode->width;
        int m_height = videomode->height;

        // Calculate new window position
        int x = m_pos_x + (m_width / 2) - (current->width / 2);
        int y = m_pos_y + (m_height / 2) - (current->height / 2);

        // Set the window position
        ::glfwSetWindowPos(window, x, y);
    }

    /**
     * \brief Show the window
     */
    void show() {
        ::glfwShowWindow(window);
    }

    /**
     * \brief Hide the window
     */
    void hide() {
        ::glfwHideWindow(window);
    }

    /**
     * \brief Close the window
     *
     * Instruct the window to close. Means \c should_close() will return \c true.
     */
    void close() {
        ::glfwSetWindowShouldClose(window, 1);
    }

    /**
     * \brief Get properties of the current window
     *
     * Get a copy of properties of the current window
     *
     * \return Current properties
     */
    WindowProperties current_properties() {
        return WindowProperties(*current);
    }

    //! Whether the window is focused
    bool focus_flag = false;

    /**
     * \brief Get whether the window is focused
     *
     * \return Whether the window is focused
     */
    bool is_focused() {
        return focus_flag;
    }

    /**
     * \brief Update the window
     *
     * Swap buffers and poll for events
     */
    void update() {
        // Swap front and back buffers
        ::glfwSwapBuffers(window::window);

        // Poll for and process events
        ::glfwPollEvents();
    }

    /**
     * \brief Return whether the window should close
     *
     * \return Whether the window should close
     */
    bool should_close() {
        return ::glfwWindowShouldClose(window) != 0;
    }

    /**
     * \brief Clean up the window
     *
     * Destroy the window and reset the pointer to \c nullptr
     */
    void clean_up() {
        ::glfwDestroyWindow(window);
        window = nullptr;
    }

    /**
     * \brief Terminate GLFW
     */
    void terminate() {
        ::glfwTerminate();
    }

    // Callbacks
    /**
     * \brief Send signal about a size event
     *
     * \param w Event window
     * \param width New width
     * \param height New height
     */
    void size_callback(::GLFWwindow* w, int width, int height) {
        // Skip if not the global window
        if (w != window) {
            return;
        }

        // Update properties
        ::glfwGetWindowSize(window, &current->width, &current->height);
        ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);

        // Fire the signal
        (*size_signal)(width, height);
    }

    /**
     * \brief Send signal about focus event
     *
     * \param w Event window
     * \param focused \c 0 if not focused, otherwise focused
     */
    void focus_callback(::GLFWwindow* w, int focused) {
        // Skip if not the global window
        if (w != window) {
            return;
        }

        // Update the flag
        focus_flag = (focused != 0);

        // Fire the signal
        (*focus_signal)(focused != 0);
    }

    /**
     * \brief Send signal about close event
     *
     * \param w Event window
     */
    void close_callback(::GLFWwindow* w) {
        // Skip if not the global window
        if (w != window) {
            return;
        }

        // Fire the signal
        (*close_signal)();
    }

    /**
     * \brief Attach callbacks to the window
     *
     * Attach callbacks that fire appropriate signals to the window
     */
    void attach_callbacks() {
        if (window) {
            log::log(lg, log::info, "Attaching callbacks...");

            ::glfwSetWindowSizeCallback(window, size_callback);
            ::glfwSetWindowFocusCallback(window, focus_callback);
            ::glfwSetWindowCloseCallback(window, close_callback);

            log::log(lg, log::info, "Attached callbacks");
        }
    }

    // Connectors
    /**
     * \brief Connect a slot to the size signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_size(const size_signal_t::slot_type& slot) {
        log::log(lg, log::info, "Connecting slot to size signal");
        return size_signal->connect(slot);
    }

    /**
     * \brief Connect a slot to the focus signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_focus(const focus_signal_t::slot_type& slot) {
        log::log(lg, log::info, "Connecting slot to focus signal");
        return focus_signal->connect(slot);
    }

    /**
     * \brief Connect a slot to the close signal
     *
     * \param slot Slot to connect
     * \return Connection
     */
    connection connect_close(const close_signal_t::slot_type& slot) {
        log::log(lg, log::info, "Connecting slot to close signal");
        return close_signal->connect(slot);
    }

    /**
     * \brief Set the GLFW window creation hints
     */
    void set_hints() {
        // Reset hints
        glfwDefaultWindowHints();

        // Context
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        // Window
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }

    /**
     * \brief Initialize window's OpenGL context after it has been created
     */
    bool init_context() {
        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            log::log(lg, log::fatal, "Failed to initialize OpenGL context");
            return false;
        }
        log::log(lg, log::info, "OpenGL context initialized");

        // Initialize with vSync set as in current properties
        if (current->v_sync) {
            ::glfwSwapInterval(1);
        } else {
            ::glfwSwapInterval(0);
        }

        return true;
    }

    /**
     * \brief Make the window windowed of the given size
     *
     * If the window exists, transform it into the right size and make it windowed.
     * If it doesn't exist, create one with the given size and other properties from the current window properties.
     *
     * Writes the resulting properties into the current properties on success.
     *
     * \param width Width to use
     * \param height Height to use
     * \return \c false on failure, \c true otherwise
     */
    bool make_windowed(const int width, const int height) {
        log::log(lg, log::info, "Making windowed window...");

        // Create the window if one doesn't exist
        if (!window) {
            // Create the window
            set_hints();
            window = glfwCreateWindow(width, height, process_title().c_str(), nullptr, nullptr);

            // Handle creation errors
            if (!window) {
                log::log(lg, log::fatal, "Window creation failed");
                return false;
            }

            // Initialize context
            init_context();

            // Attach callbacks
            attach_callbacks();

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = windowed;
            current->monitor = nullptr;

            // Show the window
            show();

            // Log the action
            log::log(lg, log::info, "Created new window in windowed state");

            return true;
        } else {    // Otherwise modify existing window
            // Only resize if already windowed
            if (current->state == windowed) {
                // Log the decision
                log::log(lg, log::info, "Window already in windowed state, only changing size");

                set_size(width, height);
                return true;
            }

            // Modify the window
            ::glfwSetWindowMonitor(window, nullptr, 0, 0, width, height, GLFW_DONT_CARE);

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = windowed;
            current->monitor = nullptr;

            // Log the action
            log::log(lg, log::info, "Modified the window to windowed state");

            return true;
        }
    }

    /**
     * \brief Make the window borderless on the given monitor
     *
     * If the window exists, make it borderless on the given monitor.
     * If it doesn't exist, create one on the given monitor and other properties from the current window properties.
     *
     * Writes the resulting properties into the current properties on success.
     *
     * \param monitor Monitor to use
     * \return \c false on failure, \c true otherwise
     */
    bool make_borderless(::GLFWmonitor* monitor) {
        log::log(lg, log::info, "Making borderless window...");

        // Create the window if one doesn't exist
        if (!window) {
            // Get the video mode
            const ::GLFWvidmode* mode = ::glfwGetVideoMode(monitor);

            // Create the window
            set_hints();
            ::glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            ::glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            ::glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            ::glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            window = ::glfwCreateWindow(mode->width, mode->height, process_title().c_str(), monitor, nullptr);

            // Handle creation errors
            if (!window) {
                log::log(lg, log::fatal, "Window creation failed");
                return false;
            }

            // Initialize context
            init_context();

            // Attach callbacks
            attach_callbacks();

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = borderless;
            current->monitor = monitor;

            // Show the window
            show();

            // Log the action
            log::log(lg, log::info, "Created new window in borderless state");

            return true;
        } else {    // Otherwise modify existing window
            // Do nothing if borderless on the same monitor
            if (current->state == borderless && current->monitor == monitor) {
                // Log the decision
                log::log(lg, log::info, "Window already in borderless state on the same monitor");

                return true;
            }

            // Get the video mode
            const ::GLFWvidmode* mode = ::glfwGetVideoMode(monitor);

            // Modify the window
            ::glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = borderless;
            current->monitor = monitor;

            // log the action
            log::log(lg, log::info, "Modified the window to borderless state");

            return true;
        }
    }

    /**
     * \brief Make the window fullscreen of the given size on the given monitor
     *
     * If the window exists, make it fullscreen of the given size on the given monitor.
     * If it doesn't exist, create one of the fiven size on the given monitor and other properties from the current
     * window properties.
     *
     * Writes the resulting properties into the current properties on success.
     *
     * \param width Width to use
     * \param height Height to use
     * \param monitor Monitor to use
     * \return \c false on failure, \c true otherwise
     */
    bool make_fullscreen(int width, int height, ::GLFWmonitor* monitor) {
        log::log(lg, log::info, "Making fullscreen window...");

        // Create the window if one doesn't exist
        if (!window) {
            // Create the window
            set_hints();
            window = ::glfwCreateWindow(width, height, process_title().c_str(), monitor, nullptr);

            // Handle creation errors
            if (!window) {
                log::log(lg, log::fatal, "Window creation failed");
                return false;
            }

            // Initialize context
            init_context();

            // Attach callbacks
            attach_callbacks();

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = fullscreen;
            current->monitor = monitor;

            // Show the window
            show();

            // Log the action
            log::log(lg, log::info, "Created new window in fullscreen state");

            return true;
        } else {    // Otherwise modify existing window
            // If fullscreen on the same monitor, only change size if needed
            if (current->state == fullscreen) {
                // Log the decision
                log::log(lg, log::info, "Window already in fullscreen state on the same monitor, only adjusting size");

                set_size(width, height);
                return true;
            }

            // Modify the window
            ::glfwSetWindowMonitor(window, monitor, 0, 0, width, height, GLFW_DONT_CARE);

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fb_width, &current->fb_height);
            current->state = fullscreen;
            current->monitor = monitor;

            // Log the action
            log::log(lg, log::info, "Modified the window to fullscreen state");

            return true;
        }
    }

    // Temporary storage for modify dialog
    //! Title buffer
    char modify_title[64];  //TODO: is 64 enough?
    //! Size buffer
    glm::ivec2 modify_size;
    //! Window state buffer
    window_state modify_state;
    //! Monitor pointer buffer
    GLFWmonitor* modify_monitor;
    //! Window state selector buffer
    int modify_state_no;
    //! Window state selector labels
    constexpr const char *modify_state_values[3] {"windowed", "borderless", "fullscreen"};
    //! Monitor ID buffer
    int modify_monitor_no;

    /**
     * \brief Reset temporary storage values for modify dialog
     */
    void modify_reset_temp() {
        current->title.copy(modify_title, 64);
        modify_size = {current->width, current->height};
        modify_state = current->state;
        modify_monitor = current->monitor;
        switch (current->state) {
            case windowed: modify_state_no = 0; break;
            case borderless: modify_state_no = 1; break;
            case fullscreen: modify_state_no = 2; break;
        }
        int count;
        GLFWmonitor* *m = glfwGetMonitors(&count);
        for (int i = 0; i < count; i++, m++) {
            if (*m == current->monitor) {
                modify_monitor_no = i;
                break;
            }
        }
    }

    /**
     * \brief Show the ImGui debug window
     *
     * \param open Pointer to window's open flag for the close widget
     */
    void debug_window(bool *open) {
        if (ImGui::Begin("Window", open)) {
            ImGui::Text("Window size: %d x %d", current->width, current->height);
            ImGui::Text("Frame buffer size: %d x %d", current->fb_width, current->fb_height);
            ImGui::Text("Window title: %s", current->title.c_str());
            ImGui::Text("Processed title: %s", process_title().c_str());
            const char *state;
            switch (current->state) {
                case windowed:
                    state = "windowed";
                    break;
                case fullscreen:
                    state = "fullscreen";
                    break;
                case borderless:
                    state = "borderless";
                    break;
                default:
                    state = "invalid";
            }
            ImGui::Text("Window state: %s", state);
            ImGui::Text("Window monitor: %s",
                        (current->monitor == nullptr) ? "none" : ::glfwGetMonitorName(current->monitor));
            ImGui::Text("Vsync: %s", current->v_sync ? "enabled" : "disabled");
            if (ImGui::Button("Modify")) {
                modify_reset_temp();

                ImGui::OpenPopup("Modify Window");
            }
            if (ImGui::BeginPopupModal("Modify Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                show_modify();

                if (ImGui::Button("Close")) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    /**
     * \brief Show the window modify popup contents
     */
    void show_modify() {
        ImGui::InputText("title", modify_title, 64);
        ImGui::InputInt2("size", &modify_size[0]);
        ImGui::ListBox("state", &modify_state_no, modify_state_values, 3);

        if (modify_state_no != 0) {
            ImGui::InputInt("monitor", &modify_monitor_no);
        }


        if (ImGui::Button("Apply")) {
            set_title(modify_title);
            int count;
            GLFWmonitor* *monitors = glfwGetMonitors(&count);

            if (modify_monitor_no >= count || (modify_monitor = monitors[modify_monitor_no]) == nullptr) {
                std::ostringstream message;
                message << "Monitor " << modify_monitor_no << " not available when modifying the window, using primary";
                log::log(lg, log::error, message.str());
                modify_monitor = glfwGetPrimaryMonitor();
                modify_monitor_no = 0;
            }

            switch (modify_state_no) {
                case 0: modify_state = windowed; break;
                case 1: modify_state = borderless; break;
                case 2: modify_state = fullscreen; break;
            }

            if (modify_state == windowed) {
                make_windowed(modify_size.x, modify_size.y);
            } else if (modify_state == borderless) {
                make_borderless(modify_monitor);
            } else {
                // modify_state == fullscreen
                make_fullscreen(modify_size.x, modify_size.y, modify_monitor);
            }

            modify_reset_temp();
        }
    }
}
