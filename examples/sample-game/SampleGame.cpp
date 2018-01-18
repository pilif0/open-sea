/*
 * Main point of the sample game example.
 *
 * Adapted from http://www.glfw.org/documentation.html
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <open-sea/Test.h>

#include <iostream>

// GLFW error callback that prints to console
void error_callback(int error, const char* description) {
    std::cout << "GLFW error " << error << ": " << description << std::endl;
}

int main() {
    GLFWwindow* window;

    // Set the callback
    glfwSetErrorCallback(error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Initialisation failed" << std::endl;
        return -1;
    }

    // Ask for OpenGL 3.3 forward compatible context
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    std::cout << "Initialised\n";

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, get_test_string().c_str(), NULL, NULL);
    if (!window) {
        std::cout << "Window creation failed" << std::endl;

        glfwTerminate();
        return -1;
    }

    std::cout << "Window created\n";

    // Make the window's context current
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    std::cout << "Main loop ended" << std::endl;

    glfwTerminate();
    return 0;
}
