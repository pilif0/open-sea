/**
 * Profiler implementation
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include <open-sea/Profiler.h>

#include <sstream>

namespace open_sea::profiler {

    //--- start Node implementation
    Node::Node(int parent, const std::string &label)
        : parent(parent), label(label) {
        time = glfwGetTime();
        child = 0;
        next = 0;
    }
    //--- end Node implementation

    //! Frame tree being built
    std::vector<Node> in_progress;
    //! Last completed frame tree
    std::vector<Node> completed;
    //! Current node being executed
    int current = -1;
    //! Last child of current
    int last_child = -1;

    /**
     * \brief Start profiling
     * Start profiling by clearing the frame tree buffer and adding the root node.
     */
    void start() {
        // Clear buffer and add root
        in_progress.clear();
        current = 0;
        last_child = -1;
        in_progress.emplace_back(-1, "Root");
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
//        current = -1;
//        last_child = -1;
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
        auto n = static_cast<int>(in_progress.size());
        if (in_progress[current].child <= 0)
            in_progress[current].child = n;
        if (last_child > 0)
            in_progress[last_child].next = n;
        in_progress.emplace_back(current, label);

        // Update pointers
        current = n;
        last_child = -1;
    }

    /**
     * \brief Pop a block of code from the profiling stack
     */
    void pop() {
        // Skip if not started
        if (current < 0) return;

        // Compute current duration
        in_progress[current].time = glfwGetTime() - in_progress[current].time;

        // Update pointers
        last_child = current;
        current = in_progress[current].parent;
    }

    /**
     * \brief Get the last completed frame tree
     *
     * \return Last completed frame tree
     */
    std::vector<Node> get_last() { return completed; }

    // Recursive helper for show_text
    void write_rec(int i) {
        // Write the text
        ImGui::Text("%s - %.3f ms", completed[i].label.c_str(), completed[i].time * 1e3);

        // Do children if any
        if (completed[i].child > 0) {
            ImGui::Indent();
            write_rec(completed[i].child);
            ImGui::Unindent();
        }

        // Do next sibling if any
        if (completed[i].next > 0) {
            write_rec(completed[i].next);
        }
    }

    /**
     * \brief Show the text gui for the profiler
     */
    void show_text() {
        if (completed.empty()) {
            ImGui::TextUnformatted("No last completed frame tree");
        } else {
            // Line for each node, indent on children
            write_rec(0);
        }
    }

    constexpr float HEIGHT = 20.0f;
    constexpr float HEIGHT_PAD = HEIGHT + 3.0f;
    ImColor colA = IM_COL32(0, 255, 0, 255);
    ImColor colB = IM_COL32(255, 0, 0, 255);
    ImColor colT = IM_COL32(0, 0, 0, 255);

    // Recursive helper for show_graphical that draws the children of node
    void draw_rec(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size, int depth, float x_offset, int node, bool a) {
        // Loop over all children of the node
        int child = completed[node].child;
        while (child > 0) {
            // Compute width
            auto width = static_cast<float>(canvas_size.x * completed[child].time / completed[0].time);

            // Compute corners
            ImVec2 top_left{canvas_pos.x + x_offset, canvas_pos.y + (depth * HEIGHT_PAD)};
            ImVec2 bot_right{canvas_pos.x + x_offset + width, canvas_pos.y + HEIGHT + (depth * HEIGHT_PAD)};

            // Draw the rectangle
            draw_list->AddRectFilled(top_left, bot_right, a ? colA : colB);

            // Draw the tag
            std::ostringstream stream;
            stream << completed[child].label << " - " << completed[child].time * 1000 << " ms";
            std::string tag = stream.str();
            draw_list->AddText(top_left, colT, tag.data());

            // Recursively draw its children
            draw_rec(draw_list, canvas_pos, canvas_size, depth + 1, x_offset, child, true);

            // Increment anchors
            child = completed[child].next;
            x_offset += width;
            a = !a;
        }
    }

    void show_graphical() {
        if (completed.empty()) {
            ImGui::TextUnformatted("No last completed frame tree");
        } else {
            // Line for each hierarchy level, rectangle for each node sized relatively by duration and with tooltip info

            // Set up regions
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();

            double root_dur = completed[0].time;

            // Draw root
            draw_list->AddRectFilled(
                    canvas_pos,
                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + HEIGHT),
                    colA
            );
            std::ostringstream stream;
            stream << completed[0].label << " - " << completed[0].time * 1000 << " ms";
            std::string tag = stream.str();
            draw_list->AddText(canvas_pos, colT, tag.data());

            // Have each node recursively draw its children
            draw_rec(draw_list, canvas_pos, canvas_size, 1, 0.0f, 0, true);
        }
    }
}
