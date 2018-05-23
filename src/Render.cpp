/*
 * Renderer implementations
 */

#include <open-sea/Render.h>
#include <open-sea/Model.h>
#include <open-sea/ImGui.h>

#include <vector>

namespace open_sea::render {

    //--- start UntexturedRenderer implementation
    /**
     * \brief Construct a renderer
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
     * Render a set of entities through a camera.
     *
     * \param camera Camera
     * \param e Entities
     * \param count Number of entities
     */
    void UntexturedRenderer::render(std::shared_ptr<gl::Camera> camera, ecs::Entity *e, unsigned count) {
        // Use the shader and set the projection view matrix
        shader->use();
        glUniformMatrix4fv(pMatLocation, 1, GL_FALSE, &camera->getProjViewMatrix()[0][0]);

        // Prepare info and index destination
        std::vector<RenderInfo> infos(count);
        std::vector<int> indices(count);

        // Get world matrix pointers
        transformMgr->lookup(e, indices.data(), count);
        int *i = indices.data();
        RenderInfo *r = infos.data();
        for (int j = 0; j < count; j++, i++, r++) {
            if (*i != -1)
                r->matrix = &transformMgr->data.matrix[*i][0][0];
        }

        // Get the model information
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

        // Render the information
        r = infos.data();
        for (int j = 0; j < count; j++, r++) {
            // Skip invalid entities
            if (r->matrix != nullptr) {
                glUniformMatrix4fv(wMatLocation, 1, GL_FALSE, r->matrix);
                glBindVertexArray(r->vao);
                glDrawElements(GL_TRIANGLES, r->vertexCount, GL_UNSIGNED_INT, nullptr);
            }
        }

        // Reset state
        glBindVertexArray(0);
        shader->unset();
    }

    /**
     * \brief Show ImGui debug information
     */
    void UntexturedRenderer::showDebug() {
        if (ImGui::CollapsingHeader("Shader Program", ImGuiTreeNodeFlags_DefaultOpen)) {
            shader->showDebug();
        }
        //TODO
    }
    //--- end UntexturedRenderer implementation
}
