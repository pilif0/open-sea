/**
 * Profiler implementation
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <open-sea/Profiler.h>

namespace open_sea::profiler {

    //--- start Node implementation
//    Node::Node(int parent, int previous, const std::string &label)
//        : parent(parent), previous(previous), label(label) {
//        next = 0;
//        child = 0;
//        time = glfwGetTime();
//    }
    Node::Node(int parent, const std::string &label)
        : parent(parent), label(label) {
        time = glfwGetTime();
        child = 0;
    }
    //--- end Node implementation

    //! Frame tree being built
    std::vector<Node> in_progress;
    int current = -1;

    /**
     * \brief Start profiling
     * Start profiling by clearing the frame tree buffer and adding the root node.
     */
    void start() {
        // Clear buffer and add root
        in_progress.clear();
        current = 0;
        in_progress.emplace_back({});
    }

    /**
     * \brief Finish profiling
     * Finish profiling by computing root node and copying from the frame tree buffer.
     */
    void finish() {
        // Skip if not started
        if (current < 0) return;

        // Compute root duration
        in_progress[0].time = glfwGetTime() - in_progress[0].time;

        // Copy  buffer
        completed = in_progress;
        current = -1;
    }

    /**
     * \brief Push a block of code on the profiling stack
     *
     * \param label Label
     */
    void push(const std::string &label) {
        // Skip if not started
        if (current < 0) return;

        // Add a child to current node
        in_progress.emplace_back(current, label);
    }

    /**
     * \brief Pop a block of code from the profiling stack
     */
    void pop() {
        // Skip if not started
        if (current < 0) return;

        // Compute current duration
        in_progress[current].time = glfwGetTime() - in_progress[current].time;

        // Move current
        current = in_progress[current].parent;
    }
}
