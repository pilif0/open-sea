/*
 * Main point of the sample game example.
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <open-sea/Test.h>
#include <open-sea/Log.h>
namespace log = open_sea::log;
#include <open-sea/Window.h>
namespace window = open_sea::window;
#include <open-sea/Input.h>
namespace input = open_sea::input;
#include <open-sea/ImGui.h>
namespace imgui = open_sea::imgui;

#include <imgui.h>

int main() {
    // Initialize logging
    log::init_logging();
    log::severity_logger lg = log::get_logger("Sample Game");

    // Initialize window module
    if (!window::init())
        return -1;

    // Create window
    window::set_title("Sample Game");
    if (!window::make_windowed(1280, 720))
        return -1;

    // Add focus notifier to the window
    window::connection focusConnection = window::connect_focus([](bool f){
        static log::severity_logger lg = log::get_logger("Focus Notifier");
        if (f)
            log::log(lg, log::info, "Focused");
        else
            log::log(lg, log::info, "Unfocused");
    });

    // Initialize input
    input::init();

    // Add close action to ESC
    input::connection c = input::connect_key([](int k, int c, input::state s, int m){
        if (s == input::press && k == GLFW_KEY_ESCAPE)
            window::close();
    });

    // Initialize ImGui
    imgui::init();

    // Add ImGui display toggle to F3
    bool show_imgui = false;
    input::connection imguiToggle = input::connect_key([&show_imgui](int k, int c, input::state s, int m){
        if (s == input::press && k == GLFW_KEY_F3)
            show_imgui = !show_imgui;
    });

    // ImGui test data
    bool show_demo_window = true;

    // Loop until the user closes the window
    while (!window::should_close()) {
        // Clear
        glClear(GL_COLOR_BUFFER_BIT);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Additional window open flags
            static bool show_window_debug = false;

            // System stats
            {
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
                ImGui::Begin("System Statistics");

                ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                            1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);

                if (ImGui::CollapsingHeader("Additional windows:")) {
                    ImGui::Checkbox("Window info", &show_window_debug);
                }


                ImGui::End();
            }

            // Window info
            if (show_window_debug)
                window::show_debug();

            // Demo window
            {
                ImGui::SetNextWindowPos(ImVec2(650, 20),
                                        ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
                ImGui::ShowDemoWindow(&show_demo_window);
            }

            //Render
            imgui::render();
        }

        // Update the window
        window::update();
    }
    log::log(lg, log::info, "Main loop ended");

    c.disconnect();
    focusConnection.disconnect();
    imgui::clean_up();
    window::clean_up();
    log::clean_up();
    window::terminate();

    return 0;
}
