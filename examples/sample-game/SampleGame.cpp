/*
 * Main point of the sample game example.
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 *
 * Based on GLFW example from http://www.glfw.org/documentation.html
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <open-sea/Test.h>
#include <open-sea/Log.h>

#include <sstream>

namespace log = open_sea::log;

// GLFW error callback that prints to console
void error_callback(int error, const char* description) {
    static log::severity_logger lg;
    std::ostringstream stringStream;
    stringStream << "GLFW error " << error << ": " << description;
    log::log(lg, log::error, stringStream.str());
}

int main() {
    // Initialize logging
    log::init_logging();
    log::severity_logger lg;

    GLFWwindow* window;

    // Set the callback
    glfwSetErrorCallback(error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        log::log(lg, log::fatal, "GLFW initialisation failed");
        return -1;
    }
    log::log(lg, log::info, "GLFW initialised");

    // Ask for OpenGL 3.3 forward compatible context
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, get_test_string().c_str(), NULL, NULL);
    if (!window) {
        log::log(lg, log::fatal, "Window creation failed");

        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        log::log(lg, log::fatal, "Failed to initialize OpenGL context");
        return -1;
    }
    log::log(lg, log::info, "Window and context created");

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }
    log::log(lg, log::info, "Main loop ended");

    glfwTerminate();
    log::clean_up();
    return 0;
}
