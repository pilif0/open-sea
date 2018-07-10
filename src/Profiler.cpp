/*
 * Profiler implementation
 */
#include <open-sea/Profiler.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <sstream>

namespace open_sea::profiler {

    //--- start Info implementation
    /**
     * \brief Construct information from a label
     * Construct code block information from a label and assign the execution start time.
     * \param label
     */
    Info::Info(const std::string &label)
        : label(label) {
        time = glfwGetTime();
    }
    //--- end Info implementation

    std::ostream& operator<<(std::ostream &os, const Info &info) {
        std::ostringstream stream;
        stream << info.label
               << " - "
               << info.time * 1e3
               << " ms";
        os << stream.str();
        return os;
    }

    //! Pointer to frame track being built
    // Empty when not started
    std::shared_ptr<track> in_progress{};
    //! Pointer to last completed frame track
    std::shared_ptr<track> completed{};
    //! Pointer to frame track with the maximum recorded root duration
    std::shared_ptr<track> maximum{};

    /**
     * \brief Start profiling
     * Start profiling by constructing a new frame track.
     */
    void start() {
        // Clear buffer and push root
        in_progress = std::make_shared<track>();
        in_progress->push(Info("Root"));
    }

    /**
     * \brief Finish profiling
     * Finish profiling by computing root node and copying from the frame tree buffer.
     */
    void finish() {
        // Skip if not started
        if (!in_progress) return;

        // Pop root
        pop();

        // Copy the frame track into completed and clear in_progress
        in_progress.swap(completed);
        in_progress.reset();

        // Update maximum if relevant
        if (!maximum || (*completed->getStore())[0].content.time > (*maximum->getStore())[0].content.time) {
            maximum = completed;
        }
    }

    /**
     * \brief Push a block of code onto the profiling stack
     *
     * \param label Label
     */
    void push(const std::string &label) {
        // Skip if not started
        if (!in_progress) return;

        // Push onto the track
        in_progress->push(Info(label));
    }

    /**
     * \brief Pop a block of code from the profiling stack
     */
    void pop() {
        // Skip if not started
        if (!in_progress) return;

        // Compute the duration and pop the track element
        in_progress->top().time = glfwGetTime() - in_progress->top().time;
        in_progress->pop();
    }

    /**
     * \brief Get the last completed frame tree
     *
     * \return Last completed frame tree
     */
    std::shared_ptr<track> get_last() { return completed; }

    /**
     * \brief Get the maximum recorded frame tree
     *
     * \return Maximum recorded frame tree
     */
    std::shared_ptr<track> get_maximum() { return maximum; }

    /**
     * \brief Clear the maximum recorded frame tree
     */
    void clear_maximum() { maximum.reset(); }

    //! Whether to show maximum instead of last
    bool show_maximum = false;

    /**
     * \brief Show the text gui for the profiler
     */
    void show_text() {
        // Subject selection checkbox
        ImGui::Checkbox("Show Maximum", &show_maximum);

        // Selected track text
        std::shared_ptr<track> subject = show_maximum ? maximum : completed;
        ImGui::TextUnformatted(subject ?
                               subject->toIndentedString().c_str() :
                               "No completed frame track");
    }
}
