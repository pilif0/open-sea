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
    if (!window::init())
        return -1;

    // Create window
    constexpr glm::ivec2 window_size{1280, 720};
    window::set_title("Sample Game");
    if (!window::make_windowed(window_size.x, window_size.y))
        return -1;

    // Initialize input
    input::init();

    // Add close action to ESC
    input::connection c = input::connect_key([](int k, int c, input::state s, int m) {
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
    input::connection imgui_toggle = input::connect_key([&show_imgui](int k, int c, input::state s, int m) {
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
                test_camera_ort->setSize(glm::vec2{window::current_properties().fbWidth, window::current_properties().fbHeight});
                test_camera_per->setSize(glm::vec2{window::current_properties().fbWidth, window::current_properties().fbHeight});
                windowed = false;
            } else {
                // Set windowed and update camera
                window::make_windowed(window_size.x, window_size.y);
                test_camera_ort->setSize(glm::vec2{window::current_properties().fbWidth, window::current_properties().fbHeight});
                test_camera_per->setSize(glm::vec2{window::current_properties().fbWidth, window::current_properties().fbHeight});
                windowed = true;
            }
        }
    });

    // Generate test entities
    unsigned N = 1024;
    std::shared_ptr<ecs::EntityManager> test_manager = std::make_shared<ecs::EntityManager>();
    ecs::Entity entities[N];
    test_manager->create(entities, N);
    debug::add_entity_manager(test_manager, "Test Manager");

    // Prepare and assign model
    std::shared_ptr<ecs::ModelComponent> model_comp_manager = std::make_shared<ecs::ModelComponent>();
    {
        std::shared_ptr<model::Model> model(model::UntexModel::fromFile("examples/sample-game/data/models/cube.obj"));
        if (!model)
            return -1;
        model_comp_manager->modelToIndex(model);
        std::vector<int> models(N);   // modelIdx == 0, because it is the first model

        model_comp_manager->add(entities, models.data(), N);
    }
    debug::add_component_manager(model_comp_manager, "Model");

    // Prepare and assign random transformations
    std::shared_ptr<ecs::TransformationComponent> trans_comp_manager = std::make_shared<ecs::TransformationComponent>();
    {
        // Prepare random distributions
        std::random_device device;
        std::mt19937_64 generator;

        std::uniform_int_distribution<int> posX(-640, 640);
        std::uniform_int_distribution<int> posY(-360, 360);
        std::uniform_int_distribution<int> posZ(0, 750);

        std::uniform_real_distribution<float> angle(0.0f, 360.0f);
        glm::vec3 axis{0.0f, 0.0f, 1.0f};

        std::uniform_real_distribution<float> scale(1.0f, 20.0f);

        // Prepare data arrays
        std::vector<glm::vec3> positions(N);
        std::vector<glm::quat> orientations(N);
        std::vector<glm::vec3> scales(N);

        // Randomise the data
        glm::vec3 *p = positions.data();
        glm::quat *o = orientations.data();
        glm::vec3 *s = scales.data();
        for (int i = 0; i < N; i++, p++, o++, s++) {
            *p = glm::vec3(posX(generator), posY(generator), posZ(generator));
            *o = glm::angleAxis(angle(generator), axis);
            float f = scale(generator);
            *s = glm::vec3(f, f, f);
        }

        os_log::log(lg, os_log::info, "Transformations generated");

        // Add the components
        trans_comp_manager->add(entities, positions.data(), orientations.data(), scales.data(), N);
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
    controls::Free::Config controls_config {
            .forward = input::unified_input::keyboard(GLFW_KEY_W),
            .backward = input::unified_input::keyboard(GLFW_KEY_S),
            .left = input::unified_input::keyboard(GLFW_KEY_A),
            .right = input::unified_input::keyboard(GLFW_KEY_D),
            .up = input::unified_input::keyboard(GLFW_KEY_LEFT_SHIFT),
            .down = input::unified_input::keyboard(GLFW_KEY_LEFT_CONTROL),
            .clockwise = input::unified_input::keyboard(GLFW_KEY_Q),
            .counter_clockwise = input::unified_input::keyboard(GLFW_KEY_E),
            .speed_x = 150.0f,
            .speed_z = 150.0f,
            .speed_y = 150.0f,
            .turn_rate = 0.3f,
            .roll_rate = 30.0f
    };
    std::unique_ptr<controls::Controls> controls = std::make_unique<controls::Free>(trans_comp_manager, camera_guide, controls_config);

    // Add suspend controls button
    bool suspend_controls = false;
    const input::unified_input suspend_binding = input::unified_input::keyboard(GLFW_KEY_F1);
    input::connect_unified([&suspend_controls, suspend_binding](input::unified_input i, input::state a){
        if (i == suspend_binding && a == input::press) {
            suspend_controls = !suspend_controls;
        }
    });

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
        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update cursor delta
        input::update_cursor_delta();

        // Update camera guide controls
        if (!suspend_controls)
            controls->transform();
        else
            glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // Update cameras based on associated guides
        camera_follow->transform();

        // Draw the entities
        static bool use_per_cam = true;
        renderer->render((use_per_cam) ? test_camera_per : test_camera_ort, entities, N);

        // Maintain components
        model_comp_manager->gc(*test_manager);
        trans_comp_manager->gc(*test_manager);
        camera_comp_manager->gc(*test_manager);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Main menu
            debug::main_menu();

            // Test camera controls
            {
                ImGui::Begin("Test camera controls");

                controls->showDebug();

                ImGui::End();
            }

            // Test environment controls
            {
                ImGui::Begin("Test environment controls");

                ImGui::Checkbox("Use perspective camera", &use_per_cam);
                ImGui::Checkbox("Suspend controls", &suspend_controls);

                ImGui::Text("Test camera:");
                if (use_per_cam)
                    test_camera_per->showDebugControls();
                else
                    test_camera_ort->showDebugControls();
                ImGui::Spacing();

                ImGui::End();
            }

            //Render
            imgui::render();
        }

        // Update the window
        window::update();

        // Update delta time
        open_sea::time::update_delta();
    }
    os_log::log(lg, os_log::info, "Main loop ended");

    // Clean up OpenGL objects before termination of the context
    model_comp_manager.reset();
    renderer.reset();

    c.disconnect();
    imgui_toggle.disconnect();
    imgui::clean_up();
    window::clean_up();
    window::terminate();
    os_log::clean_up();

    return 0;
}
