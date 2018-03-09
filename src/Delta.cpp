/**
 * Delta time implementation
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <open-sea/Delta.h>
#include <open-sea/Log.h>
namespace log = open_sea::log;

#include <boost/circular_buffer.hpp>

namespace open_sea::time {
    log::severity_logger lg = log::get_logger("Time");

    //! Current delta time in seconds
    double delta_time;
    //! Time of last update
    double last_update;

    // Average FPS counters
    //! Time elapsed since last average FPS update
    double fps_elapsed;
    //! Number of frames since last average FPS update
    int frames;
    //! Last finished average FPS value
    double average_FPS;

    // Debug data
    constexpr int history_length = 1000;
    boost::circular_buffer<float> history(history_length);

    /**
     * \brief Start delta time tracking
     * Reset all the counters.
     * Has to be called after GLFW initialization and should be immediately before the main loop starts.
     */
    // TODO: Make sure starting delta time at 0 doesn't break anything (division by 0?)
    void start_delta() {
        delta_time = 0;
        last_update = ::glfwGetTime();
        fps_elapsed = 0;
        frames = 0;
        average_FPS = 0;
        history.clear();

        log::log(lg, log::info, "Started delta time tracking");
    }

    /**
     * \brief Update the delta time tracking
     * Update all the counters and compute values as required.
     * Should be done at the end of each frame.
     */
    void update_delta() {
        // Update counters
        double buffer = last_update;
        last_update = ::glfwGetTime();
        delta_time = last_update - buffer;
        fps_elapsed += delta_time;
        frames++;

        // Compute average FPS
        if (fps_elapsed >= 1) {
            average_FPS = frames / fps_elapsed;

            // Reset the counters
            fps_elapsed = 0;
            frames = 0;

            /* Alternative:
            average_FPS = frames;

            // Reset the counters
            fps_elapsed -= 1;
            frames = 0;*/
        }

        // Update the history
        history.push_back((float) delta_time);
    }

    /**
     * \brief Get value of delta time
     * Get value of delta time as a fraction of a second.
     *
     * \return Delta time
     */
    double get_delta() {
        return delta_time;
    }

    /**
     * \brief Get the immediate FPS
     * Get the immediate FPS, i.e. the multiplicative inverse of delta time.
     *
     * \return Immediate FPS
     */
    double get_FPS_immediate() {
        return 1 / delta_time;
    }

    /**
     * \brief Get the average FPS
     * Get the average FPS, recalculated at most every second.
     *
     * \return Average FPS
     */
    double get_FPS_average() {
        return average_FPS;
    }

    /**
     * \brief Get the delta time for \c i far in history
     * Get the element of \c history at index \c i. Used by plotting in the debug widget.
     *
     * \param data User data
     * \param i Index
     * \return Delta time \c i far in history
     */
    float debug_history_get(void* data, int i) {
        return history[i];
    }

    /**
     * \brief Show the ImGui debug widget
     */
    void debug_widget() {
        // Plot the delta time
        ImGui::PlotLines("##plot", debug_history_get, &history, history_length);
        ImGui::SameLine();
        ImGui::Text("Delta time\n(%.3f ms)", get_delta() * 1000);

        // Plot the FPS
        ImGui::Text("FPS: %.1f (%.1f avg)", get_FPS_immediate(), get_FPS_average());
    }
}