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

    // Initialize input
    input::init();

    // Add close action to ESC
    input::connection c = input::connect_key([](int k, int c, input::state s, int m){
        if (s == input::press && k == GLFW_KEY_ESCAPE)
            window::close();
    });

    // Loop until the user closes the window
    while (!window::should_close()) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Update the window
        window::update();
    }
    log::log(lg, log::info, "Main loop ended");

    c.disconnect();
    window::clean_up();
    log::clean_up();
    window::terminate();

    return 0;
}
