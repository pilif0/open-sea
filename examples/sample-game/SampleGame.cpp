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
namespace os_log = open_sea::log;
namespace window = open_sea::window;
namespace input = open_sea::input;
namespace imgui = open_sea::imgui;
namespace gl = open_sea::gl;
namespace model = open_sea::model;
namespace ecs = open_sea::ecs;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <boost/filesystem.hpp>

#include <sstream>

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

    // Prepare test shader
    std::unique_ptr<gl::ShaderProgram> test_shader = std::make_unique<gl::ShaderProgram>();
    test_shader->attachVertexFile("data/shaders/Test.vshader");
    test_shader->attachFragmentFile("data/shaders/Test.fshader");
    test_shader->link();
    test_shader->validate();
    int pM_location = test_shader->getUniformLocation("projectionMatrix");
    int wM_location = test_shader->getUniformLocation("worldMatrix");

    // Prepare test cameras
    std::unique_ptr<gl::OrthographicCamera> test_camera_ort =
            std::make_unique<gl::OrthographicCamera>(
                    glm::vec3{-640.0f, -360.0f, 1000.0f},
                    glm::quat(),
                    glm::vec2{1280, 720},
                    0.1f, 1000.0f);
    std::unique_ptr<gl::PerspectiveCamera> test_camera_per =
            std::make_unique<gl::PerspectiveCamera>(
                    glm::vec3{0.0f, 0.0f, 1000.0f},
                    glm::quat(),
                    glm::vec2{1280, 720},
                    0.1f, 1000.0f, 90.0f);

    // Generate test entities
    ecs::EntityManager test_manager;
    ecs::Entity entities[2];
    test_manager.create(entities, 2);

    // Prepare and assign test models
    std::unique_ptr<ecs::ModelComponent> model_comp_manager = std::make_unique<ecs::ModelComponent>(2);
    {
        std::shared_ptr<model::Model> models[]{
                model::Model::fromFile("examples/sample-game/data/models/teapot.obj"),
                model::UntexModel::fromFile("examples/sample-game/data/models/teapot.obj")
        };
        if (!models[0] || !models[1]) {
            return -1;
        }

        model_comp_manager->add(entities, models, 2);
    }

    // Prepare and assign test transformations
    std::unique_ptr<ecs::TransformationComponent> trans_comp_manager = std::make_unique<ecs::TransformationComponent>(2);
    {
        glm::vec3 positions[]{
                {10.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 0.0f}
        };
        glm::quat orientations[]{
                glm::angleAxis(0.0f, glm::vec3{1.0f, 0.0f, 0.0f}),
                glm::angleAxis(0.0f, glm::vec3{1.0f, 0.0f, 0.0f})
        };
        glm::vec3 scales[]{
                {100.0f, 100.0f, 100.0f},
                {100.0f, 100.0f, 100.0f}
        };

        trans_comp_manager->add(entities, positions, orientations, scales, 2);
    }

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

        // Draw the test entity
        static bool use_per_cam = true;
        static bool use_tex_ent = true;
        glm::mat4 camera_matrix = (use_per_cam) ?
                                  test_camera_per->getProjViewMatrix() :
                                  test_camera_ort->getProjViewMatrix();
        test_shader->use();
        glUniformMatrix4fv(pM_location, 1, GL_FALSE, &camera_matrix[0][0]);
        int indices[2]{};
        trans_comp_manager->lookup(entities, indices, 2);
        glUniformMatrix4fv(wM_location, 1, GL_FALSE, &(trans_comp_manager->data.matrix[(use_tex_ent) ? 0 : 1])[0][0]);
        model_comp_manager->lookup(entities, indices, 2);
        std::shared_ptr<model::Model> current_model(model_comp_manager->models[
                (use_tex_ent) ?
                      model_comp_manager->data.model[indices[0]] :
                      model_comp_manager->data.model[indices[1]]
        ]);
        current_model->draw();
        gl::ShaderProgram::unset();

        // Maintain components
        model_comp_manager->gc(test_manager);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Entity test
            {
                ImGui::Begin("Entity test");

                test_manager.showDebug();
                ImGui::Spacing();

                ImGui::Text("Textured entity index: %i", entities[0].index());
                ImGui::Text("Textured entity generation: %i", entities[0].generation());
                ImGui::Text("Untextured entity index: %i", entities[1].index());
                ImGui::Text("Untextured entity generation: %i", entities[1].generation());

                ImGui::End();
            }

            // Test controls
            {
                ImGui::Begin("Test controls");

                ImGui::Checkbox("Use perspective camera", &use_per_cam);
                ImGui::Checkbox("Use textured entity", &use_tex_ent);

                ImGui::Text("Current model:");
                current_model->showDebug();
                ImGui::Spacing();

                ImGui::Text("Test camera:");
                if (use_per_cam)
                    test_camera_per->showDebugControls();
                else
                    test_camera_ort->showDebugControls();
                ImGui::Spacing();

                ImGui::Text("World transformation control:");
                constexpr float delta = 50.0f;
                constexpr float deltaAngle = 20.0f;
                constexpr float deltaFactor = 1.2f;
                int index[1];
                trans_comp_manager->lookup(entities + ((use_tex_ent) ? 0 : 1), index, 1);
                if (ImGui::Button("-X")) {
                    glm::vec3 deltas[1]{ {-delta, 0.0f, 0.0f} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("+X")) {
                    glm::vec3 deltas[1]{ {delta, 0.0f, 0.0f} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("-Y")) {
                    glm::vec3 deltas[1]{ {0.0f, -delta, 0.0f} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("+Y")) {
                    glm::vec3 deltas[1]{ {0.0f, delta, 0.0f} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("-Z")) {
                    glm::vec3 deltas[1]{ {0.0f, 0.0f, -delta} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("+Z")) {
                    glm::vec3 deltas[1]{ {0.0f, 0.0f, delta} };
                    trans_comp_manager->translate(index, deltas, 1);
                }
                if (ImGui::Button("Anticlockwise")) {
                    glm::quat rots[1]{ glm::angleAxis(glm::radians(deltaAngle), glm::vec3{0.0f, 0.0f, 1.0f}) };
                    trans_comp_manager->rotate(index, rots, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Clockwise")) {
                    glm::quat rots[1]{ glm::angleAxis(-glm::radians(deltaAngle), glm::vec3{0.0f, 0.0f, 1.0f}) };
                    trans_comp_manager->rotate(index, rots, 1);
                }
                if (ImGui::Button("Scale Up")) {
                    glm::vec3 scales[1]{ {deltaFactor, deltaFactor, deltaFactor} };
                    trans_comp_manager->scale(index, scales, 1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Scale Down")) {
                    glm::vec3 scales[1]{ {1 / deltaFactor, 1 / deltaFactor, 1 / deltaFactor} };
                    trans_comp_manager->scale(index, scales, 1);
                }
                ImGui::Spacing();

                ImGui::Text("Position: %.1f, %.1f, %.1f", trans_comp_manager->data.position[index[0]].x,
                            trans_comp_manager->data.position[index[0]].y, trans_comp_manager->data.position[index[0]].z);
                ImGui::Text("Orientation: %.3f, %.3f, %.3f, %.3f", trans_comp_manager->data.orientation[index[0]].x,
                            trans_comp_manager->data.orientation[index[0]].y, trans_comp_manager->data.orientation[index[0]].z,
                            trans_comp_manager->data.orientation[index[0]].w);
                ImGui::Text("Scale: %.1f, %.1f, %.1f", trans_comp_manager->data.scale[index[0]].x,
                            trans_comp_manager->data.scale[index[0]].y, trans_comp_manager->data.scale[index[0]].z);

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
    test_shader.reset();

    c.disconnect();
    imgui_toggle.disconnect();
    imgui::clean_up();
    window::clean_up();
    window::terminate();
    os_log::clean_up();

    return 0;
}
