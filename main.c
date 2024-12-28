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

static uint32_t main_thread;
static uint32_t refresh_thread;

static void glfw_error_callback(int error, const char *description);
static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void glfw_window_refresh_callback(GLFWwindow *window);
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);  
    window = glfwCreateWindow(2000, 1000, "GLFW Window", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetWindowRefreshCallback(window, glfw_window_refresh_callback);

    double now, last_frame = 0.0, delta;
    const double SPF_LIMIT = 1. / 60.;

    const char *vert_src =
        "#version 330 core\n"
        "layout (location = 0) in vec3 position; // the position variable has attribute position 0\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 1.0);\n"
        "}\n";
    const char *frag_src =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(0.75, 0.25, 0.25, 1.0);\n"
        "}\n";
    unsigned int program, vert_shader, frag_shader;
    program = glCreateProgram();
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vert_shader, 1, &vert_src, NULL);
    glCompileShader(vert_shader);
    glShaderSource(frag_shader, 1, &frag_src, NULL);
    glCompileShader(frag_shader);
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    glUseProgram(program);

    int vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const float position[][3] = {
        {-.5, .5, 0.},
        {-.5, -.5, 0.},
        {.5, .5, 0.},
        {-.5, -.5, 0.},
        {.5, .5, 0.},
        {.5, -.5, 0.},
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof position, position, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);

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
            printf("Frame time = %.1lfms\n", 1000. * delta);
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
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    render();

    glfwSwapBuffers(window);
}

static void render()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
