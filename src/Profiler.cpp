/**
 * Profiler implementation
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <open-sea/Profiler.h>

namespace open_sea::profiler {

    //--- start Node implementation
    Node::Node(int parent, int previous, const std::string &label)
        : parent(parent), previous(previous), label(label) {
        next = 0;
        child = 0;
        time = glfwGetTime();
    }
    //--- end Node implementation
}
