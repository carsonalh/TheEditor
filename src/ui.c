#include "theeditor.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

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

static size_t container_stack_height = 0;
static FRect *container_stack = NULL;

static inline FRect frect_mask_compute(size_t nrects, const FRect *stack)
{
    Vec2 offset = {0};
    float greatest_left = -INFINITY, least_right = INFINITY;
    float greatest_top = -INFINITY, least_bottom = INFINITY;
    FRect r, t;

    for (int i = 0; i < nrects; i++)
    {
        r = stack[i];

        t = r;
        t.x += offset.x;
        t.y += offset.y;

        if (t.x > greatest_left) greatest_left = t.x;
        if (t.y > greatest_top) greatest_top = t.y;
        if (t.x + t.width < least_right) least_right = t.x + t.width;
        if (t.y + t.height < least_bottom) least_bottom = t.y + t.height;

        offset = v2_add(offset, (Vec2) {r.x, r.y});
    }

    r.x = greatest_left;
    r.y = greatest_top;
    r.width = fmaxf(least_right - greatest_left, 0);
    r.height = fmaxf(least_bottom - greatest_top, 0);

    return r;
}

/* Applies the container stack to convert these coordinates to global ones. */
static FRect frect_transformed(FRect rect)
{
    Vec2 x = {rect.x, rect.y};

    for (int i = 0; i < container_stack_height; i++)
        x = v2_add(x, (Vec2){container_stack[i].x, container_stack[i].y});

    rect.x = x.x;
    rect.y = x.y;

    return rect;
}

void ui_begin(void)
{
    if (!container_stack)
    {
        container_stack = malloc(MAX_UI_NEST_DEPTH * sizeof *container_stack);
    }

    hot = 0;

    memset(container_stack, 0, MAX_UI_NEST_DEPTH * sizeof *container_stack);
    container_stack[0] = (FRect){
        .x = 0, .y = 0,
        .width = window_width, .height = window_height,
    };
    container_stack_height = 1;
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

    assert(container_stack_height < MAX_UI_NEST_DEPTH);
    container_stack[container_stack_height] = (FRect){
        0, 0,
        500, window_height,
    };
    container_stack_height++;

    const FRect mask = frect_mask_compute(container_stack_height, container_stack);
    render_push_colored_quad((FRect){0, 0, window_width, window_height}, COLOR_RGB(0xA03030), -1, &mask);

    filetree_item_y_offset = 0;
}

void ui_filetree_end(void)
{
    container_stack_height--;
}

bool ui_filetree_item(const FileTreeItem *item, int id)
{
    float x, y, width, height;
    bool was_pressed = false;

    x = 0;
    y = filetree_item_y_offset;
    width = 500;
    height = 48;

    filetree_item_y_offset += 48;

    Vec2 offset = {
        (float)(48 * (item->depth - 1)),
        (float)(filetree_item_y_offset - 10),
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

	assert(1 <= container_stack_height && container_stack_height <= MAX_UI_NEST_DEPTH);

    const FRect mask = frect_mask_compute(container_stack_height, container_stack);

    if (active == id)
    {
        render_push_colored_quad(
            (FRect) { x, y, width, height },
            COLOR_RGB(0x808080),
            0,
			&mask
		);
    }
    else if (hot == id)
    {
        render_push_colored_quad(
            (FRect) { x, y, width, height },
            COLOR_RGB(0x404040),
            0,
            &mask
		);
    }

    for (int i = 0; i < item->len_name; i++)
    {
        Vec2 with_bearing = v2_add(offset, filetree_glyph_info[item->name[i] - ' '].bearing);

        render_push_textured_quad(
            filetree_atlas,
            (int)(item->name[i] - ' '),
            with_bearing,
            1,
			NULL
		);

        offset.x += filetree_glyph_info[item->name[i] - ' '].advance_x;
        offset.y += filetree_glyph_info[item->name[i] - ' '].advance_y;
    }

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
