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
    int u_projection, u_sampler;
    unsigned int vao, position_vbo, color_vbo, use_texture_vbo, tex_coords_vbo;
    unsigned int program;
    unsigned int tex_id;
    GLFWwindow *window;
} RenderData;

RenderData rd = {0};

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

static const char *vert_src = "\
#version 330 core\n\
layout (location = 0) in vec3 position; // the position variable has attribute position 0\n\
layout (location = 1) in vec3 color;\n\
layout (location = 2) in int useTexture;\n\
layout (location = 3) in vec2 texCoords;\n\
\n\
uniform mat4 projection;\n\
\n\
out vec3 vertex_Color;\n\
flat out int vertex_UseTexture;\n\
out vec2 vertex_TexCoords;\n\
\n\
void main()\n\
{\n\
    vertex_Color = color;\n\
    vertex_UseTexture = useTexture;\n\
    vertex_TexCoords = texCoords;\n\
    gl_Position = projection * vec4(position, 1.0);\n\
}\n\
";

static const char *frag_src = "\
#version 330 core\n\
out vec4 FragColor;\n\
\n\
in vec3 vertex_Color;\n\
flat in int vertex_UseTexture;\n\
in vec2 vertex_TexCoords;\n\
uniform sampler2D uFontAtlas;\n\
\n\
void main()\n\
{\n\
    vec3 color = vertex_Color;\n\
\n\
    if (vertex_UseTexture != 0)\n\
    {\n\
        float r = texture(uFontAtlas, vertex_TexCoords).x;\n\
        color = vec3(r, r, r);\n\
    }\n\
\n\
    FragColor = vec4(color, 1.0);\n\
}\n\
";

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
    rd.window = window;
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    gladSetGLPostCallback(glad_post_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);
    glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

    double now, last_frame = 0.0, delta;
    const double SPF_LIMIT = 1. / 60.;

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

        // texture atlas display
        {300, 300, -0.5},
        {300, 900, -0.5},
        {900, 300, -0.5},
        {300, 900, -0.5},
        {900, 300, -0.5},
        {900, 900, -0.5},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof position, position, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    float side_color[3], bottom_color[3], main_color[3];
    color_as_rgb(layout.side_panel.background, side_color);
    color_as_rgb(layout.bottom_panel.background, bottom_color);
    color_as_rgb(layout.main_panel.background, main_color);
    float color[24][3] = {0};
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
    int use_texture[24] = {0};
    memset(&use_texture[18], 0xff, 6 * sizeof use_texture[0]);
    glGenBuffers(1, &rd.use_texture_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.use_texture_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof use_texture, (void*)use_texture, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, 0, NULL);
    float tex_coords[24][2] = {
        [18] = {0, 0},
        [19] = {0, 1},
        [20] = {1, 0},
        [21] = {0, 1},
        [22] = {1, 0},
        [23] = {1, 1},
    };
    glGenBuffers(1, &rd.tex_coords_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.tex_coords_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof tex_coords, tex_coords, GL_STATIC_DRAW);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);

    int aw, ah;
    aw = ah = 1024;
    uint8_t *atlas = calloc(1024 * 1024, sizeof (uint8_t));

    String message = STRLIT("Hello, OpenGL font rendering!");
    printf("the message is %s\nand is %d chars long.\n", message.data, message.length);
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

    printf("%d characters put to the atlas; they are:\n", nchars);

    for (int i = 0; i < nchars; i++)
        printf("%c\n", charcodes[i]);

    FontId face = font_create_face("C:/Windows/Fonts/Consola.ttf");
    Rect *positions = malloc(nchars * sizeof *positions);
    if (font_atlas_fill(aw, ah, atlas, nchars, charcodes, face, positions))
        printf("filled the atlas\n");
    else
        printf("failed to fill the atlas\n");

    glGenTextures(1, &rd.tex_id);
    glBindTexture(GL_TEXTURE_2D, rd.tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, aw, ah, 0, GL_RED, GL_UNSIGNED_BYTE, atlas);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(atlas);

    rd.u_sampler = glGetUniformLocation(rd.program, "uFontAtlas");
    if (rd.u_sampler < 0)
    {
        fprintf(stderr, "Failure to find uniform variable uFontAtlas\n");
        return EXIT_FAILURE;
    }
    glUniform1i(rd.u_sampler, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rd.tex_id);

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

        // texture atlas display
        {300, 300, -0.5},
        {300, 600, -0.5},
        {600, 300, -0.5},
        {300, 600, -0.5},
        {600, 300, -0.5},
        {600, 600, -0.5},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof position, position, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void render()
{
    glClearColor(1.0, 0.0, 0.0, 1.0);
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
    glDrawArrays(GL_TRIANGLES, 0, 24);
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
