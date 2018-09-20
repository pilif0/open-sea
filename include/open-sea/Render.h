/** \file Render.h
 * Renderer systems using components
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_RENDER_H
#define OPEN_SEA_RENDER_H

#include <open-sea/Entity.h>
#include <open-sea/Components.h>
#include <open-sea/GL.h>
#include <open-sea/Debuggable.h>

#include <memory>
#include <utility>

//! Renderer systems and related functions
namespace open_sea::render {
    /**
     * \addtogroup Render
     * \brief Renderer systems and related functions
     *
     * Renderer systems using various ECS Components to render Entities.
     * A renderer is a system that uses components associated with an entity to render it in a certain way.
     *
     * @{
     */

    /** \class UntexturedRenderer
     * \brief Renderer using untextured models
     */
    class UntexturedRenderer : public debug::Debuggable {
        public:
            //! Model component manager
            std::shared_ptr<ecs::ModelComponent> modelMgr{};
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transformMgr{};
            //! Shader program
            std::unique_ptr<gl::ShaderProgram> shader{};
            //! Projection matrix uniform location
            GLint pMatLocation;
            //! World matrix uniform location
            GLint wMatLocation;
            UntexturedRenderer(std::shared_ptr<ecs::ModelComponent> m, std::shared_ptr<ecs::TransformationComponent> t);

            //! Render information for a single entity
            struct RenderInfo {
                //! Pointer to world matrix
                float *matrix = nullptr;
                //! Vertex Array ID
                GLuint vao = 0;
                //! Number of vertices to draw
                unsigned vertexCount = 0;
            };
            void render(std::shared_ptr<gl::Camera> camera, ecs::Entity* e, unsigned count);

            void showDebug() override;
    };

    /**
     * @}
     */
}

#endif //OPEN_SEA_RENDER_H
