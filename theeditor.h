﻿#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;

typedef struct {
    int x, y;
    int width, height;
} Rect;

typedef struct {
    size_t length;
    char *data;
} String;

#define STRLIT(literal) ((String){.length = strlen(literal), .data = literal})

typedef uint32_t Color;

#define COLOR_RGB(x) (Color)((((Color)x) << 8) | 0xff)
#define COLOR_RGBA(x) ((Color)(x))

typedef struct {
    int width;
    Color background;
    bool hidden;
} SidePanel;

typedef struct {
    int height;
    Color background;
    bool hidden;
} BottomPanel;

typedef struct {
    Color background;
} MainPanel;

typedef struct {
    SidePanel side_panel;
    BottomPanel bottom_panel;
    MainPanel main_panel;
} Layout;

extern Layout layout;

void color_as_rgb(Color c, float out[3]);
void color_as_rgba(Color c, float out[4]);

void layout_init(void);

typedef struct {
    size_t index;
    Rect position;
    Vec2 bearing;
    float advance_x;
    float advance_y;
} GlyphInfo;

typedef int FontId;
bool font_init(void);
bool font_uninit(void);
/** Returns -1 if failed. */
FontId font_create_face(const char *path);
/** Returns true if there was no overflow. */
bool font_atlas_fill(size_t width, size_t height, uint8_t *atlas, size_t ncodes, const uint32_t *char_codes, FontId face, GlyphInfo *out_glyphinfos);

/** To be called once before all render functions */
void render_init(void);
/** Set the viewport of the renderer, just use this for resizing the window. */
void render_viewport(Rect pos);
/** To be called before rendering, persists per frame.  Currently only supporting 8 bit single channel textures. */
int render_init_texture_atlas(size_t width, size_t height, const uint8_t *buffer,
                              size_t nsubtextures, const Rect *subtexture_boxes);
// TODO let a user replace a texture atlas data with a new one
/** Renders at the location with the top left as the origin by default.  Removed after draw. */
void render_push_textured_quad(int atlasid, int subtexid, Vec2 pos);
/** Removed after draw. */
void render_push_colored_quad(Rect pos, Color color);
/** Draws the elements to the screen and and resets the per-frame queue. */
void render_draw(void);
/** Cleans up the renderer when done. */
void render_uninit(void);

void ui_begin(void);
void ui_end(void);
void ui_mouse_position(float x, float y);
void ui_mouse_button(bool down);

#define FILENAME_LEN 264

typedef enum {
    // Common flags
    FT_FILE      = 1 << 0,
    FT_DIRECTORY = 1 << 1,
    // To be set only if FT_DIRECTORY is set
    FT_OPEN      = 1 << 2,
    FT_EXPLORED  = 1 << 3,
} FileTreeItemFlags;

typedef struct {
    size_t len_name;
    const char *name;
    int depth;
    FileTreeItemFlags flags;
} FileTreeItem;

void ft_init(size_t *len_listing, FileTreeItem **listing);
void ft_uninit(size_t len_listing, FileTreeItem *listing);
void ft_expand(size_t *len_listing, FileTreeItem **listing, int index);
void ft_collapse(size_t len_listing, FileTreeItem* listing, int index);

#endif // THE_EDITOR_H
