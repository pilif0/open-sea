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
#include <open-sea/Model.h>
#include <open-sea/Entity.h>
#include <open-sea/Components.h>
#include <open-sea/Render.h>
#include <open-sea/Systems.h>
namespace os_log = open_sea::log;
namespace window = open_sea::window;
namespace input = open_sea::input;
namespace imgui = open_sea::imgui;
namespace os_time = open_sea::time;
namespace gl = open_sea::gl;
namespace model = open_sea::model;
namespace ecs = open_sea::ecs;
namespace render = open_sea::render;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/filesystem.hpp>

#include <sstream>
#include <random>
#include <vector>

//! Whether mouse input should be used for turning the camera
bool camera_want_turn = false;
//! Last cursor position used for turning the camera
glm::vec2 last_cursor_pos{};

/**
 * \brief Slot to toggle camera turn mode with RMB click
 *
 * \param button
 * \param action
 * \param mods
 */
void camera_mode_toggle(int button, input::state action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_2 && action == input::press) {
        if (camera_want_turn) {
            camera_want_turn = false;
            ::glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            camera_want_turn = true;
            ::glfwSetInputMode(window::window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            last_cursor_pos = input::cursor_position();
        }
    }
}

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
    window::set_title("Sample Game");
    if (!window::make_windowed(1280, 720))
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

    // ImGui test data
    bool show_demo_window = true;

    // Prepare test cameras
    std::shared_ptr<gl::Camera> test_camera_ort =
            std::make_shared<gl::OrthographicCamera>(
                    glm::vec3{},
                    glm::quat(),
                    glm::vec2{1280, 720},
                    0.1f, 1000.0f);
    std::shared_ptr<gl::Camera> test_camera_per =
            std::make_shared<gl::PerspectiveCamera>(
                    glm::vec3{},
                    glm::quat(),
                    glm::vec2{1280, 720},
                    0.1f, 1000.0f, 90.0f);

    // Generate test entities
    unsigned N = 1024;
    ecs::EntityManager test_manager;
    ecs::Entity entities[N];
    test_manager.create(entities, N);

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

    // Prepare renderer
    std::unique_ptr<render::UntexturedRenderer> renderer = std::make_unique<render::UntexturedRenderer>(model_comp_manager, trans_comp_manager);

    // Create camera guide entity
    ecs::Entity camera_guide = test_manager.create();
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

    // Prepare camera follow system
    std::unique_ptr<ecs::CameraFollow> camera_follow = std::make_unique<ecs::CameraFollow>(trans_comp_manager, camera_comp_manager);

    // Connect a slot to click of right mouse button to toggle camera turn mode
    input::connect_mouse(camera_mode_toggle);

    // Set background to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Loop until the user closes the window
    open_sea::time::start_delta();
    while (!window::should_close()) {
        // Clear
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update camera guide position
        glm::vec3 camera_global{};
        {
            constexpr float camera_speed = 150;     // Units per second
            int camera_index = trans_comp_manager->lookup(camera_guide);
            glm::vec3 local{};

            // Gather input
            if (input::key_state(GLFW_KEY_A) == input::press)
                // A pressed -> move left from the perspective of the camera
                local.x += -1;
            if (input::key_state(GLFW_KEY_D) == input::press)
                // D pressed -> move right from the perspective of the camera
                local.x += 1;
            if (input::key_state(GLFW_KEY_W) == input::press)
                // W pressed -> move forward from the perspective of the camera
                local.z += -1;
            if (input::key_state(GLFW_KEY_S) == input::press)
                // S pressed -> move backward from the perspective of the camera
                local.z += 1;
            if (input::key_state(GLFW_KEY_LEFT_SHIFT) == input::press)
                // Shift pressed -> move up from the perspective of the camera
                local.y += 1;
            if (input::key_state(GLFW_KEY_LEFT_CONTROL) == input::press)
                // Ctrl pressed -> move down from the perspective of the camera
                local.y += -1;

            // Normalise
            float l = glm::length(local);
            if (l != 0.0f && l != 1.0f)
                local /= l;

            // Transform and apply
            camera_global = glm::rotate(trans_comp_manager->data.orientation[camera_index], local);
            camera_global *= camera_speed * os_time::get_delta();
            trans_comp_manager->translate(&camera_index, &camera_global, 1);
        }

        // Update camera guide rotation
        glm::vec2 cursor_delta{};
        glm::quat camera_rot{};
        if (camera_want_turn) {
            constexpr float turn_rate = 0.3f;                   // Degrees per screen unit
            constexpr glm::vec3 pitch_axis{1.0f, 0.0f, 0.0f};   // Positive x axis
            constexpr glm::vec3 yaw_axis{0.0f, 1.0f, 0.0f};     // Positive y axis
            constexpr glm::vec3 roll_axis{0.0f, 0.0f, 1.0f};     // Positive z axis
            int camera_index = trans_comp_manager->lookup(camera_guide);

            // Compute delta
            glm::vec2 cursor_pos = input::cursor_position();
            cursor_delta = cursor_pos - last_cursor_pos;

            // Positive pitch is up
            float pitch = cursor_delta.y * (- turn_rate);

            // Positive yaw is left
            float yaw = cursor_delta.x * (- turn_rate);

            // Positive roll is counter clockwise
            float roll = 0.0f;
            if (input::key_state(GLFW_KEY_Q) == input::press)
                roll += turn_rate * 100;
            if (input::key_state(GLFW_KEY_E) == input::press)
                roll -= turn_rate * 100;
            roll *= os_time::get_delta();

            // Compute transformation quaternion and transform
            if (pitch != 0 || yaw != 0 || roll != 0) {
                // Transform axes of rotation
                glm::quat camera_or = trans_comp_manager->data.orientation[camera_index];
                glm::vec3 tr_pitch_ax = glm::rotate(camera_or,  pitch_axis);
                glm::vec3 tr_yaw_ax = glm::rotate(camera_or, yaw_axis);
                glm::vec3 tr_roll_ax = glm::rotate(camera_or, roll_axis);

                // Compute transformation
                glm::quat pitchQ = glm::angleAxis(glm::radians(pitch), tr_pitch_ax);
                glm::quat yawQ = glm::angleAxis(glm::radians(yaw), tr_yaw_ax);
                glm::quat rollQ = glm::angleAxis(glm::radians(roll), tr_roll_ax);
                camera_rot = rollQ * yawQ * pitchQ;

                // Apply
                trans_comp_manager->rotate(&camera_index, &camera_rot, 1);
            }

            // Update last cursor position
            last_cursor_pos = cursor_pos;
        }

        // Update cameras based on associated guides
        camera_follow->transform();

        // Draw the entities
        static bool use_per_cam = true;
        renderer->render((use_per_cam) ? test_camera_per : test_camera_ort, entities, N);

        // Maintain components
        model_comp_manager->gc(test_manager);
        trans_comp_manager->gc(test_manager);
        camera_comp_manager->gc(test_manager);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Entity test
            {
                ImGui::Begin("Entity test");

                test_manager.showDebug();

                if (ImGui::CollapsingHeader("Model Component Manager")) {
                    model_comp_manager->showDebug();
                }
                if (ImGui::CollapsingHeader("Transformation Component Manager")) {
                    trans_comp_manager->showDebug();
                }

                ImGui::End();
            }

            // Test controls
            {
                ImGui::Begin("Test controls");

                ImGui::Checkbox("Use perspective camera", &use_per_cam);

                ImGui::Text("Camera want turn: %s", (camera_want_turn) ? "true" : "false");
                ImGui::Text("Cursor delta: %.3f, %.3f", cursor_delta.x, cursor_delta.y);
                ImGui::Text("Camera control delta: %.3f, %.3f, %.3f", camera_global.x, camera_global.y, camera_global.z);
                ImGui::Text("Camera control rotation: %.3f, %.3f, %.3f, %.3f", camera_rot.x, camera_rot.y, camera_rot.z, camera_rot.w);

                ImGui::Text("Test camera:");
                if (use_per_cam)
                    test_camera_per->showDebugControls();
                else
                    test_camera_ort->showDebugControls();
                ImGui::Spacing();

                ImGui::End();
            }

            // Additional window open flags
            static bool show_window_debug = false;
            static bool show_input_debug = false;
            static bool show_opengl_debug = false;
            static bool show_demo = false;

            // System stats
            {
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
                ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
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
