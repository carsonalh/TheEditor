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
} SceneData;

static SceneData sd = {0};

static void glfw_error_callback(int error, const char *description);
static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
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
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    double now, last_frame = 0.0, delta;
    const double SPF_LIMIT = 1. / 60.;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    render_init();
    render_viewport((Rect){0, 0, width, height});

    layout_init();

    int aw, ah;
    aw = ah = 1024;
    uint8_t *atlas = calloc(1024 * 1024, sizeof (uint8_t));

    String message = STRLIT("Hello, OpenGL font rendering!");
    uint32_t *charcodes = calloc(message.length, sizeof (char));

    int nchars = 0;
    for (int i = 0; i < message.length; i++)
    {
        bool found = false;

        for (int j = 0; j < message.length; j++)
            if (charcodes[j] == message.data[i])
            {
                found = true;
                break;
            }

        if (!found)
            charcodes[nchars++] = message.data[i];
    }

    FontId face = font_create_face("C:/Windows/Fonts/Consola.ttf");
    GlyphInfo *glyphs = malloc(nchars * sizeof *glyphs);
    Rect *positions = malloc(nchars * sizeof *positions);

    if (font_atlas_fill(aw, ah, atlas, nchars, charcodes, face, glyphs))
        printf("filled the atlas\n");
    else
        printf("failed to fill the atlas\n");

    for (int i = 0; i < nchars; i++)
        memcpy(&positions[i], &glyphs[i].position, sizeof positions[i]);

    sd.atlas_id = render_init_texture_atlas(aw, ah, atlas, nchars, positions);
    sd.subtexture_id = 0;

    free(positions);
    free(atlas);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        now = glfwGetTime();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);


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
    printf("A key was pressed!\n");
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
}

static void render()
{
    render_push_colored_quad((Rect){100, 100, 600, 100}, COLOR_RGB(0x24221f));
    render_push_textured_quad(sd.atlas_id, sd.subtexture_id, (Vec2){100, 300});
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
