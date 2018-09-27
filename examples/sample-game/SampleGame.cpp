/*
 * Main point of the sample game example.
 *
 * Author: Filip Smola (smola.filip@hotmail.com)
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <open-sea/Log.h>
#include <open-sea/Window.h>
#include <open-sea/Input.h>
#include <open-sea/ImGui.h>
#include <open-sea/Delta.h>
#include <open-sea/GL.h>
#include <open-sea/Model.h>
#include <open-sea/Entity.h>
#include <open-sea/Components.h>
#include <open-sea/Render.h>
#include <open-sea/Systems.h>
#include <open-sea/Controls.h>
#include <open-sea/Debug.h>
#include <open-sea/Profiler.h>
namespace os_log = open_sea::log;
namespace window = open_sea::window;
namespace input = open_sea::input;
namespace imgui = open_sea::imgui;
namespace os_time = open_sea::time;
namespace gl = open_sea::gl;
namespace model = open_sea::model;
namespace ecs = open_sea::ecs;
namespace render = open_sea::render;
namespace controls = open_sea::controls;
namespace debug = open_sea::debug;
namespace profiler = open_sea::profiler;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/filesystem.hpp>

#include <sstream>
#include <random>
#include <vector>

int main() {
    // Initialize logging
    os_log::init_logging();
    os_log::severity_logger lg = os_log::get_logger("Sample Game");

    // Set the current path to outside the example directory
    boost::filesystem::current_path("../../");
    os_log::log(lg, os_log::info, "Working directory set to outside the example directory");

    // Initialize window module
    if (!window::init()) {
        return -1;
    }

    // Create window
    glm::ivec2 window_size{1280, 720};
    window::set_title("Sample Game");
    if (!window::make_windowed(window_size.x, window_size.y)) {
        return -1;
    }

    // Initialize input
    input::init();

    // Add close action to ESC
    input::connect_key([](int k, int c, input::state s, int m) {
        if (s == input::press && k == GLFW_KEY_ESCAPE) {
            window::close();
        }
    });

    // Start OpenGL error handling
#if !defined(OPEN_SEA_DEBUG_LOG)
    gl::log_errors();
#endif

    // Initialize ImGui
    imgui::init();

    // Add ImGui display toggle to F3
    bool show_imgui = false;
    input::connect_key([&show_imgui](int k, int c, input::state s, int m) {
        if (s == input::press && k == GLFW_KEY_F3) {
            // Toggle the display flag
            show_imgui = !show_imgui;
        }
    });

    // Prepare test cameras
    std::shared_ptr<gl::Camera> test_camera_ort =
            std::make_shared<gl::OrthographicCamera>(
                    glm::mat4(1.0f),
                    glm::vec2{window_size.x, window_size.y},
                    0.1f, 1000.0f);
    std::shared_ptr<gl::Camera> test_camera_per =
            std::make_shared<gl::PerspectiveCamera>(
                    glm::mat4(1.0f),
                    glm::vec2{window_size.x, window_size.y},
                    0.1f, 1000.0f, 90.0f);

    // Add borderless fullscreen control to F11
    bool windowed = true;
    input::connection borderless_toggle = input::connect_key([test_camera_ort, test_camera_per, window_size, &windowed]
                                                                     (int k, int c, input::state s, int m){
        if (k == GLFW_KEY_F11 && s == input::press) {
            if (windowed) {
                // Set borderless and update camera
                window::make_borderless(glfwGetPrimaryMonitor());
                test_camera_ort->set_size(
                        glm::vec2{window::current_properties().fb_width, window::current_properties().fb_height});
                test_camera_per->set_size(
                        glm::vec2{window::current_properties().fb_width, window::current_properties().fb_height});
                windowed = false;
            } else {
                // Set windowed and update camera
                window::make_windowed(window_size.x, window_size.y);
                test_camera_ort->set_size(
                        glm::vec2{window::current_properties().fb_width, window::current_properties().fb_height});
                test_camera_per->set_size(
                        glm::vec2{window::current_properties().fb_width, window::current_properties().fb_height});
                windowed = true;
            }
        }
    });

    // Generate test entities
    unsigned n = 1024;
    std::shared_ptr<ecs::EntityManager> test_manager = std::make_shared<ecs::EntityManager>();
    ecs::Entity entities[n];
    test_manager->create(entities, n);
    debug::add_entity_manager(test_manager, "Test Manager");

    // Prepare and assign model
    std::shared_ptr<ecs::ModelComponent> model_comp_manager = std::make_shared<ecs::ModelComponent>();
    {
        std::shared_ptr<model::Model> model(model::UntexModel::from_file("examples/sample-game/data/models/cube.obj"));
        if (!model) {
            return -1;
        }
        model_comp_manager->model_to_index(model);
        std::vector<int> models(n);   // modelIdx == 0, because it is the first model

        model_comp_manager->add(entities, models.data(), n);
    }
    debug::add_component_manager(model_comp_manager, "Model");

    // Prepare and assign random transformations
    std::shared_ptr<ecs::TransformationComponent> trans_comp_manager = std::make_shared<ecs::TransformationComponent>();
    {
        // Prepare random distributions
        std::random_device device;
        std::mt19937_64 generator;

        std::uniform_int_distribution<int> pos_x(-640, 640);
        std::uniform_int_distribution<int> pos_y(-360, 360);
        std::uniform_int_distribution<int> pos_z(0, 750);

        std::uniform_real_distribution<float> angle(0.0f, 360.0f);
        glm::vec3 axis{0.0f, 0.0f, 1.0f};

        std::uniform_real_distribution<float> scale(1.0f, 20.0f);

        // Prepare data arrays
        std::vector<glm::vec3> positions(n);
        std::vector<glm::quat> orientations(n);
        std::vector<glm::vec3> scales(n);

        // Randomise the data
        glm::vec3 *p = positions.data();
        glm::quat *o = orientations.data();
        glm::vec3 *s = scales.data();
        for (int i = 0; i < n; i++, p++, o++, s++) {
            *p = glm::vec3(pos_x(generator), pos_y(generator), pos_z(generator));
            *o = glm::angleAxis(angle(generator), axis);
            float f = scale(generator);
            *s = glm::vec3(f, f, f);
        }

        os_log::log(lg, os_log::info, "Transformations generated");

        // Add the components
        trans_comp_manager->add(entities, positions.data(), orientations.data(), scales.data(), n);
        os_log::log(lg, os_log::info, "Transformations set");
    }
    debug::add_component_manager(trans_comp_manager, "Transformation");

    // Prepare renderer
    std::shared_ptr<render::UntexturedRenderer> renderer = std::make_shared<render::UntexturedRenderer>(model_comp_manager, trans_comp_manager);
    debug::add_system(renderer, "Untextured Renderer");

    // Create camera guide entity
    ecs::Entity camera_guide = test_manager->create();
    glm::vec3 camera_guide_pos{0.0f, 0.0f, 1000.0f};
    glm::quat camera_guide_ori{0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec3 camera_guide_sca(1.0f, 1.0f, 1.0f);
    trans_comp_manager->add(&camera_guide, &camera_guide_pos, &camera_guide_ori, &camera_guide_sca, 1);

    // Prepare camera component
    std::shared_ptr<ecs::CameraComponent> camera_comp_manager = std::make_shared<ecs::CameraComponent>();
    {
        std::shared_ptr<gl::Camera> cameras[]{
                std::shared_ptr(test_camera_per),
                std::shared_ptr(test_camera_ort)
        };

        ecs::Entity es[]{
                camera_guide,
                camera_guide
        };

        camera_comp_manager->add(es, cameras, 2);
    }
    debug::add_component_manager(camera_comp_manager, "Camera");

    // Prepare camera follow system
    std::shared_ptr<ecs::CameraFollow> camera_follow = std::make_shared<ecs::CameraFollow>(trans_comp_manager, camera_comp_manager);
    debug::add_system(camera_follow, "Camera Follow");
    
    // Prepare controls for the camera guide
    int controls_no = 0;
    controls::Free::Config controls_free_config {
            .forward = input::UnifiedInput::keyboard(GLFW_KEY_W),
            .backward = input::UnifiedInput::keyboard(GLFW_KEY_S),
            .left = input::UnifiedInput::keyboard(GLFW_KEY_A),
            .right = input::UnifiedInput::keyboard(GLFW_KEY_D),
            .up = input::UnifiedInput::keyboard(GLFW_KEY_LEFT_SHIFT),
            .down = input::UnifiedInput::keyboard(GLFW_KEY_LEFT_CONTROL),
            .clockwise = input::UnifiedInput::keyboard(GLFW_KEY_Q),
            .counter_clockwise = input::UnifiedInput::keyboard(GLFW_KEY_E),
            .speed_x = 150.0f,
            .speed_z = 150.0f,
            .speed_y = 150.0f,
            .turn_rate = 0.3f,
            .roll_rate = 30.0f
    };
    std::shared_ptr<controls::Controls> controls_free = std::make_shared<controls::Free>(trans_comp_manager, camera_guide, controls_free_config);
    debug::add_controls(controls_free, "Free Controls");
    controls::FPS::Config controls_fps_config {
            .forward = input::UnifiedInput::keyboard(GLFW_KEY_W),
            .backward = input::UnifiedInput::keyboard(GLFW_KEY_S),
            .left = input::UnifiedInput::keyboard(GLFW_KEY_A),
            .right = input::UnifiedInput::keyboard(GLFW_KEY_D),
            .speed_x = 150.0f,
            .speed_z = 150.0f,
            .turn_rate = 0.3f
    };
    std::shared_ptr<controls::Controls> controls_fps = std::make_shared<controls::FPS>(trans_comp_manager, camera_guide, controls_fps_config);
    debug::add_controls(controls_fps, "FPS Controls");
    controls::TopDown::Config controls_td_config {
            .left = input::UnifiedInput::keyboard(GLFW_KEY_A),
            .right = input::UnifiedInput::keyboard(GLFW_KEY_D),
            .up = input::UnifiedInput::keyboard(GLFW_KEY_LEFT_SHIFT),
            .down = input::UnifiedInput::keyboard(GLFW_KEY_LEFT_CONTROL),
            .clockwise = input::UnifiedInput::keyboard(GLFW_KEY_Q),
            .counter_clockwise = input::UnifiedInput::keyboard(GLFW_KEY_E),
            .speed_x = 150.0f,
            .speed_y = 150.0f,
            .roll_rate = 30.0f
    };
    std::shared_ptr<controls::Controls> controls_td = std::make_shared<controls::TopDown>(trans_comp_manager, camera_guide, controls_td_config);
    debug::add_controls(controls_td, "Top Down Controls");

    // Add suspend controls button
    bool suspend_controls = false;
    const input::UnifiedInput suspend_binding = input::UnifiedInput::keyboard(GLFW_KEY_F1);
    input::connect_unified([&suspend_controls, suspend_binding](input::UnifiedInput i, input::state a){
        if (i == suspend_binding && a == input::press) {
            suspend_controls = !suspend_controls;
        }
    });

    // Add profiler menu
    bool profiler_toggle = true;
    bool profiler_text_display = false;
    bool profiler_graphical_display = false;
    debug::menu_func profiler_menu = [&profiler_toggle, &profiler_text_display, &profiler_graphical_display](){
        if (ImGui::MenuItem("Toggle Profile", nullptr, &profiler_toggle)) {}
        if (ImGui::MenuItem("Text Display", nullptr, &profiler_text_display)) {}
        if (ImGui::MenuItem("Graphical Display", nullptr, &profiler_graphical_display)) {}
        if (ImGui::MenuItem("Clear Maximum")) { profiler::clear_maximum(); }
    };
    debug::add_menu(profiler_menu, "Profiler");

    // Add test environment menu
    bool use_per_camera = true;
    bool camera_info = false;
    debug::menu_func environment_menu = [&use_per_camera, &controls_no, &suspend_controls, &camera_info](){
        if (ImGui::MenuItem("Suspend Controls", nullptr, &suspend_controls)) {}
        if (ImGui::BeginMenu("Active Camera:")) {
            if (ImGui::MenuItem("Perspective", nullptr, use_per_camera)) { use_per_camera = true; }
            if (ImGui::MenuItem("Orthographic", nullptr, !use_per_camera)) { use_per_camera = false; }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Active Controls:")) {
            if (ImGui::MenuItem("Free", nullptr, (controls_no == 0))) { controls_no = 0; }
            if (ImGui::MenuItem("FPS", nullptr, (controls_no == 1))) { controls_no = 1; }
            if (ImGui::MenuItem("Top Down", nullptr, (controls_no == 2))) { controls_no = 2; }

            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Camera Info", nullptr, &camera_info)) {}
    };
    debug::add_menu(environment_menu, "Test Environment");

    // Set background to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Update cursor delta once before main loop to avoid extreme first cursor delta
    input::update_cursor_delta();

    // Loop until the user closes the window
    open_sea::time::start_delta();
    while (!window::should_close()) {
        // Start profiling
        if (profiler_toggle) {
            profiler::start();
        }

        // Clear
        profiler::push("glClear");
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        profiler::pop();

        // Update cursor delta
        profiler::push("Input Update");
        input::update_cursor_delta();
        profiler::pop();

        // Update camera guide controls
        profiler::push("Camera Controls");
        if (!suspend_controls) {
            switch (controls_no) {
                case 0: controls_free->transform(); break;
                case 1: controls_fps->transform(); break;
                case 2: controls_td->transform(); break;
                default: return -1;
            }
        } else {
            input::set_cursor_mode(input::cursor_mode::normal);
        }
        profiler::pop();

        // Update cameras based on associated guides
        profiler::push("Camera Transform");
        camera_follow->transform();
        profiler::pop();

        profiler::push("Draw");
        // Decide what camera to use
        std::shared_ptr<gl::Camera> camera = (use_per_camera) ? test_camera_per : test_camera_ort;

        // Draw the entities
        renderer->render(camera, entities, n);
        profiler::pop();

        // Maintain components
        profiler::push("Maintain Components");
        model_comp_manager->gc(*test_manager);
        trans_comp_manager->gc(*test_manager);
        camera_comp_manager->gc(*test_manager);
        profiler::pop();

        // ImGui debug GUI
        profiler::push("ImGui Debug GUI");
        if (show_imgui) {
            // Prepare new frame
            profiler::push("New Frame");
            imgui::new_frame();
            profiler::pop();

            // Main menu
            profiler::push("Main Menu");
            debug::main_menu();
            profiler::pop();

            // Camera info window
            profiler::push("Camera Info");
            if (camera_info) {
                if (ImGui::Begin("Active Camera")) {
                    camera->show_debug();
                }
                ImGui::End();
            }
            profiler::pop();

            // Profiler text display
            profiler::push("Profiler - Text Diplay");
            if (profiler_text_display) {
                if (ImGui::Begin("Profiler - Text Display")) {
                    profiler::show_text();
                }
                ImGui::End();
            }
            profiler::pop();

            // Profiler graphical GUI
            profiler::push("Profiler - Graphical Display");
            if (profiler_graphical_display) {
                if (ImGui::Begin("Profiler - Graphical Display")) {
                    profiler::show_graphical();
                }
                ImGui::End();
            }
            profiler::pop();

            //Render
            profiler::push("Render");
            imgui::render();
            profiler::pop();
        }
        profiler::pop();

        // Update the window
        profiler::push("Window Update");
        window::update();
        profiler::pop();

        // Try to finish profiling
        profiler::finish();

        // Update delta time
        open_sea::time::update_delta();
    }
    os_log::log(lg, os_log::info, "Main loop ended");

    // Clean up OpenGL objects before termination of the context
    model_comp_manager.reset();
    renderer.reset();
    debug::clean_up();
    imgui::clean_up();

    // Terminate OpenGL context and clean up window and logging modules
    window::clean_up();
    window::terminate();
    os_log::clean_up();

    return 0;
}
