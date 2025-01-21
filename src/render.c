#include "theeditor.h"
#include <glad/gl.h>
#include <string.h>
#include <stdio.h>

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
    float a = 1.0f;\n\
\n\
    if (vertex_UseTexture != 0)\n\
    {\n\
        a = texture(uFontAtlas, vertex_TexCoords).x;\n\
        color = vec3(a, a, a);\n\
    }\n\
\n\
    FragColor = vec4(color, a);\n\
}\n\
";

#define MAX_TEXTURE_UNITS 16
#define MAX_RENDERABLE_QUADS 2048

typedef struct {
    size_t width, height;
    size_t n_positions;
    const Rect *positions;
    unsigned int tex_id;
} TextureAtlas;

typedef struct {
    int u_projection, u_sampler;
    unsigned int vao, position_vbo, color_vbo, use_texture_vbo, tex_coords_vbo;
    unsigned int program;

    size_t window_width;
    size_t window_height;

    size_t n_tex_atlases;
    TextureAtlas tex_atlases[MAX_TEXTURE_UNITS];

    size_t n_quads;
    float buf_position[6 * MAX_RENDERABLE_QUADS][3];
    float buf_color[6 * MAX_RENDERABLE_QUADS][3];
    float buf_tex_coords[6 * MAX_RENDERABLE_QUADS][2];
    int buf_use_texture[6 * MAX_RENDERABLE_QUADS];
} RenderData;

static RenderData rd = {0};

void render_init(void)
{
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
        exit(EXIT_FAILURE);
    }

    rd.u_sampler = glGetUniformLocation(rd.program, "uFontAtlas");
    if (rd.u_sampler < 0)
    {
        fprintf(stderr, "Failure to find uniform variable uFontAtlas\n");
        exit(EXIT_FAILURE);
    }

    glGenVertexArrays(1, &rd.vao);
    glBindVertexArray(rd.vao);

    glGenBuffers(1, &rd.position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.position_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 6 * 3 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &rd.color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.color_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 6 * 3 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &rd.use_texture_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.use_texture_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 6 * sizeof (int), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &rd.tex_coords_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd.tex_coords_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 6 * 2 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void render_viewport(Rect pos)
{
    rd.window_width = pos.width;
    rd.window_height = pos.height;

    glViewport(pos.x, pos.y, pos.width, pos.height);
}

int render_init_texture_atlas(size_t width, size_t height, const uint8_t *buffer,
                              size_t nsubtextures, const Rect *subtexture_boxes)
{
    if (rd.n_tex_atlases >= MAX_TEXTURE_UNITS)
        return -1;

    TextureAtlas *ta = &rd.tex_atlases[rd.n_tex_atlases];

    glGenTextures(1, &ta->tex_id);
    glBindTexture(GL_TEXTURE_2D, ta->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUniform1i(rd.u_sampler, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ta->tex_id);

    // TODO make u_sampler a uniform buffer of texture ids, and add a texture id property to the shader
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, ta->tex_id);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
    // glUniform1i(rd.u_sampler, ta->tex_id);

    // TODO must remove
    // ((Rect*)subtexture_boxes)[0] = (Rect) {0, 0, width, height};

    ta->n_positions = nsubtextures;
    Rect *positions = malloc(nsubtextures * sizeof *subtexture_boxes);
    memcpy(positions, subtexture_boxes, nsubtextures * sizeof *subtexture_boxes);
    ta->positions = positions;
    ta->width = width;
    ta->height = height;

    return rd.n_tex_atlases++;
}

/** Renders at the location with the top left as the origin by default.  Removed after draw. */
void render_push_textured_quad(int atlasid, int subtexid, Vec2 pos)
{
    float x0, y0, x1, y1;
    float width, height;
    const Rect *subtexture;
    const TextureAtlas *atlas;

    atlas = &rd.tex_atlases[atlasid];
    subtexture = &rd.tex_atlases[atlasid].positions[subtexid];
    width = (float)subtexture->width;
    height = (float)subtexture->height;
    memcpy(&rd.buf_position[rd.n_quads * 6 + 0][0], (float[3]){pos.x, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 1][0], (float[3]){pos.x, pos.y + height}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 2][0], (float[3]){pos.x + width, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 3][0], (float[3]){pos.x, pos.y + height}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 4][0], (float[3]){pos.x + width, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 5][0], (float[3]){pos.x + width, pos.y + height}, sizeof (float[3]));

    memset(&rd.buf_use_texture[rd.n_quads * 6], 0xff, 6 * sizeof rd.buf_use_texture[0]);

    x0 = (float)subtexture->x / (float)atlas->width;
    y0 = (float)subtexture->y / (float)atlas->width;
    x1 = x0 + (float)subtexture->width / (float)atlas->width;
    y1 = y0 + (float)subtexture->height / (float)atlas->width;

    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 0][0], (float[2]){x0, y0}, sizeof (float[2]));
    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 1][0], (float[2]){x0, y1}, sizeof (float[2]));
    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 2][0], (float[2]){x1, y0}, sizeof (float[2]));
    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 3][0], (float[2]){x0, y1}, sizeof (float[2]));
    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 4][0], (float[2]){x1, y0}, sizeof (float[2]));
    memcpy(&rd.buf_tex_coords[rd.n_quads * 6 + 5][0], (float[2]){x1, y1}, sizeof (float[2]));

    rd.n_quads++;
}

void render_push_colored_quad(Rect pos, Color color)
{
    memcpy(&rd.buf_position[rd.n_quads * 6 + 0][0], (float[3]){pos.x, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 1][0], (float[3]){pos.x, pos.y + pos.height}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 2][0], (float[3]){pos.x + pos.width, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 3][0], (float[3]){pos.x, pos.y + pos.height}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 4][0], (float[3]){pos.x + pos.width, pos.y}, sizeof (float[3]));
    memcpy(&rd.buf_position[rd.n_quads * 6 + 5][0], (float[3]){pos.x + pos.width, pos.y + pos.height}, sizeof (float[3]));

    float rgb[3];
    for (int i = 0; i < 6; i++)
    {
        color_as_rgb(color, rgb);
        memcpy(&rd.buf_color[rd.n_quads * 6 + i], rgb, sizeof rd.buf_color[0]);
    }

    rd.n_quads++;
}

/** Draws the elements to the screen and and resets the per-frame queue. */
void render_draw(void)
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, rd.position_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd.n_quads * 6 * sizeof rd.buf_position[0], rd.buf_position);
    glBindBuffer(GL_ARRAY_BUFFER, rd.color_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd.n_quads * 6 * sizeof rd.buf_color[0], rd.buf_color);
    glBindBuffer(GL_ARRAY_BUFFER, rd.use_texture_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd.n_quads * 6 * sizeof rd.buf_use_texture[0], rd.buf_use_texture);
    glBindBuffer(GL_ARRAY_BUFFER, rd.tex_coords_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd.n_quads * 6 * sizeof rd.buf_tex_coords[0], rd.buf_tex_coords);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // for (int i = 0; i < 6 * rd.n_quads; i++)
    // {
    //     printf("uv %d: {%f, %f}\n",  i,
    //     rd.buf_tex_coords[i][0],
    //     rd.buf_tex_coords[i][1]
    //     );
    // }
    // printf("drawing %d quads\n", rd.n_quads);

    float left, right, top, bottom, nearplane, farplane;

    left = 0;
    right = rd.window_width;
    top = 0;
    bottom = rd.window_height;
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
    glDrawArrays(GL_TRIANGLES, 0, 6 * rd.n_quads);
    glBindVertexArray(0);
    glUseProgram(0);

    memset(rd.buf_position, 0, sizeof rd.buf_position);
    memset(rd.buf_color, 0, sizeof rd.buf_color);
    memset(rd.buf_tex_coords, 0, sizeof rd.buf_tex_coords);
    memset(rd.buf_use_texture, 0, sizeof rd.buf_use_texture);
    rd.n_quads = 0;
}

/** Cleans up the renderer when done. */
void render_uninit(void)
{
}
