/*
 * Main point of the sample game example.
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 *
 * Based on GLFW example from http://www.glfw.org/documentation.html
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <open-sea/config.h>
#include <open-sea/Test.h>

#include <fstream>
#include <iomanip>
#include <iostream>

// Boost shared pointer
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

// Boost logging
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;
namespace triv = boost::log::trivial;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;

// GLFW error callback that prints to console
void error_callback(int error, const char* description) {
    src::severity_logger<triv::severity_level> lg;
    BOOST_LOG_SEV(lg, triv::error) << "GLFW error " << error << ": " << description;
}

// Initialize the file sink with path "log/main.log"
void init_file_sink() {
    // Prepare the sink
    typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
    boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
    boost::shared_ptr<std::ofstream> stream = boost::make_shared<std::ofstream>("log/main.log");
    sink->locked_backend()->add_stream(stream);

#if !defined(OPEN_SEA_DEBUG)
    // Filter out log records that are not at least warnings
    sink->set_filter(
            logging::trivial::severity >= logging::trivial::warning
    );
#endif

    // Enable auto flush
    sink->locked_backend()->auto_flush(true);

    // Set the formatter
    sink->set_formatter(
            expr::stream
                    << std::hex << std::setw(8) << std::setfill('0') << expr::attr<unsigned int>("LineID")
                    << ": <" << logging::trivial::severity
                    << "> " << expr::smessage
    );

    // Add the sink
    logging::core::get()->add_sink(sink);
}

// Initialize logging
void init_logging() {
    // Initialize the file sink
    init_file_sink();

    // Add common logging attributes
    logging::add_common_attributes();

    // Note start of logging
    src::severity_logger<triv::severity_level> lg;
    BOOST_LOG_SEV(lg, triv::info) << "Logging initialized";
}

int main() {
    // Initialize logging
    init_logging();
    src::severity_logger<triv::severity_level> lg;

    GLFWwindow* window;

    // Set the callback
    glfwSetErrorCallback(error_callback);

    // Initialize GLFW
    if (!glfwInit()) {
        BOOST_LOG_SEV(lg, triv::fatal) << "GLFW initialisation failed";
        return -1;
    }
    BOOST_LOG_SEV(lg, triv::info) << "GLFW initialised";

    // Ask for OpenGL 3.3 forward compatible context
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(640, 480, get_test_string().c_str(), NULL, NULL);
    if (!window) {
        BOOST_LOG_SEV(lg, triv::fatal) << "Window creation failed";

        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        BOOST_LOG_SEV(lg, triv::fatal) << "Failed to initialize OpenGL context";
        return -1;
    }
    BOOST_LOG_SEV(lg, triv::info) << "Window and context created";

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }
    BOOST_LOG_SEV(lg, triv::info) << "Main loop ended";

    glfwTerminate();
    return 0;
}
