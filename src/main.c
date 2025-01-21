#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <stdbool.h>
// #include <processthreadsapi.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "theeditor.h"

typedef struct {
    int atlas_id, subtexture_id;
    String message;
    GlyphInfo *msg_info;
    int *msg_indices;
    int width, height;
    SidePanel side_panel;
    BottomPanel bottom_panel;
} SceneData;

static SceneData sd = {0};

static void glfw_error_callback(int error, const char *description);
static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void glfw_cursor_pos_callback(GLFWwindow *window, double pos_x, double pos_y);
static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void glfw_window_refresh_callback(GLFWwindow *window);
static void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void glad_post_callback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...);
static void GLAPIENTRY
gl_error_callback(GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam );

static void render();

int main(int nargs, const char *argv[])
{
    GLFWwindow *window;
    int error;

    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialise glfw\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    if (!font_init())
    {
        fprintf(stderr, "Failed to initalise the freetype library\n");
        return EXIT_FAILURE;
    }

    // FT_Face face;
    // if (FT_New_Face(ft, "C:/Windows/Fonts/Consola.ttf", 0, &face))
    // {
    //     fprintf(stderr, "FreeType: Failed to load font consolas from C:/Windows/Fonts/Consola.ttf\n");
    //     return EXIT_FAILURE;
    // }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    window = glfwCreateWindow(2000, 1000, "GLFW Window", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    gladSetGLPostCallback(glad_post_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_pos_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    double now, last_frame = 0.0, delta;
    const double SPF_LIMIT = 1. / 60.;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    render_init();
    render_viewport((Rect){0, 0, width, height});

    layout_init();

    size_t len_listing;
    FileTreeItem *listing;
    ft_init(&len_listing, &listing);
    ft_expand(&len_listing, &listing, 0);
    printf("Expanded a total of %zu items in the directory:\n", len_listing);
    for (int i = 0; i < len_listing; i++)
    {
        for (int j = 0; j < listing[i].depth; j++)
            printf("    ");

		printf("%.*s\n", listing[i].len_name, listing[i].name);
    }
    ft_uninit(len_listing, listing);

    int aw, ah;
    aw = ah = 1024;
    uint8_t *atlas = calloc(1024 * 1024, sizeof (uint8_t));

    sd.message = STRLIT("Hello, OpenGL font rendering! 123");
    uint32_t *charcodes = calloc(sd.message.length, sizeof (char));
    sd.msg_indices = calloc(sd.message.length, sizeof *sd.msg_indices);

    int nchars = 0;
    for (int i = 0; i < sd.message.length; i++)
    {
        bool found = false;

        for (int j = 0; j < nchars; j++)
            if (charcodes[j] == sd.message.data[i])
            {
                sd.msg_indices[i] = j;
                found = true;
                break;
            }

        if (!found)
        {
            charcodes[nchars] = (uint32_t)sd.message.data[i];
            sd.msg_indices[i] = nchars;
            nchars++;
        }
    }

    FontId face = font_create_face("C:/Windows/Fonts/Times.ttf");
    GlyphInfo *glyphs = malloc(nchars * sizeof *glyphs);
    Rect *positions = malloc(nchars * sizeof *positions);

    if (!font_atlas_fill(aw, ah, atlas, nchars, charcodes, face, glyphs))
    {
        fprintf(stderr, "Failed to fill the font atlas.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < nchars; i++)
        memcpy(&positions[i], &glyphs[i].position, sizeof positions[i]);

    sd.atlas_id = render_init_texture_atlas(aw, ah, atlas, nchars, positions);
    sd.subtexture_id = 0;
    sd.msg_info = glyphs;
    sd.bottom_panel.height = sd.side_panel.width = 600;
    sd.bottom_panel.hidden = sd.side_panel.hidden = true;

    free(positions);
    free(atlas);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        now = glfwGetTime();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        sd.width = width;
        sd.height = height;

        delta = now - last_frame;

        if (delta >= SPF_LIMIT)
        {
            render();
            glfwSwapBuffers(window);
            last_frame = now;
            // printf("Frame time = %.1lfms\n", 1000. * delta);
        }
    }
    glfwDestroyWindow(window);

    glfwTerminate();
    return EXIT_SUCCESS;
}

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW error: %s\n", description);
}

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS)
        return;

    switch (key)
    {
    case GLFW_KEY_S:
        sd.side_panel.hidden = !sd.side_panel.hidden;
        break;
    case GLFW_KEY_B:
        sd.bottom_panel.hidden = !sd.bottom_panel.hidden;
        break;
    }
}

static void glfw_cursor_pos_callback(GLFWwindow *window, double pos_x, double pos_y)
{
    ui_mouse_position((float)pos_x, (float)pos_y);
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        switch (action)
        {
        case GLFW_PRESS:
            ui_mouse_button(true);
            break;
        case GLFW_RELEASE:
            ui_mouse_button(false);
            break;
        }
}

static void glfw_window_refresh_callback(GLFWwindow *window)
{
    // int width, height;
    // glfwGetFramebufferSize(window, &width, &height);
    // glViewport(0, 0, width, height);

    render();
    glfwSwapBuffers(window);
}

static void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    render_viewport((Rect){0, 0, width, height});
    sd.width = width;
    sd.height = height;
}

static void render()
{
    if (!sd.side_panel.hidden)
    {
        render_push_colored_quad((Rect){0, 0, sd.side_panel.width, sd.height}, COLOR_RGB(0x24221f));
    }

    if (!sd.bottom_panel.hidden)
    {
        render_push_colored_quad((Rect){
            sd.side_panel.hidden ? 0 : sd.side_panel.width,
            sd.height - sd.bottom_panel.height,
            sd.width - (sd.side_panel.hidden ? 0 : sd.side_panel.width),
            sd.bottom_panel.height,
        }, COLOR_RGB(0x101010));
    }

    Vec2 origin = {100, 300};

    for (int i = 0; i < sd.message.length; i++)
    {
        Vec2 pos;
        pos.x = origin.x + sd.msg_info[sd.msg_indices[i]].bearing.x;
        pos.y = origin.y + sd.msg_info[sd.msg_indices[i]].bearing.y;
        render_push_textured_quad(sd.atlas_id, sd.msg_indices[i], pos);
        origin.x += sd.msg_info[sd.msg_indices[i]].advance_x;
        origin.y += sd.msg_info[sd.msg_indices[i]].advance_y;
    }

    render_push_colored_quad((Rect){100, 300, 900, 100}, COLOR_RGB(0xff0000));

    render_draw();
}

static void glad_post_callback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...)
{
    // This crashes the program for some reason:

    // if (glGetError())
    // {
    //     fprintf(stderr, "GL call failed: %s()\n", name);
    //     exit(EXIT_FAILURE);
    // }
}
