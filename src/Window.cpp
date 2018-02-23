/*
 * Window implementation
 */
#include <open-sea/Window.h>
#include <open-sea/Log.h>
namespace log = open_sea::log;
#include <open-sea/config.h>

#include <sstream>

namespace open_sea::window {
    // Logger for this module
    log::severity_logger lg = log::get_logger("Window");

    //! Current window properties (default values until a window is created)
    std::unique_ptr<window_properties> current = std::make_unique<window_properties>();

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
        std::ostringstream stringStream;
        stringStream << "GLFW error " << error << ": " << description;
        log::log(lg, log::error, stringStream.str());
    }

    /**
     * \brief Initialize GLFW
     * Initialize GLFW and log the action
     *
     * \return \c false on failure, \c true otherwise
     */
    bool init_glfw() {
        // Set the error callback
        ::glfwSetErrorCallback(error_callback);

        // Initialize GLFW
        if (!::glfwInit()) {
            log::log(lg, log::fatal, "GLFW initialisation failed");
            return false;
        }
        log::log(lg, log::info, "GLFW initialised");
        return true;
    }

    /**
     * \brief Prepare the title in the current window properties for printing
     * Append engine information to the title in the current window propeties.
     *
     * \return Processed title
     */
    std::string process_title() {
        std::ostringstream processedTitle;
        processedTitle << current->title                                    // Write the title
                       << " (Open Sea v" << OPEN_SEA_VERSION_FULL << ")";   // Append engine information
        return processedTitle.str();
    }

    /**
     * \brief Set the window title
     * Set the window title and log the action
     *
     * \param title New value
     */
    void set_title(const std::string &title) {
        // Write the new title
        current->title = title;

        // Modify the window
        if (window)
            ::glfwSetWindowTitle(window, process_title().c_str());

        // Log the action
        log::log(lg, log::info, (std::string("Title set to: ")).append(title));
    }

    /**
     * \brief Set size of the windowed window
     * Set size of the window if in windowed or fullscreen state.
     * Do nothing if the window is in borderless state (the size there is governed by the monitor), the window doesn't
     * exist, or for non-positive parameters.
     *
     * \param width New width
     * \param height New height
     */
    void set_size(int width, int height) {
        // Skip if either parameter is non-positive
        if (width < 1 || height < 1)
            return;

        // Skip if the window doesn't exist (size is always set on creation)
        if (!window)
            return;

        // Skip when not windowed
        if (current->state == borderless)
            return;

        // Skip when no adjustment necessary
        if (current->width == width && current->height == height)
            return;

        // Modify the window
        ::glfwSetWindowSize(window, width, height);

        // Write the new sizes
        ::glfwGetWindowSize(window, &current->width, &current->height);
        ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);

        // Update the viewport size
        ::glViewport(0, 0, current->fbWidth, current->fbHeight);

        // Log the action
        std::ostringstream message;
        message << "Size set to [" << width << "," << height << "]";
        log::log(lg, log::info, message.str());
    }

    /**
     * \brief Enable vSync
     * Enable vertical synchronisation (swap interval of 1).
     * Once enabled, cannot be disabled.
     */
    void enableVSync() {
        // Skip if already enabled
        if (current->vSync)
            return;

        // Enable vSync
        ::glfwSwapInterval(1);

        // Write the property
        current->vSync = true;
    }

    /**
     * \brief Center the window
     * Center the window on the primary monitor.
     * Do nothing for non-windowed windows.
     */
    void center() {
        // Skip if not windowed
        if (current->state != windowed)
            return;

        // Retrieve primary monitor position and size
        int mPosX, mPosY;
        ::glfwGetMonitorPos(::glfwGetPrimaryMonitor(), &mPosX, &mPosY);
        const ::GLFWvidmode* videomode = ::glfwGetVideoMode(::glfwGetPrimaryMonitor());
        int mWidth = videomode->width;
        int mHeight = videomode->height;

        // Calculate new window position
        int x = mPosX + (mWidth / 2) - (current->width / 2);
        int y = mPosY + (mHeight / 2) - (current->height / 2);

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
     * \breif Hide the window
     */
    void hide() {
        ::glfwHideWindow(window);
    }

    /**
     * \brief Close the window
     * Instruct the window to close. Means \c should_close() will return \c true.
     */
    void close() {
        ::glfwSetWindowShouldClose(window, 1);
    }

    /**
     * \brief Update the window
     * Swap buffers and poll for events
     */
    void update() {
        // Swap front and back buffers
        glfwSwapBuffers(window::window);

        // Poll for and process events
        glfwPollEvents();
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

    /**
     * \brief Set window size callback
     * Set a GLFW callback function for changes in window size
     *
     * \param cbfun Callback function
     */
    void set_size_callback(::GLFWwindowsizefun cbfun) {
        ::glfwSetWindowSizeCallback(window, cbfun);
    }

    /**
     * \brief Set window focus callback
     * Set a GLFW callback function for changes in window focus
     *
     * \param cbfun Callback function
     */
    void set_focus_callback(::GLFWwindowfocusfun cbfun) {
        ::glfwSetWindowFocusCallback(window, cbfun);
    }

    /**
     * \brief Set window close callback
     * Set a GLFW callback function for changes in window close flag
     *
     * \param cbfun Callback function
     */
    void set_close_callback(::GLFWwindowclosefun cbfun) {
        ::glfwSetWindowCloseCallback(window, cbfun);
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

        return true;
    }

    /**
     * \brief Make the window windowed of the given size
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

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
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
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
            current->state = windowed;
            current->monitor = nullptr;

            // Log the action
            log::log(lg, log::info, "Modified the window to windowed state");

            return true;
        }
    }

    /**
     * \brief Make the window borderless on the given monitor
     * If the window exists, make it borderless on the given monitor.
     * If it doesn't exist, create one on the given monitor and other properties from the current window properties.
     *
     * Writes the resulting properties into the current properties on success.
     *
     * \param monitor Monitor to use
     * \return \c false on failure, \c true otherwise
     */
    bool make_borderless(::GLFWmonitor* monitor) {
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

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
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
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
            current->state = borderless;
            current->monitor = monitor;

            // log the action
            log::log(lg, log::info, "Modified the window to borderless state");

            return true;
        }
    }

    /**
     * \brief Make the window fullscreen of the given size on the given monitor
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

            // Write new properties
            ::glfwGetWindowSize(window, &current->width, &current->height);
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
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
            ::glfwGetFramebufferSize(window, &current->fbWidth, &current->fbHeight);
            current->state = fullscreen;
            current->monitor = monitor;

            // log the action
            log::log(lg, log::info, "Modified the window to fullscreen state");

            return true;
        }
    }
}