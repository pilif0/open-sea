/** \file ImGui.cpp
 * ImGui integration
 *
 * Based on the ImGui example at https://github.com/ocornut/imgui/tree/master/examples/opengl3_example
 *
 * \author Filip Smola
 */

#include <open-sea/ImGui.h>
#include <open-sea/Window.h>
#include <open-sea/Input.h>
#include <open-sea/Log.h>
#include <open-sea/Delta.h>
#include <open-sea/GL.h>
#include <open-sea/Profiler.h>

#include <optional>
#include <memory>

namespace open_sea::imgui {
    //! Module logger
    log::severity_logger lg = log::get_logger("ImGui");

    //! Different cursors used by ImGui
    GLFWcursor* cursors[ImGuiMouseCursor_Count_] = { 0 };
    //! Flags for mouse button presses
    bool mouse_just_pressed[3] = {false, false, false};

    // OpenGL data
    static GLuint       font_texture = 0;
    static std::unique_ptr<gl::ShaderProgram> shader_program;
    static int          attrib_location_tex = 0, attrib_location_proj_mtx = 0;
    static int          attrib_location_position = 0, attrib_location_uv = 0, attrib_location_color = 0;
    static unsigned int vbo = 0, elements = 0;

    /**
     * \brief Keyboard input callback
     *
     * \param key Relevant GLFW key code
     * \param scancode System-specific key code
     * \param action Relevant action
     * \param mods Bitfield of applied modifier keys
     */
    void key_callback(int key, int /*scancode*/, int action, int /*mods*/) {
        ImGuiIO& io = ImGui::GetIO();

        // Set key state
        if (action == input::press) {
            io.KeysDown[key] = true;
        }
        if (action == input::release) {
            io.KeysDown[key] = false;
        }

        // Update modifier flags
        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    }

    /**
     * \brief Mouse button input callback
     *
     * \param button Relevant GLFW button code
     * \param action Relevan action
     * \param mods Bitfield of applied modifier keys
     */
    void mouse_callback(int button, int action, int /*mods*/) {
        if (action == input::press) {
            // Set the corresponding flag
            switch (button) {
                case GLFW_MOUSE_BUTTON_1: mouse_just_pressed[0] = true; break;
                case GLFW_MOUSE_BUTTON_2: mouse_just_pressed[1] = true; break;
                case GLFW_MOUSE_BUTTON_3: mouse_just_pressed[2] = true; break;
                default: break;
            }
        }
    }

    /**
     * \brief Scroll input callback
     *
     * \param xoffset Horizontal scroll offset
     * \param yoffset Vertical scroll offset
     */
    void scroll_callback(double /*xoffset*/, double yoffset) {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel = (float) yoffset;
    }

    /**
     * \brief Character input callback
     *
     * \param codepoint Unicode codepoint of the character
     */
    void char_callback(unsigned int codepoint) {
        if (codepoint > 0 && codepoint < 0x10000) {
            ImGui::GetIO().AddInputCharacter((unsigned short) codepoint);
        }
    }

    /**
     * \brief Initialize ImGui integration
     * Map keys, set callbacks, create cursors
     */
    void init() {
        log::log(lg, log::info, "Initializing ImGui integration...");

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        // Map keys
        io.KeyMap[ImGuiKey_Tab]         = GLFW_KEY_TAB;
        io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_KEY_LEFT;
        io.KeyMap[ImGuiKey_RightArrow]  = GLFW_KEY_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow]     = GLFW_KEY_UP;
        io.KeyMap[ImGuiKey_DownArrow]   = GLFW_KEY_DOWN;
        io.KeyMap[ImGuiKey_PageUp]      = GLFW_KEY_PAGE_UP;
        io.KeyMap[ImGuiKey_PageDown]    = GLFW_KEY_PAGE_DOWN;
        io.KeyMap[ImGuiKey_Home]        = GLFW_KEY_HOME;
        io.KeyMap[ImGuiKey_End]         = GLFW_KEY_END;
        io.KeyMap[ImGuiKey_Insert]      = GLFW_KEY_INSERT;
        io.KeyMap[ImGuiKey_Delete]      = GLFW_KEY_DELETE;
        io.KeyMap[ImGuiKey_Backspace]   = GLFW_KEY_BACKSPACE;
        //io.KeyMap[ImGuiKey_Space]     = GLFW_KEY_SPACE;
        io.KeyMap[ImGuiKey_Enter]       = GLFW_KEY_ENTER;
        io.KeyMap[ImGuiKey_Escape]      = GLFW_KEY_ESCAPE;
        io.KeyMap[ImGuiKey_A]           = GLFW_KEY_A;
        io.KeyMap[ImGuiKey_C]           = GLFW_KEY_C;
        io.KeyMap[ImGuiKey_V]           = GLFW_KEY_V;
        io.KeyMap[ImGuiKey_X]           = GLFW_KEY_X;
        io.KeyMap[ImGuiKey_Y]           = GLFW_KEY_Y;
        io.KeyMap[ImGuiKey_Z]           = GLFW_KEY_Z;

        io.SetClipboardTextFn = [](void* /*u*/, const char* in){input::set_clipboard(in);};
        io.GetClipboardTextFn = [](void* /*u*/){return input::get_clipboard();};
#ifdef _WIN32
        io.ImeWindowHandle = glfwGetWin32Window(g_Window);
#endif

        // Load cursors
        cursors[ImGuiMouseCursor_Arrow] = ::glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        cursors[ImGuiMouseCursor_TextInput] = ::glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        //g_MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        cursors[ImGuiMouseCursor_ResizeNS] = ::glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        cursors[ImGuiMouseCursor_ResizeEW] = ::glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        cursors[ImGuiMouseCursor_ResizeNESW] = ::glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        cursors[ImGuiMouseCursor_ResizeNWSE] = ::glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

        // Set style
        ImGui::StyleColorsDark();

        log::log(lg, log::info, "ImGui integration initialized");
    }

    /**
     * \brief Create texture from the default font
     */
    void create_font_texture() {
        // Build texture atlas
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;

        // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely
        // to be compatible with user's existing shaders.
        // If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling
        // GetTexDataAsAlpha8() instead to save on GPU memory.
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Backup GL state
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

        // Upload texture to graphics system
        glGenTextures(1, &font_texture);
        glBindTexture(GL_TEXTURE_2D, font_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)font_texture;

        // Restore GL state
        glBindTexture(GL_TEXTURE_2D, last_texture);
    }

    /**
     * \brief Create OpenGL objects
     *
     * Create shader and font texture
     */
    void create_device_objects() {
        log::log(lg, log::info, "Creating OpenGL objects...");

        // Backup GL state
        GLint last_texture, last_array_buffer, last_vertex_array;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

        // Define shader
        shader_program = std::make_unique<gl::ShaderProgram>();
        shader_program->attach_vertex_file("data/shaders/ImGui.vshader");
        shader_program->attach_fragment_file("data/shaders/ImGui.fshader");
        shader_program->link();
        shader_program->validate();

        // Get the uniform locations
        attrib_location_tex = shader_program->get_uniform_location("Texture");
        attrib_location_proj_mtx = shader_program->get_uniform_location("ProjMtx");
        attrib_location_position = shader_program->get_attribute_location("Position");
        attrib_location_uv = shader_program->get_attribute_location("UV");
        attrib_location_color = shader_program->get_attribute_location("Color");

        // Generate buffers
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &elements);

        // Create font texture
        create_font_texture();
        log::log(lg, log::info, "Font texture created");

        // Restore GL state
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindVertexArray(last_vertex_array);

        log::log(lg, log::info, "OpenGL objects created");
    }

    /**
     * \brief Prepare for a new frame
     *
     * Update display size, delta time, input and cursor
     */
    void new_frame() {
        profiler::push("Device objects");
        if (!font_texture) {
            create_device_objects();
        }
        profiler::pop();

        profiler::push("Set up");
        ImGuiIO& io = ImGui::GetIO();

        // Setup display size (every frame to accommodate for window resizing)
        window::WindowProperties properties = window::current_properties();
        io.DisplaySize = ImVec2((float) properties.width, (float) properties.height);
        io.DisplayFramebufferScale = ImVec2(
                properties.width > 0 ? ((float) properties.fb_width / properties.width) : 0,
                properties.height > 0 ? ((float) properties.fb_height / properties.height) : 0
        );

        // Setup time step
        io.DeltaTime = static_cast<float>(open_sea::time::get_delta());
        profiler::pop();

        // Setup inputs
        profiler::push("Inputs");
        profiler::push("Cursor");
        profiler::push("Focus query");
        if (window::is_focused()) {
            profiler::pop();

            profiler::push("Mode query");
            if (input::get_cursor_mode() != input::cursor_mode::disabled) {
                profiler::pop();

                if (io.WantSetMousePos) {
                    // Set mouse position if requested by io.WantMoveMouse flag (used when io.NavMovesTrue is enabled by
                    // user and using directional navigation)
                    profiler::push("Set position");
                    glfwSetCursorPos(window::window, (double) io.MousePos.x, (double) io.MousePos.y);
                    profiler::pop();
                } else {
                    profiler::push("Get position");
                    ::glm::dvec2 cursor_pos = input::cursor_position();
                    io.MousePos = ImVec2((float) cursor_pos.x, (float) cursor_pos.y);
                    profiler::pop();
                }
            } else {
                profiler::pop();

                io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
            }
        } else {
            profiler::pop();

            io.MousePos = ImVec2(-FLT_MAX,-FLT_MAX);
        }
        profiler::pop();

        profiler::push("Mouse buttons");
        for (int i = 0; i < 3; i++) {
            // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release
            // events that are shorter than 1 frame.
            io.MouseDown[i] = mouse_just_pressed[i] || input::mouse_state(i) == input::press;
            mouse_just_pressed[i] = false;
        }
        profiler::pop();
        profiler::pop();

        // Update OS/hardware mouse cursor if imgui isn't drawing a software cursor
        profiler::push("Cursor icon");
        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
        if (input::get_cursor_mode() != input::cursor_mode::disabled) {
            // Skip this when the cursor is disabled

            if (io.MouseDrawCursor || cursor == ImGuiMouseCursor_None) {
                input::set_cursor_mode(input::cursor_mode::hidden);
            } else {
                glfwSetCursor(window::window, cursors[cursor] ? cursors[cursor] : cursors[ImGuiMouseCursor_Arrow]);
                input::set_cursor_mode(input::cursor_mode::normal);
            }
        }
        profiler::pop();

        // Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use
        // to dispatch inputs (or not) to your application.
        profiler::push("Internal");
        ImGui::NewFrame();
        profiler::pop();
    }

    /**
     * \brief Draw the GUI
     */
    void render() {
        // Get the draw data
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        // Get framebuffer size and scale coordinates accordingly
        ImGuiIO& io = ImGui::GetIO();
        int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0) {
            return;
        }
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

        // Backup GL state
        GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
        glActiveTexture(GL_TEXTURE0);
        GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
        GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
        GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
        GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
        GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
        GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
        GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
        GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
        GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
        GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
        GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
        GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
        GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Change viewport to have origin in top-left corner
        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

        // Set up orthographic projection matrix
        const float ortho_projection[4][4] =
                {
                        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
                        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
                        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
                        {-1.0f,                  1.0f,                   0.0f, 1.0f },
                };
        shader_program->use();
        glUniform1i(attrib_location_tex, 0);
        glUniformMatrix4fv(attrib_location_proj_mtx, 1, GL_FALSE, &ortho_projection[0][0]);
        glBindSampler(0, 0); // Rely on combined texture/sampler state.

        // Recreate the VAO every time
        // (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts, and we don't track
        // creation/deletion of windows so we don't have an obvious key to use to cache them.)
        GLuint vao_handle = 0;
        glGenVertexArrays(1, &vao_handle);
        glBindVertexArray(vao_handle);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(attrib_location_position);
        glEnableVertexAttribArray(attrib_location_uv);
        glEnableVertexAttribArray(attrib_location_color);
        glVertexAttribPointer(attrib_location_position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(attrib_location_uv, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
        glVertexAttribPointer(attrib_location_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

        // Draw
        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const ImDrawIdx* idx_buffer_offset = 0;

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback) {
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                    glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
                }
                idx_buffer_offset += pcmd->ElemCount;
            }
        }
        glDeleteVertexArrays(1, &vao_handle);

        // Restore modified GL state
        glUseProgram(last_program);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindSampler(0, last_sampler);
        glActiveTexture(last_active_texture);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
        glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
        glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
        if (last_enable_blend) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        if (last_enable_cull_face) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
        if (last_enable_depth_test) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        if (last_enable_scissor_test) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    }


    /**
     * \brief Clean up after ImGui
     *
     * Destroy cursors and OpenGL objects used by ImGui
     */
    void clean_up() {
        log::log(lg, log::info, "Cleaning up...");

        // Destroy GLFW mouse cursors
        for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_Count_; cursor_n++) {
            ::glfwDestroyCursor(cursors[cursor_n]);
        }
        memset(cursors, 0, sizeof(cursors));

        // Destroy OpenGL objects
        if (vbo) {
            glDeleteBuffers(1, &vbo);
        }
        if (elements) {
            glDeleteBuffers(1, &elements);
        }
        vbo = elements = 0;

        shader_program.reset();

        if (font_texture) {
            glDeleteTextures(1, &font_texture);
            ImGui::GetIO().Fonts->TexID = 0;
            font_texture = 0;
        }

        log::log(lg, log::info, "Cleaned up");
    }

}
