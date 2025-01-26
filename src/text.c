#include "theeditor.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define NUM_FACES 32
static FT_Face faces[NUM_FACES] = {0};
static FT_Library ft = {0};

bool font_init(void)
{
    return !FT_Init_FreeType(&ft);
}

bool font_uninit(void)
{
    return !FT_Done_FreeType(ft);
}

FontId font_create_face(const char *path)
{
    for (int i = 0; i < NUM_FACES; i++)
    {
        if (!faces[i])
        {
            if (FT_New_Face(ft, path, 0, &faces[i]))
                return -1;

            return i;
        }
    }

    return -1;
}

void font_delete_face(FontId id)
{
    FT_Done_Face(faces[id]);
    faces[id] = NULL;
}

bool font_atlas_fill(size_t width, size_t height, uint8_t *atlas,
                     size_t ncodes, const uint32_t *char_codes,
                     FontId face_id,
                     GlyphInfo *out_glyphinfos,
                     FontAtlasFillState *fill_state)
{
    FontAtlasFillState local_state = {0};
    FT_Face face = faces[face_id];

    // TODO parameterise
    FT_Set_Pixel_Sizes(face, 0, 32);

    if (!fill_state)
    {
        fill_state = &local_state;
    }

    const FontAtlasFillState initial_state = *fill_state;

    for (int i = 0; i < ncodes; i++)
    {
        uint32_t c = char_codes[i];

        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            goto fill_failure;

        if (face->glyph->bitmap.width > width)
            goto fill_failure;

        if (fill_state->x + face->glyph->bitmap.width > width)
        {
            fill_state->y = fill_state->max_y;
            fill_state->x = 0;
        }

        if (fill_state->y + face->glyph->bitmap.rows > height)
            goto fill_failure;

        if (out_glyphinfos)
        {
            out_glyphinfos[i] = (GlyphInfo){
                .position = {
                    .x = fill_state->x,
                    .y = fill_state->y,
                    .width = face->glyph->bitmap.width,
                    .height = face->glyph->bitmap.rows,
                },
                .advance = {
                    .x = (float)face->glyph->advance.x / 64.0f,
                    .y = (float)face->glyph->advance.y / 64.0f,
                },
                .bearing = { (float)face->glyph->bitmap_left, -(float)face->glyph->bitmap_top },
            };
        }

        uint8_t *b = face->glyph->bitmap.buffer;
        for (int row = 0; row < (int)face->glyph->bitmap.rows; row++)
        {
            memcpy(&atlas[width * (fill_state->y + row) + fill_state->x],
                   &b[face->glyph->bitmap.width * row],
                   sizeof *b * face->glyph->bitmap.width);
        }

        fill_state->x += face->glyph->bitmap.width;

        if (fill_state->y + face->glyph->bitmap.rows > fill_state->max_y)
            fill_state->max_y = fill_state->y + face->glyph->bitmap.rows;
    }

    return true;

fill_failure:
    *fill_state = initial_state;
    return false;
}