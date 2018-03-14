/*
 * Main point of the sample game example.
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <open-sea/Log.h>
#include <open-sea/Window.h>
#include <open-sea/Input.h>
#include <open-sea/ImGui.h>
#include <open-sea/Delta.h>
#include <open-sea/GL.h>
namespace log = open_sea::log;
namespace window = open_sea::window;
namespace input = open_sea::input;
namespace imgui = open_sea::imgui;
namespace gl = open_sea::gl;

#include <boost/filesystem.hpp>

int main() {
    // Initialize logging
    log::init_logging();
    log::severity_logger lg = log::get_logger("Sample Game");

    // Set the current path to outside the example directory
    boost::filesystem::current_path("../");
    log::log(lg, log::info, "Working directory set to outside the example directory");

    // Initialize window module
    if (!window::init())
        return -1;

    // Create window
    window::set_title("Sample Game");
    if (!window::make_windowed(1280, 720))
        return -1;

    // Initialize input
    input::init();

    // Add close action to ESC
    input::connection c = input::connect_key([](int k, int c, input::state s, int m){
        if (s == input::press && k == GLFW_KEY_ESCAPE)
            window::close();
    });

    // Start OpenGL error handling
#if !defined(OPEN_SEA_DEBUG)
    gl::log_errors();
#endif

    // Initialize ImGui
    imgui::init();

    // Add ImGui display toggle to F3
    bool show_imgui = false;
    input::connection imgui_toggle = input::connect_key([&show_imgui](int k, int c, input::state s, int m){
        if (s == input::press && k == GLFW_KEY_F3) {
            // Toggle the display flag
            show_imgui = !show_imgui;

            // Toggle input connection
            if (show_imgui)
                imgui::connect_listeners();
            else
                imgui::disconnect_listeners();
        }
    });

    // ImGui test data
    bool show_demo_window = true;

    // Loop until the user closes the window
    open_sea::time::start_delta();
    while (!window::should_close()) {
        // Clear
        glClear(GL_COLOR_BUFFER_BIT);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Additional window open flags
            static bool show_window_debug = false;
            static bool show_input_debug = false;
            static bool show_opengl_debug = false;
            static bool show_demo = false;

            // System stats
            {
                ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
                ImGui::Begin("System Statistics");

                open_sea::time::debug_widget();

                if (ImGui::CollapsingHeader("Additional windows")) {
                    ImGui::Checkbox("Window info", &show_window_debug);
                    ImGui::Checkbox("Input info", &show_input_debug);
                    ImGui::Checkbox("OpenGL info", &show_opengl_debug);
                    ImGui::Checkbox("ImGui demo", &show_demo);
                }


                ImGui::End();
            }

            // Window info
            if (show_window_debug)
                window::show_debug();

            // Input info
            if (show_input_debug)
                input::show_debug();

            // OpenGL info
            if (show_opengl_debug)
                gl::debug_window();

            // Demo window
            if (show_demo) {
                ImGui::SetNextWindowPos(ImVec2(650, 20),
                                        ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
                ImGui::ShowDemoWindow(&show_demo_window);
            }

            //Render
            imgui::render();
        }

        // Update the window
        window::update();

        // Update delta time
        open_sea::time::update_delta();
    }
    log::log(lg, log::info, "Main loop ended");

    c.disconnect();
    imgui_toggle.disconnect();
    imgui::clean_up();
    window::clean_up();
    log::clean_up();
    window::terminate();

    return 0;
}
