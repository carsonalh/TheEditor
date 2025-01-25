#include "theeditor.h"
#include <glad/gl.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define MAX_TEXTURE_UNITS 16
#define MAX_RENDERABLE_QUADS 1024

#define STR_(X) #X
#define STR(X) STR_(X)

static const char *vert_src = "\
#version 330 core\n\
layout (location = 0) in vec4 position;\n\
layout (location = 1) in vec3 color;\n\
layout (location = 2) in int useTexture;\n\
layout (location = 3) in vec4 texCoords;\n\
layout (location = 4) in vec4 clipMask;\n\
layout (location = 5) in int textureId;\n\
layout (location = 6) in int z;\n\
\n\
out VS_OUT\n\
{\n\
    vec3 color;\n\
    flat int useTexture;\n\
    flat int textureId;\n\
    vec4 texCoords;\n\
    vec4 clipMask;\n\
    float z;\n\
} vs_out;\n\
\n\
void main()\n\
{\n\
    vs_out.color = color;\n\
    vs_out.clipMask = clipMask;\n\
    vs_out.useTexture = useTexture;\n\
    vs_out.textureId = textureId;\n\
    vs_out.texCoords = texCoords;\n\
    vs_out.z = float(z) / 128.01f;\n\
    gl_Position = position;\n\
}\n\
";

static const char *geom_src = "\
#version 330 core\n\
\n\
layout (points) in;\n\
layout (triangle_strip, max_vertices = 4) out;\n\
\n\
uniform mat4 projection;\n\
\n\
in VS_OUT\n\
{\n\
    vec3 color;\n\
    flat int useTexture;\n\
    flat int textureId;\n\
    vec4 texCoords;\n\
    vec4 clipMask;\n\
    float z;\n\
} gs_in[];\n\
\n\
out vec3 frag_Color;\n\
flat out int frag_UseTexture;\n\
flat out int frag_TextureId;\n\
out vec2 frag_TexCoords;\n\
out vec4 frag_ClipMask;\n\
out vec3 frag_Position;\n\
\n\
void main()\n\
{\n\
    float x, y, width, height;\n\
    float tex_x, tex_y, tex_width, tex_height;\n\
    x = gl_in[0].gl_Position.x;\n\
    y = gl_in[0].gl_Position.y;\n\
    width = gl_in[0].gl_Position.z;\n\
    height = gl_in[0].gl_Position.w;\n\
    tex_x = gs_in[0].texCoords.x;\n\
    tex_y = gs_in[0].texCoords.y;\n\
    tex_width = gs_in[0].texCoords.z;\n\
    tex_height = gs_in[0].texCoords.w;\n\
\n\
    frag_Color = gs_in[0].color;\n\
    frag_UseTexture = gs_in[0].useTexture;\n\
    frag_TextureId = gs_in[0].textureId;\n\
    frag_TexCoords = vec2(tex_x, tex_y);\n\
    frag_ClipMask = gs_in[0].clipMask;\n\
    frag_Position = vec3(x, y, gs_in[0].z);\n\
    gl_Position = projection * vec4(frag_Position, 1.0);\n\
    EmitVertex();\n\
\n\
    frag_Color = gs_in[0].color;\n\
    frag_UseTexture = gs_in[0].useTexture;\n\
    frag_TextureId = gs_in[0].textureId;\n\
    frag_TexCoords = vec2(tex_x + tex_width, tex_y);\n\
    frag_ClipMask = gs_in[0].clipMask;\n\
    frag_Position = vec3(x + width, y, gs_in[0].z);\n\
    gl_Position = projection * vec4(frag_Position, 1.0);\n\
    EmitVertex();\n\
\n\
    frag_Color = gs_in[0].color;\n\
    frag_UseTexture = gs_in[0].useTexture;\n\
    frag_TextureId = gs_in[0].textureId;\n\
    frag_TexCoords = vec2(tex_x, tex_y + tex_height);\n\
    frag_ClipMask = gs_in[0].clipMask;\n\
    frag_Position = vec3(x, y + height, gs_in[0].z);\n\
    gl_Position = projection * vec4(frag_Position, 1.0);\n\
    EmitVertex();\n\
\n\
    frag_Color = gs_in[0].color;\n\
    frag_UseTexture = gs_in[0].useTexture;\n\
    frag_TextureId = gs_in[0].textureId;\n\
    frag_TexCoords = vec2(tex_x + tex_width, tex_y + tex_height);\n\
    frag_ClipMask = gs_in[0].clipMask;\n\
    frag_Position = vec3(x + width, y + height, gs_in[0].z);\n\
    gl_Position = projection * vec4(frag_Position, 1.0);\n\
    EmitVertex();\n\
\n\
    EndPrimitive();\n\
}\n\
";

static const char *frag_src = "\
#version 330 core\n\
\n\
uniform sampler2D uFontAtlas[" STR(MAX_TEXTURE_UNITS) "];\n\
\n\
in vec3 frag_Color;\n\
flat in int frag_UseTexture;\n\
flat in int frag_TextureId;\n\
in vec2 frag_TexCoords;\n\
in vec4 frag_ClipMask;\n\
in vec3 frag_Position;\n\
\n\
out vec4 FragColor;\n\
\n\
void main()\n\
{\n\
    vec3 color = frag_Color;\n\
    float a = 1.0f;\n\
\n\
    if (frag_UseTexture != 0)\n\
    {\n\
        a = texture(uFontAtlas[frag_TextureId], frag_TexCoords).x;\n\
        color = vec3(a, a, a);\n\
    }\n\
\n\
    {\n\
		float x, y, width, height;\n\
\n\
		x = frag_ClipMask.x;\n\
		y = frag_ClipMask.y;\n\
		width = frag_ClipMask.z;\n\
		height = frag_ClipMask.w;\n\
\n\
        if (!(x < frag_Position.x && frag_Position.x < x + width &&\n\
            y < frag_Position.y && frag_Position.y < y + height))\n\
        {\n\
            a = 0.0f;\n\
        }\n\
        // color = vec3(1.0f, frag_Position.xy / 1000.0f);\n\
    }\n\
\n\
    FragColor = vec4(color, a);\n\
}\n\
";

typedef struct {
    size_t width, height;
    size_t n_positions;
    const Rect *positions;
    unsigned int tex_id;
} TextureAtlas;

typedef struct {
    int u_projection, u_sampler;
    unsigned int vao, position_vbo, color_vbo, use_texture_vbo, tex_coords_vbo, tex_index_vbo, clip_mask_vbo, z_vbo;
    unsigned int program;

    size_t window_width;
    size_t window_height;

    size_t n_tex_atlases;
    TextureAtlas tex_atlases[MAX_TEXTURE_UNITS];
    unsigned int tex_ids[MAX_TEXTURE_UNITS];

    size_t n_quads;
    FRect buf_position[MAX_RENDERABLE_QUADS];
    Vec3 buf_color[MAX_RENDERABLE_QUADS];
    FRect buf_tex_coords[MAX_RENDERABLE_QUADS];
    int8_t buf_use_texture[MAX_RENDERABLE_QUADS];
    FRect buf_clip_masks[MAX_RENDERABLE_QUADS];
    int8_t buf_tex_ids[MAX_RENDERABLE_QUADS];
    int8_t buf_z[MAX_RENDERABLE_QUADS];
} RenderData;

static RenderData *rd = NULL;

void render_init(void)
{
    assert(!rd && "render_init() can only be called once");

    unsigned int vert_shader, geom_shader, frag_shader;
    rd = calloc(1, sizeof *rd);
    rd->program = glCreateProgram();
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vert_shader, 1, &vert_src, NULL);
    glCompileShader(vert_shader);
    glShaderSource(geom_shader, 1, &geom_src, NULL);
    glCompileShader(geom_shader);
    glShaderSource(frag_shader, 1, &frag_src, NULL);
    glCompileShader(frag_shader);
    glAttachShader(rd->program, vert_shader);
    glAttachShader(rd->program, geom_shader);
    glAttachShader(rd->program, frag_shader);
    glLinkProgram(rd->program);
    glDeleteShader(vert_shader);
    glDeleteShader(geom_shader);
    glDeleteShader(frag_shader);

    char buffer[1024] = {0};
    glGetShaderInfoLog(geom_shader, 1024, NULL, buffer);
    // glGetProgramInfoLog(rd->program, 1024, NULL, buffer);
    printf("%s\n", buffer);

    rd->u_projection = glGetUniformLocation(rd->program, "projection");
    if (rd->u_projection < 0)
    {
        printf("Error getting uniform 'projection' from the program.\n");
        exit(EXIT_FAILURE);
    }

    rd->u_sampler = glGetUniformLocation(rd->program, "uFontAtlas");
    if (rd->u_sampler < 0)
    {
        fprintf(stderr, "Failure to find uniform variable uFontAtlas\n");
        exit(EXIT_FAILURE);
    }

    glGenVertexArrays(1, &rd->vao);
    glBindVertexArray(rd->vao);

    glGenBuffers(1, &rd->position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->position_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 3 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &rd->color_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->color_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 3 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &rd->use_texture_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->use_texture_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * sizeof (int8_t), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(2, 1, GL_BYTE, 0, NULL);
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &rd->tex_coords_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->tex_coords_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 4 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &rd->clip_mask_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->clip_mask_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 4 * sizeof (float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(4);

    glGenBuffers(1, &rd->tex_index_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->tex_index_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 1 * sizeof (int8_t), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(5, 1, GL_BYTE, 0, NULL);
    glEnableVertexAttribArray(5);

    glGenBuffers(1, &rd->z_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rd->z_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_RENDERABLE_QUADS * 1 * sizeof (int8_t), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribIPointer(6, 1, GL_BYTE, 0, NULL);
    glEnableVertexAttribArray(6);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void render_viewport(Rect pos)
{
    rd->window_width = pos.width;
    rd->window_height = pos.height;

    glViewport(pos.x, pos.y, pos.width, pos.height);
}

int render_init_texture_atlas(size_t width, size_t height, const uint8_t *buffer,
                              size_t nsubtextures, const Rect *subtexture_boxes)
{
    if (rd->n_tex_atlases >= MAX_TEXTURE_UNITS)
        return -1;

    TextureAtlas *ta = &rd->tex_atlases[rd->n_tex_atlases];

    glActiveTexture(GL_TEXTURE0 + ta->tex_id);
    glGenTextures(1, &ta->tex_id);
    glBindTexture(GL_TEXTURE_2D, ta->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (GLsizei)width, (GLsizei)height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);

    rd->tex_ids[rd->n_tex_atlases] = ta->tex_id;
    glUniform1iv(rd->u_sampler, MAX_TEXTURE_UNITS, (int*)rd->tex_ids);

    ta->n_positions = nsubtextures;
    Rect *positions = malloc(nsubtextures * sizeof *subtexture_boxes);
    memcpy(positions, subtexture_boxes, nsubtextures * sizeof *subtexture_boxes);
    ta->positions = positions;
    ta->width = width;
    ta->height = height;

    return (int)rd->n_tex_atlases++;
}

/** Renders at the location with the top left as the origin by default.  Removed after draw. */
void render_push_textured_quad(int atlasid, int subtexid, Vec2 pos, int8_t z, const FRect *clip_mask)
{
    assert(rd->n_quads < MAX_RENDERABLE_QUADS && "cannot render more quads the buffer size");

    const Rect *subtexture;
    const TextureAtlas *atlas;

    atlas = &rd->tex_atlases[atlasid];
    subtexture = &rd->tex_atlases[atlasid].positions[subtexid];

    FRect position = {
        .x = pos.x, .y = pos.y,
        .width = (float)subtexture->width,
        .height = (float)subtexture->height,
    };
    rd->buf_position[rd->n_quads] = position;

    rd->buf_use_texture[rd->n_quads] = 1;

    FRect texture_space_rect = {
        .x = (float)subtexture->x / (float)atlas->width,
        .y = (float)subtexture->y / (float)atlas->width,
        .width = (float)subtexture->width / (float)atlas->width,
        .height = (float)subtexture->height / (float)atlas->width,
    };
    rd->buf_tex_coords[rd->n_quads] = texture_space_rect;

    FRect clip;

    if (clip_mask)
    {
        clip = *clip_mask;
    }
    else
    {
        clip = (FRect)
        {
            .x = 0, .y = 0,
            .width = (float)rd->window_width,
            .height = (float)rd->window_height,
        };
    }

    rd->buf_clip_masks[rd->n_quads] = clip;
    rd->buf_z[rd->n_quads] = z;
    rd->buf_tex_ids[rd->n_quads] = atlasid;

    rd->n_quads++;
}

void render_push_colored_quad(FRect pos, Color color, int8_t z, const FRect *clip_mask)
{
    assert(rd->n_quads < MAX_RENDERABLE_QUADS && "cannot render more quads the buffer size");

    memcpy(&rd->buf_position[rd->n_quads], (float[4]){pos.x, pos.y, pos.width, pos.height}, sizeof rd->buf_position[0]);

    float rgb[3];
    color_as_rgb(color, rgb);
    memcpy(&rd->buf_color[rd->n_quads], rgb, sizeof rd->buf_color[0]);
    rd->buf_use_texture[rd->n_quads] = 0;

    float clip_x, clip_y, clip_width, clip_height;

    if (clip_mask)
    {
        clip_x = clip_mask->x;
        clip_y = clip_mask->y;
        clip_width = clip_mask->width;
        clip_height = clip_mask->height;
    }
    else
    {
        clip_x = clip_y = 0;
        clip_width = (float)rd->window_width;
        clip_height = (float)rd->window_height;
    }

    memcpy(&rd->buf_clip_masks[rd->n_quads], (float[4]){clip_x, clip_y, clip_width, clip_height}, sizeof rd->buf_clip_masks[0]);

    rd->buf_tex_ids[rd->n_quads] = 0;
    rd->buf_z[rd->n_quads] = z;

    rd->n_quads++;
}

/** Draws the elements to the screen and and resets the per-frame queue. */
void render_draw(void)
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, rd->position_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_position[0], rd->buf_position);
    glBindBuffer(GL_ARRAY_BUFFER, rd->color_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_color[0], rd->buf_color);
    glBindBuffer(GL_ARRAY_BUFFER, rd->use_texture_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_use_texture[0], rd->buf_use_texture);
    glBindBuffer(GL_ARRAY_BUFFER, rd->tex_coords_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_tex_coords[0], rd->buf_tex_coords);
    glBindBuffer(GL_ARRAY_BUFFER, rd->clip_mask_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_clip_masks[0], rd->buf_clip_masks);
    glBindBuffer(GL_ARRAY_BUFFER, rd->tex_index_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_tex_ids[0], rd->buf_tex_ids);
    glBindBuffer(GL_ARRAY_BUFFER, rd->z_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, rd->n_quads * sizeof rd->buf_z[0], rd->buf_z);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    float left, right, top, bottom, nearplane, farplane;

    left = 0;
    right = (float)rd->window_width;
    top = 0;
    bottom = (float)rd->window_height;
    nearplane = -1;
    farplane = 1;

    float camera_matrix[4][4] = {
        {2.f / (right - left), 0.f, 0.f, - (right + left) / (right - left)},
        {0.f, 2.f / (top - bottom), 0.f, - (top + bottom) / (top - bottom)},
        {0.f, 0.f, -2.f / (farplane - nearplane), - (farplane + nearplane) / (farplane - nearplane)},
        {0.f, 0.f, 0.f, 1.f},
    };

    glUseProgram(rd->program);
    glUniformMatrix4fv(rd->u_projection, 1, GL_TRUE, (float*)camera_matrix);
    glBindVertexArray(rd->vao);
    glDrawArrays(GL_POINTS, 0, (GLsizei)rd->n_quads);
    glBindVertexArray(0);
    glUseProgram(0);

    rd->n_quads = 0;
}

/** Cleans up the renderer when done. */
void render_uninit(void)
{
    free(rd);
}
