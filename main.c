#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <stdbool.h>
#include <processthreadsapi.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "theeditor.h"

typedef struct {
    int u_projection;
    unsigned int vao, position_vbo, color_vbo;
    unsigned int program;
    GLFWwindow *window;
} RenderData;

RenderData rd = {0};

static uint32_t main_thread;
static uint32_t refresh_thread;

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

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        fprintf(stderr, "Failed to initialise freetype library.\n");
        return EXIT_FAILURE;
    }

    FT_Face face;
    if (FT_New_Face(ft, "C:/Windows/Fonts/Consola.ttf", 0, &face))
    {
        fprintf(stderr, "FreeType: Failed to load font consolas from C:/Windows/Fonts/Consola.ttf\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);  
    window = glfwCreateWindow(2000, 1000, "GLFW Window", NULL, NULL);
    rd.window = window;
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    gladSetGLPostCallback(glad_post_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    double now, last_frame = 0.0, delta;
    const double SPF_LIMIT = 1. / 60.;

    const char *vert_src =
        "#version 330 core\n"
        "layout (location = 0) in vec3 position; // the position variable has attribute position 0\n"
        "layout (location = 1) in vec3 color;\n"
        "uniform mat4 projection;\n"
        "\n"
        "out vec3 vertex_Color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vertex_Color = color;\n"
        "    gl_Position = projection * vec4(position, 1.0);\n"
        "}\n";
    const char *frag_src =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "in vec3 vertex_Color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(vertex_Color, 1.0);\n"
        "}\n";
    unsigned int vert_shader, frag_shader;
    rd.program = glCreateProgram();
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vert_shader, 1, &vert_src, NULL);
    glCompileShader(vert_shader);
    glShaderSource(frag_shader, 1, &frag_src, NULL);
    glCompileShader(frag_shader);
    glAttachShader(rd.program, vert_shader);
    glAttachShader(rd.program, frag_shader);
    glLinkProgram(rd.program);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    rd.u_projection = glGetUniformLocation(rd.program, "projection");
    if (rd.u_projection < 0)
    {
        printf("Error getting uniform 'projection' from the program.\n");
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    layout_init();

    glGenVertexArrays(1, &rd.vao);
    glBindVertexArray(rd.vao);
    glGenBuffers(1, &rd.position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.position_vbo);
    const float position[][3] = {
        // side panel
        {0, 0, 0},
        {0, (float)height, 0},
        {(float)layout.side_panel.width, 0, 0},
        {0, (float)height, 0},
        {(float)layout.side_panel.width, 0, 0},
        {(float)layout.side_panel.width, (float)height, 0},

        // bottom panel
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {layout.side_panel.width, height, 0},
        {width, height - layout.bottom_panel.height, 0},
        {layout.side_panel.width, height, 0},
        {width, height - layout.bottom_panel.height, 0},
        {width, height, 0},

        // main panel
        {layout.side_panel.width, 0, 0},
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {width, 0, 0},
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {width, 0, 0},
        {width, height - layout.bottom_panel.height, 0},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof position, position, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    float side_color[3], bottom_color[3], main_color[3];
    color_as_rgb(layout.side_panel.background, side_color);
    color_as_rgb(layout.bottom_panel.background, bottom_color);
    color_as_rgb(layout.main_panel.background, main_color);
    float color[18][3];
    for (int i = 0; i < 6; i++)
        memcpy(color[i], side_color, sizeof color[i]);
    for (int i = 6; i < 12; i++)
        memcpy(color[i], bottom_color, sizeof color[i]);
    for (int i = 12; i < 18; i++)
        memcpy(color[i], main_color, sizeof color[i]);
    glGenBuffers(1, &rd.color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.color_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof color, (float*)color, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

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
    glViewport(0, 0, width, height);

    glBindBuffer(GL_ARRAY_BUFFER, rd.position_vbo);
    const float position[][3] = {
        // side panel
        {0, 0, 0},
        {0, (float)height, 0},
        {(float)layout.side_panel.width, 0, 0},
        {0, (float)height, 0},
        {(float)layout.side_panel.width, 0, 0},
        {(float)layout.side_panel.width, (float)height, 0},

        // bottom panel
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {layout.side_panel.width, height, 0},
        {width, height - layout.bottom_panel.height, 0},
        {layout.side_panel.width, height, 0},
        {width, height - layout.bottom_panel.height, 0},
        {width, height, 0},

        // main panel
        {layout.side_panel.width, 0, 0},
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {width, 0, 0},
        {layout.side_panel.width, height - layout.bottom_panel.height, 0},
        {width, 0, 0},
        {width, height - layout.bottom_panel.height, 0},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof position, position, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void render()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    int width, height;
    glfwGetFramebufferSize(rd.window, &width, &height);

    float left, right, top, bottom, nearplane, farplane;

    left = 0;
    right = width;
    top = 0;
    bottom = height;
    nearplane = -1;
    farplane = 1;

    float camera_matrix[4][4] = {
        {2.f / (right - left), 0.f, 0.f, - (right + left) / (right - left)},
        {0.f, 2.f / (top - bottom), 0.f, - (top + bottom) / (top - bottom)},
        {0.f, 0.f, -2.f / (farplane - nearplane), - (farplane + nearplane) / (farplane - nearplane)},
        {0.f, 0.f, 0.f, 1.f},
    };

    glUseProgram(rd.program);
    glUniformMatrix4fv(rd.u_projection, 1, GL_TRUE, (float*)camera_matrix);
    glBindVertexArray(rd.vao);
    glDrawArrays(GL_TRIANGLES, 0, 18);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void glad_post_callback(void *ret, const char *name, GLADapiproc apiproc, int len_args, ...)
{
    // This crashes the program for some reason:

    // if (glGetError() != GL_NO_ERROR)
    // {
    //     fprintf(stderr, "GL call failed: %s()\n", name);
    //     exit(EXIT_FAILURE);
    // }
}
