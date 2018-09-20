/** \file Render.cpp
 * Renderer implementations
 *
 * \author Filip Smola
 */

#include <open-sea/Render.h>
#include <open-sea/Model.h>
#include <open-sea/ImGui.h>
#include <open-sea/Profiler.h>

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
    UntexturedRenderer::UntexturedRenderer(std::shared_ptr<ecs::ModelComponent> m,
                                           std::shared_ptr<ecs::TransformationComponent> t)
            : modelMgr(std::move(m)), transformMgr(std::move(t)) {
        // Initialise the shader
        shader = std::make_unique<gl::ShaderProgram>();
        shader->attachVertexFile("data/shaders/Test.vshader");
        shader->attachFragmentFile("data/shaders/Test.fshader");
        shader->link();
        shader->validate();
        pMatLocation = shader->getUniformLocation("projectionMatrix");
        wMatLocation = shader->getUniformLocation("worldMatrix");
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
        glUniformMatrix4fv(pMatLocation, 1, GL_FALSE, &camera->getProjViewMatrix()[0][0]);

        // Prepare info and index destination
        std::vector<RenderInfo> infos(count);
        std::vector<int> indices(count);
        profiler::pop();

        // Get world matrix pointers
        profiler::push("World Matrices");
        transformMgr->lookup(e, indices.data(), count);
        int *i = indices.data();
        RenderInfo *r = infos.data();
        for (int j = 0; j < count; j++, i++, r++) {
            if (*i != -1) {
                r->matrix = &transformMgr->data.matrix[*i][0][0];
            }
        }
        profiler::pop();

        // Get the model information
        profiler::push("Models");
        modelMgr->lookup(e, indices.data(), count);
        i = indices.data();
        r = infos.data();
        for (int j = 0; j < count; j++, i++, r++) {
            // Skip invalid indices
            if (*i != -1) {
                std::shared_ptr<model::Model> model = modelMgr->getModel(modelMgr->data.model[*i]);
                r->vao = model->getVertexArray();
                r->vertexCount = model->getVertexCount();
            }
        }
        profiler::pop();

        // Render the information
        profiler::push("Render");
        r = infos.data();
        for (int j = 0; j < count; j++, r++) {
            // Skip invalid entities
            if (r->matrix != nullptr) {
                glUniformMatrix4fv(wMatLocation, 1, GL_FALSE, r->matrix);
                glBindVertexArray(r->vao);
                glDrawElements(GL_TRIANGLES, r->vertexCount, GL_UNSIGNED_INT, nullptr);
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
    void UntexturedRenderer::showDebug() {
        if (ImGui::CollapsingHeader("Shader Program")) {
            shader->showDebug();
        }

        if (ImGui::CollapsingHeader("Transformation Component Manager")) {
            ImGui::PushID("transformMgr");
            transformMgr->showDebug();
            ImGui::PopID();
        }

        if (ImGui::CollapsingHeader("Model Component Manager")) {
            ImGui::PushID("modelMgr");
            modelMgr->showDebug();
            ImGui::PopID();
        }
    }
    //--- end UntexturedRenderer implementation
}
