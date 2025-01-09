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
            // Param 3 (face index) is useful for fonts with more than one face in the file
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
                     GlyphInfo *out_glyphinfos)
{
    uint32_t c;
    size_t x = 0, y = 0, max_y = 0;
    FT_Face face;
    int i;

    face = faces[face_id];

    // TODO parameterise
    FT_Set_Pixel_Sizes(face, 0, 100);

    for (i = 0; i < ncodes; i++)
    {
        c = char_codes[i];

        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            return false;

        if (face->glyph->bitmap.width > width)
            return false;

        if (x + face->glyph->bitmap.width > width)
        {
            y = max_y;
            x = 0;
        }

        if (y + face->glyph->bitmap.rows > height)
            return false;

        if (out_glyphinfos)
            out_glyphinfos[i] = (GlyphInfo){
                .position = {
                    .x = x,
                    .y = y,
                    .width = face->glyph->bitmap.width,
                    .height = face->glyph->bitmap.rows,
                },
                .advance = (float)face->glyph->advance.x / 64.0f,
                .bearing = { face->glyph->bitmap_left, face->glyph->bitmap_top },
            };

        uint8_t *b = face->glyph->bitmap.buffer;
        for (int row = 0; row < face->glyph->bitmap.rows; row++)
        {
            memcpy(&atlas[width * (y + row) + x],
                   &b[face->glyph->bitmap.width * row],
                   sizeof *b * face->glyph->bitmap.width);
        }

        x += face->glyph->bitmap.width;

        if (y + face->glyph->bitmap.rows > max_y)
            max_y = y + face->glyph->bitmap.rows;
    }

    return true;
}