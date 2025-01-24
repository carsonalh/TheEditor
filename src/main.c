#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
// #include <processthreadsapi.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "theeditor.h"

typedef struct {
    int atlas_id, subtexture_id;
    int width, height;
    SidePanel side_panel;
    BottomPanel bottom_panel;
    size_t ft_listing_len;
    FileTreeItem *ft_listing;
    StringArena ft_arena;
} SceneData;

static SceneData sd = {0};

static void glfw_error_callback(int error, const char *description);
static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void glfw_cursor_pos_callback(GLFWwindow *window, double pos_x, double pos_y);
static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
static void glfw_window_refresh_callback(GLFWwindow *window);
static void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height);
static void glad_post_callback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...);

static void render();

int main(int nargs, const char *argv[])
{
    GLFWwindow *window;

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

    ft_init(&sd.ft_listing_len, &sd.ft_listing, &sd.ft_arena);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        now = glfwGetTime();

        glfwGetFramebufferSize(window, &width, &height);
        render_viewport((Rect){0, 0, width, height});
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
    int id = 0;

    typedef enum {
        OP_NONE = 0,
        OP_EXPAND_FILE_TREE = 1,
        OP_COLLAPSE_FILE_TREE,
    } PostUiOperation;

    PostUiOperation op = OP_NONE;
    int op_arg = -1;

    ui_begin();
		ui_filetree_begin();
			for (int i = 0; i < sd.ft_listing_len; i++)
			{
				if (ui_filetree_item(&sd.ft_listing[i], ++id))
				{
                    if (sd.ft_listing[i].flags & FTI_FILE)
                        continue;

                    assert(!op && "only one item should ever be activated per render loop");

                    if (sd.ft_listing[i].flags & FTI_OPEN)
                    {
                        op = OP_COLLAPSE_FILE_TREE;
					}
                    else
                    {
                        op = OP_EXPAND_FILE_TREE;
                    }

                    op_arg = i;
				}

                if (!(sd.ft_listing[i].flags & FTI_OPEN))
                {
                    int parent_depth = sd.ft_listing[i].depth;

                    while (
                        i + 1 < sd.ft_listing_len
                        && sd.ft_listing[i + 1].depth > parent_depth)
                        i++;
                }
			}
		ui_filetree_end();
    ui_end();

    switch (op)
    {
    case OP_EXPAND_FILE_TREE:
        ft_expand(&sd.ft_listing_len, &sd.ft_listing, &sd.ft_arena, op_arg);
        break;
    case OP_COLLAPSE_FILE_TREE:
        ft_collapse(sd.ft_listing_len, sd.ft_listing, op_arg);
        break;
    default:
        break;
    }
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
