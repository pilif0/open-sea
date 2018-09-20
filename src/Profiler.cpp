/** \file Profiler.cpp
 * Profiler implementation
 *
 * \author Filip Smola
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
     *
     * Construct code block information from a label and assign the execution start time.
     *
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
     *
     * Start profiling by constructing a new frame track.
     */
    void start() {
        // Clear buffer and push root
        in_progress = std::make_shared<track>();
        in_progress->push(Info("Root"));
    }

    /**
     * \brief Finish profiling
     *
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

    //! Whether text should show maximum instead of last
    bool text_show_maximum = false;

    /**
     * \brief Show the text gui for the profiler
     */
    void show_text() {
        // Subject selection checkbox
        ImGui::Checkbox("Show Maximum", &text_show_maximum);

        // Selected track text
        std::shared_ptr<track> subject = text_show_maximum ? maximum : completed;
        ImGui::TextUnformatted(subject ?
                               subject->toIndentedString().data() :
                               "No completed frame track");
    }

    // Graphical gui parameters
    //! Height of each bar
    float bar_height = 20.0f;
    //! Height of each row
    float row_height = bar_height + 3.0f;
    //! Horizontal bar spacing
    float bar_spacing = 1.0f;
    //! Least width of bar to display
    float least_width = 10.0f;
    //! Bar colour
    ImVec4 col_bar(0.4f, 0.8f, 1.0f, 1.0f);
    //! Text colour
    ImVec4 col_text(0.0f, 0.0f, 0.0f, 1.0f);
    //! Text padding from the top-left
    ImVec2 text_pad{3.0f, 3.0f};

    //! Add two ImGui 2D vectors
    ImVec2 add(ImVec2 a, ImVec2 b) { return ImVec2{a.x + b.x, a.y + b.y}; }

    /**
     * \brief Recursive helper for show_graphical that draws the children of a node
     *
     * \param data Tree store of the frame track
     * \param draw_list Window draw list
     * \param canvas_pos Top-left canvas corner position
     * \param canvas_size Size of the canvas
     * \param root_time Duration of the root node
     * \param depth Number of rows above the bars being displayed
     * \param x_offset Offset from the left
     * \param node Node whose children to display
     */
    void draw_rec(const std::shared_ptr<std::vector<track::Node>> &data, ImDrawList* draw_list, ImVec2 canvas_pos,
                  ImVec2 canvas_size, double root_time, int depth, float x_offset, int node) {
        // Loop over all children of the node
        int child = (*data)[node].firstChild;
        while (child != track::Node::INVALID) {
            // Get content
            track::value_type content = (*data)[child].content;

            // Compute width
            auto width = static_cast<float>(canvas_size.x * content.time / root_time);

            // Only draw if sufficient width
            if (width >= least_width) {
                // Compute corners
                ImVec2 top_left{
                        canvas_pos.x + x_offset + bar_spacing,
                        canvas_pos.y + (depth * row_height)};
                ImVec2 bot_right{
                        canvas_pos.x + x_offset + width - bar_spacing,
                        canvas_pos.y + bar_height + (depth * row_height)};

                // Draw the bar
                draw_list->AddRectFilled(top_left, bot_right, ImGui::ColorConvertFloat4ToU32(col_bar));

                // Draw the tag
                draw_list->AddText(add(top_left, text_pad), ImGui::ColorConvertFloat4ToU32(col_text), content.label.data());

                // Recursively draw its children
                draw_rec(data, draw_list, canvas_pos, canvas_size, root_time, depth + 1, x_offset, child);
            }

            // Increment anchors
            child = (*data)[child].next;
            x_offset += width;
        }
    }

    //! Whether graphical should show maximum instead of last
    bool graphical_show_maximum = false;

    /**
     * \brief Show the graphical gui for the profiler
     */
    void show_graphical() {
        // Subject selection checkbox
        ImGui::Checkbox("Show Maximum", &graphical_show_maximum);
        std::shared_ptr<track> subject = (graphical_show_maximum) ? maximum : completed;

        // Parameter control
        if (ImGui::CollapsingHeader("Parameters")) {
            ImGui::InputFloat("bar height", &bar_height);
            ImGui::InputFloat("row height", &row_height);
            ImGui::InputFloat("horizontal spacing", &bar_spacing);
            ImGui::InputFloat("least width", &least_width);
            ImGui::ColorEdit4("bar colour", &col_bar.x);
            ImGui::ColorEdit4("text colour", &col_text.x);
            ImGui::InputFloat2("text padding", &text_pad.x);
        }

        if (!subject) {
            ImGui::TextUnformatted("No completed frame track");
        } else {
            // Retrieve the data
            std::shared_ptr<std::vector<track::Node>> data = subject->getStore();

            // Set up regions
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();

            // Draw root bar
            draw_list->AddRectFilled(
                    canvas_pos,
                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + bar_height),
                    ImGui::ColorConvertFloat4ToU32(col_bar));

            // Get root content
            track::value_type root_content = (*data)[0].content;

            // Draw root tag with time
            std::ostringstream stream;
            stream << root_content.label << " - " << root_content.time * 1000 << " ms";
            std::string tag = stream.str();
            draw_list->AddText(add(canvas_pos, text_pad), ImGui::ColorConvertFloat4ToU32(col_text), tag.data());

            // Have each node recursively draw its children
            draw_rec(data, draw_list, canvas_pos, canvas_size, root_content.time, 1, 0.0f, 0);
        }
    }
}
