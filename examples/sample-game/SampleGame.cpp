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
#include <random>

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
    unsigned N = 1024;
    ecs::EntityManager test_manager;
    ecs::Entity entities[N];
    test_manager.create(entities, N);

    // Prepare and assign model
    std::unique_ptr<ecs::ModelComponent> model_comp_manager = std::make_unique<ecs::ModelComponent>();
    {
        std::shared_ptr<model::Model> model(model::UntexModel::fromFile("examples/sample-game/data/models/teapot.obj"));
        if (!model)
            return -1;
        model_comp_manager->modelToIndex(model);
        int models[N]{};   // modelIdx == 0, because it is the first model

        model_comp_manager->add(entities, models, N);
    }

    // Prepare and assign random transformations
    std::unique_ptr<ecs::TransformationComponent> trans_comp_manager = std::make_unique<ecs::TransformationComponent>();
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
        glm::vec3 positions[N]{};
        glm::quat orientations[N]{};
        glm::vec3 scales[N]{};

        // Randomise the data
        glm::vec3 *p = positions;
        glm::quat *o = orientations;
        glm::vec3 *s = scales;
        for (int i = 0; i < N; i++, p++, o++, s++) {
            *p = glm::vec3(posX(generator), posY(generator), posZ(generator));
            *o = glm::angleAxis(angle(generator), axis);
            float f = scale(generator);
            *s = glm::vec3(f, f, f);
        }

        os_log::log(lg, os_log::info, "Transformations generated");

        // Add the components
        trans_comp_manager->add(entities, positions, orientations, scales, N);
        os_log::log(lg, os_log::info, "Transformations set");
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

        // Draw the entities
        static bool use_per_cam = true;
        {
            // Set the shader and the projection-view matrix
            test_shader->use();
            glm::mat4 camera_matrix = (use_per_cam) ?
                                      test_camera_per->getProjViewMatrix() :
                                      test_camera_ort->getProjViewMatrix();
            glUniformMatrix4fv(pM_location, 1, GL_FALSE, &camera_matrix[0][0]);

            // Prepare render information
            struct RenderInfo {
                glm::mat4 matrix{1.0f};
                GLuint vao = 0;
                unsigned vertexCount = 0;
            } infos[N]{};

            // Get the world matrices of entities
            int indices[N]{};
            trans_comp_manager->lookup(entities, indices, N);
            int *i = indices;
            RenderInfo *r = infos;
            for (int j = 0; j < N; j++, i++, r++) {
                if (*i != -1)
                    r->matrix = trans_comp_manager->data.matrix[*i];
            }

            // Get the model indices
            model_comp_manager->lookup(entities, indices, N);
            i = indices;
            int models[N]{};
            int *m = models;
            int x = model_comp_manager->data.model[19];
            int y = model_comp_manager->data.model[20];
            int z = model_comp_manager->data.model[21];
            for (int j = 0; j < N; j++, i++, m++) {
                *m = model_comp_manager->data.model[*i];
            }

            // Get the model information
            m = models;
            r = infos;
            for (int j = 0; j < N; j++, m++, r++) {
                r->vao = model_comp_manager->models[*m]->getVertexArray();
                r->vertexCount = model_comp_manager->models[*m]->getVertexCount();
            }

            // Render the information
            r = infos;
            for (int j = 0; j < N; j++, r++) {
                glUniformMatrix4fv(wM_location, 1, GL_FALSE, &r->matrix[0][0]);
                glBindVertexArray(r->vao);
                glDrawElements(GL_TRIANGLES, r->vertexCount, GL_UNSIGNED_INT, nullptr);
            }

            // Reset vertex array and shader state
            glBindVertexArray(0);
            gl::ShaderProgram::unset();
        }

        // Maintain components
        model_comp_manager->gc(test_manager);
        trans_comp_manager->gc(test_manager);

        // ImGui debug GUI
        if (show_imgui) {
            // Prepare new frame
            imgui::new_frame();

            // Entity test
            {
                ImGui::Begin("Entity test");

                test_manager.showDebug();

                ImGui::End();
            }

            // Test controls
            {
                ImGui::Begin("Test controls");

                ImGui::Checkbox("Use perspective camera", &use_per_cam);

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
    test_shader.reset();

    c.disconnect();
    imgui_toggle.disconnect();
    imgui::clean_up();
    window::clean_up();
    window::terminate();
    os_log::clean_up();

    return 0;
}
