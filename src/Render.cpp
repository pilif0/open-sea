/** \file Render.cpp
 * Renderer implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Render.h>
#include <open-sea/Model.h>
#include <open-sea/ImGui.h>
#include <open-sea/Profiler.h>
#include <open-sea/GL.h>
#include <open-sea/Components.h>

#include <vector>

namespace open_sea::render {

    //--- start UntexturedRenderer implementation
    /**
     * \brief Construct a renderer
     *
     * Construct a renderer assigning it pointers to relevant component managers and initialising the shader.
     *
     * \param m Model component manager
     * \param t Transformation component manager
     */
    UntexturedRenderer::UntexturedRenderer(std::shared_ptr<ecs::ModelTable> m,
                                           std::shared_ptr<ecs::TransformationTable> t)
            : model_mgr(std::move(m)), transform_mgr(std::move(t)) {
        // Initialise the shader
        shader = std::make_unique<gl::ShaderProgram>();
        shader->attach_vertex_file("data/shaders/Test.vshader");
        shader->attach_fragment_file("data/shaders/Test.fshader");
        shader->link();
        shader->validate();
        p_mat_location = shader->get_uniform_location("projectionMatrix");
        w_mat_location = shader->get_uniform_location("worldMatrix");
    }

    /**
     * \brief Render entities through camera
     *
     * Render a set of entities through a camera.
     *
     * \param camera Camera
     * \param e Entities
     * \param count Number of entities
     */
    void UntexturedRenderer::render(std::shared_ptr<gl::Camera> camera, ecs::Entity *e, unsigned count) {
        profiler::push("Setup");
        // Use the shader and set the projection view matrix
        shader->use();
        glUniformMatrix4fv(p_mat_location, 1, GL_FALSE, &camera->get_proj_view_matrix()[0][0]);

        // Prepare info and index destination
        std::vector<RenderInfo> infos(count);
        std::vector<int> indices(count);
        profiler::pop();

        // Get world matrix pointers
        profiler::push("World Matrices");
        std::vector<ecs::TransformationTable::Data::Ptr> refs_tr(count);
        transform_mgr->table->get_reference(e, refs_tr.data(), count);
        auto i = refs_tr.begin();
        RenderInfo *r = infos.data();
        for (unsigned j = 0; j < count; j++, i++, r++) {
            if (i->matrix != nullptr) {
                r->matrix = &(*i->matrix)[0][0];
            }
        }
        profiler::pop();

        // Get the model information
        profiler::push("Models");

        std::vector<ecs::ModelTable::Data::Ptr> refs_mo(count);
        model_mgr->table->get_reference(e, refs_mo.data(), count);
        auto ref = refs_mo.data();
        r = infos.data();
        for (unsigned j = 0; j < count; j++, ref++, r++) {
            // Skip invalid indices
            if (ref->model != nullptr) {
                std::shared_ptr<model::Model> model = model_mgr->get_model(*(ref->model));
                r->vao = model->get_vertex_array();
                r->vertex_count = model->get_vertex_count();
            }
        }
        profiler::pop();

        // Render the information
        profiler::push("Render");
        r = infos.data();
        for (unsigned j = 0; j < count; j++, r++) {
            // Skip invalid entities
            if (r->matrix != nullptr) {
                glUniformMatrix4fv(w_mat_location, 1, GL_FALSE, r->matrix);
                glBindVertexArray(r->vao);
                glDrawElements(GL_TRIANGLES, r->vertex_count, GL_UNSIGNED_INT, nullptr);
            }
        }
        profiler::pop();

        // Reset state
        profiler::push("Reset");
        glBindVertexArray(0);
        shader->unset();
        profiler::pop();
    }

    /**
     * \brief Show ImGui debug information
     */
    // Note: ImGui ID stack interaction needed to separate the query modals of each component manager
    void UntexturedRenderer::show_debug() {
        if (ImGui::CollapsingHeader("Shader Program")) {
            shader->show_debug();
        }

        if (ImGui::CollapsingHeader("Transformation Component Manager")) {
            ImGui::PushID("transform_mgr");
            transform_mgr->show_debug();
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Model Component Manager")) {
            ImGui::PushID("model_mgr");
            model_mgr->show_debug();
            ImGui::PopID();
        }
    }
    //--- end UntexturedRenderer implementation
}
