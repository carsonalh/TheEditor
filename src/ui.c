#include "theeditor.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int id;
static int hot;
static int active = 0;
static float mouse_pos_x = 0;
static float mouse_pos_y = 0;
static bool mouse_button_down = false;
static float window_width;
static float window_height;

// container stack, relative to parent
// should be able to make a "mask" out of this stack, for child elements
// that should not be visible when they exceed the borders of their parent(s)

// IMPLEMENTATION GOALS
// * file tree (hideable)
// * text editor (mostly freeform ui, do not need much else than freeform display of text and rectangles)
// * console??? need to make an emulator which is a couple weeks for anything decent

// TODO: transform and clipping mask stack
// * 2D displacement transform stack (do not need rotation/scale)
// * Rect clipping mask stack  this is so parenting can exist and children can overflow
//   from their parents without it being a disaster, we take the intersection of the whole stack at any
//   point to render it; the intersection of two axis-aligned rectangles is itself a rectangle 

#define MAX_UI_NEST_DEPTH 8

static size_t mask_stack_height = 0;
static FRect *mask_stack = NULL;

static inline FRect frect_intersection(size_t nrects, const FRect *rects)
{
    float greatest_left = -INFINITY, least_right = INFINITY;
    float greatest_top = -INFINITY, least_bottom = INFINITY;
    FRect r;

    while (nrects--)
    {
        r = *rects;

        if (r.x > greatest_left) greatest_left = r.x;
        if (r.y > greatest_top) greatest_top = r.y;
        if (r.x + r.width < least_right) least_right = r.x + r.width;
        if (r.y + r.height < least_bottom) least_bottom = r.y + r.height;

        rects++;
    }

    r.x = greatest_left;
    r.y = greatest_top;
    r.width = fmaxf((int)(least_right - greatest_left), 0);
    r.height = fmaxf((int)(least_bottom - greatest_top), 0);

    return r;
}

// static size_t displacement_stack_height = 0;
// static Vec2 displacement_stack[MAX_UI_NEST_DEPTH];

void ui_begin(void)
{
    if (!mask_stack)
    {
        mask_stack = malloc(MAX_UI_NEST_DEPTH * sizeof *mask_stack);
    }

    hot = id = 0;

    memset(mask_stack, 0, MAX_UI_NEST_DEPTH * sizeof *mask_stack);
    mask_stack[0] = (FRect){
        .x = 0, .y = 0,
        .width = window_width, .height = window_height,
    };
    mask_stack_height = 1;
}

void ui_end(void)
{
    if (!mouse_button_down)
    {
        active = 0;
    }
    else if (!active)
    {
        // The user is clicking/dragging dead space; disallow other widgets to be selected
        active = -1;
    }

    render_draw();
}

static int filetree_atlas = -1;
static size_t filetree_listing_len;
static FileTreeItem *filetree_listing = NULL;
static StringArena filetree_arena;
static size_t filetree_glyph_info_len;
static GlyphInfo *filetree_glyph_info;
static float filetree_item_y_offset;

void ui_filetree_begin(void)
{
    if (filetree_atlas == -1)
    {
        FontId face = font_create_face("C:\\Windows\\Fonts\\segoeui.ttf");

        const char begin = ' ';
        const char end = '~';
        size_t width = 1024, height = 1024;
        uint8_t *atlas_data = malloc(width * height * sizeof *atlas_data);

        filetree_glyph_info_len = end - begin + 1;
        uint32_t *codes = malloc(filetree_glyph_info_len * sizeof *codes);
        for (int i = 0; i < filetree_glyph_info_len; i++)
            codes[i] = begin + i;
        filetree_glyph_info = malloc(filetree_glyph_info_len * sizeof *filetree_glyph_info);
        font_atlas_fill(width, height, atlas_data, filetree_glyph_info_len, codes, face, filetree_glyph_info);
        free(codes);
        font_delete_face(face);

        Rect *boxes = malloc(filetree_glyph_info_len * sizeof *boxes);
        for (int i = 0; i < filetree_glyph_info_len; i++)
            boxes[i] = filetree_glyph_info[i].position;
        filetree_atlas = render_init_texture_atlas(width, height, atlas_data, filetree_glyph_info_len, boxes);
        free(boxes);
    }

    if (!filetree_listing)
    {
		ft_init(&filetree_listing_len, &filetree_listing, &filetree_arena);
    }

    assert(mask_stack_height < MAX_UI_NEST_DEPTH);
    mask_stack[mask_stack_height] = (FRect){
        0, 0,
        500, window_height,
    };
    mask_stack_height++;

    filetree_item_y_offset = 0;
}

void ui_filetree_end(void)
{
    mask_stack_height--;
}

bool ui_filetree_item(const FileTreeItem *item, int id)
{
    float x, y, width, height;
    bool was_pressed = false;

    x = 0;
    y = filetree_item_y_offset;
    width = 500;
    height = 24;

    filetree_item_y_offset += 24;

    Vec2 offset = {
        48 * (item->depth - 1),
        filetree_item_y_offset += 24,
    };

    if (x <= mouse_pos_x && mouse_pos_x <= x + width &&
        y <= mouse_pos_y && mouse_pos_y <= y + height)
    {
        hot = id;

        if (!active && mouse_button_down)
        {
            active = id;
            was_pressed = true;
        }
    }

	assert(1 <= mask_stack_height && mask_stack_height <= MAX_UI_NEST_DEPTH);
    const FRect computed_mask = frect_intersection(mask_stack_height, mask_stack);

    if (active == id)
    {
        render_push_colored_quad(
            (Rect) { x, y, width, height },
            COLOR_RGB(0x808080),
			NULL
		);
    }
    else if (hot == id)
    {
        render_push_colored_quad(
            (Rect) { x, y, width, height },
            COLOR_RGB(0x404040),
            NULL
		);
    }

    for (int i = 0; i < item->len_name; i++)
    {
        Vec2 with_bearing = v2_add(offset, filetree_glyph_info[item->name[i] - ' '].bearing);

        render_push_textured_quad(
            filetree_atlas,
            (int)(item->name[i] - ' '),
            with_bearing,
			NULL
		);

        offset.x += filetree_glyph_info[item->name[i] - ' '].advance_x;
        offset.y += filetree_glyph_info[item->name[i] - ' '].advance_y;
    }

    return was_pressed;
}

bool ui_button(float x, float y, int id)
{
    bool was_pressed = false;
    float width = 200;
    float height = 100;

    if (x <= mouse_pos_x && mouse_pos_x <= x + width &&
        y <= mouse_pos_y && mouse_pos_y <= y + height)
    {
        hot = id;

        if (!active && mouse_button_down)
        {
            active = id;
            was_pressed = true;
        }
    }

    Color color_inactive = COLOR_RGB(0xff0000);
    Color color_hot = COLOR_RGB(0x00ff00);
    Color color_active = COLOR_RGB(0x0000ff);

    Color c = color_inactive;

    if (active == id)
    {
        c = color_active;
    }
    else if (hot == id)
    {
        c = color_hot;
    }

    render_push_colored_quad((Rect){x, y, width, height}, c, NULL);

    return was_pressed;
}

void ui_mouse_position(float x, float y)
{
    mouse_pos_x = x;
    mouse_pos_y = y;
}

void ui_mouse_button(bool down)
{
    mouse_button_down = down;
}

void ui_viewport(float width, float height)
{
    window_width = width;
    window_height = height;
}
