/** \file Render.h
 * Renderer systems using components
 *
 * \author Filip Smola
 */
#ifndef OPEN_SEA_RENDER_H
#define OPEN_SEA_RENDER_H

#include <glad/glad.h>

#include <open-sea/Debuggable.h>

#include <memory>
#include <utility>

// Forward declarations
namespace open_sea {
    namespace ecs {
        struct Entity;
        class EntityManager;
        class ModelComponent;
        class TransformationComponent;
    }

    namespace gl {
        class ShaderProgram;
        class Camera;
    }
}

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
            std::shared_ptr<ecs::ModelComponent> model_mgr{};
            //! Transformation component manager
            std::shared_ptr<ecs::TransformationComponent> transform_mgr{};
            //! Shader program
            std::unique_ptr<gl::ShaderProgram> shader{};
            //! Projection matrix uniform location
            GLint p_mat_location;
            //! World matrix uniform location
            GLint w_mat_location;
            UntexturedRenderer(std::shared_ptr<ecs::ModelComponent> m, std::shared_ptr<ecs::TransformationComponent> t);

            //! Render information for a single entity
            struct RenderInfo {
                //! Pointer to world matrix
                float *matrix = nullptr;
                //! Vertex Array ID
                GLuint vao = 0;
                //! Number of vertices to draw
                unsigned vertex_count = 0;
            };
            void render(std::shared_ptr<gl::Camera> camera, ecs::Entity* e, unsigned count);

            void show_debug() override;
    };

    /**
     * @}
     */
}

#endif //OPEN_SEA_RENDER_H
