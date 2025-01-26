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
static Vec2 mouse_scroll = {0};
static float window_width;
static float window_height;

#define MAX_UI_NEST_DEPTH 8
#define SCROLL_SPEED 20.0

typedef struct
{
    FRect local_rect;
    // Vec2 local_scroll_offset;
} Container;

static size_t container_stack_height = 0;
static Container *container_stack = NULL;

typedef struct
{
    int id;
    Vec2 local_scroll_offset;
} ContainerState;

static size_t container_state_len = 0;
static size_t container_state_cap = 0;
static ContainerState *container_state = NULL;
static ContainerState *container_state_current = NULL;

void ui_container_begin(ContainerFlags flags, FRect where, int id)
{
    assert(container_stack_height < MAX_UI_NEST_DEPTH);

    Vec2 scroll_offset;
    int found = -1;

    for (size_t i = 0; i < container_state_len; i++)
    {
        if (container_state[i].id == id)
        {
            scroll_offset = container_state[i].local_scroll_offset;
            found = i;
            break;
        }
    }

    if (found < 0 && container_state_len >= container_state_cap)
    {
        container_state_cap = 2 * container_state_cap;
        if (container_state_cap < 8)
            container_state_cap = 8;
        container_state = realloc(container_state, container_state_cap * sizeof *container_state);
    }

    if (flags & C_FILLWIDTH)
    {
        where.x = 0.0f;
        where.width = container_stack[container_stack_height - 1].local_rect.width;
    }
    if (flags & C_FILLHEIGHT)
    {
        where.y = 0.0f;
        where.height = container_stack[container_stack_height - 1].local_rect.height;
    }

    if (found < 0)
    {
        container_state[container_state_len] = (ContainerState)
        {
            .id = id,
            .local_scroll_offset = {0},
        };
        container_state_len++;
        container_state_current = &container_state[container_state_len];
    }
    else
    {
        Vec2 scroll = mouse_scroll;
        if (!(flags & C_SCROLLX))
            scroll.x = 0.0f;
        if (!(flags & C_SCROLLY))
            scroll.y = 0.0f;

        Vec2 *offset = &container_state[found].local_scroll_offset;
        *offset = v2_add(*offset, scroll);
        container_state_current = &container_state[found];
    }

    container_stack[container_stack_height] = (Container)
    {
        .local_rect = where,
    };
    container_stack_height++;
}

void ui_container_end()
{
    container_stack_height--;
}

static bool frect_contains_point(FRect rect, Vec2 point)
{
    return rect.x <= point.x && point.x <= rect.x + rect.width
            && rect.y <= point.y && point.y <= rect.y + rect.height;
}

static inline FRect compute_mask(size_t ncontainers, const Container *containers)
{
    Vec2 offset = {0};
    float greatest_left = -INFINITY, least_right = INFINITY;
    float greatest_top = -INFINITY, least_bottom = INFINITY;
    FRect r, t;

    for (int i = 0; i < ncontainers; i++)
    {
        r = containers[i].local_rect;

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
        x = v2_add(x, (Vec2){container_stack[i].local_rect.x, container_stack[i].local_rect.y});

    if (container_state_current)
        x = v2_add(x, container_state_current->local_scroll_offset);

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
    container_stack[0].local_rect = (FRect){
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

static float treelist_item_offset_y;
static int treelist_atlas = -1;
static size_t treelist_glyph_info_len;
static GlyphInfo *treelist_glyph_info;

void ui_treelist_begin(void)
{
    // TODO generalise this to either a ui function or a system of its own; it is quick and dirty
    if (treelist_atlas < 0)
    {
        FontId face = font_create_face("C:\\Windows\\Fonts\\segoeui.ttf");

        const char begin = ' ';
        const char end = '~';
        size_t width = 1024, height = 1024;
        uint8_t *atlas_data = malloc(width * height * sizeof *atlas_data);

        treelist_glyph_info_len = end - begin + 1;
        uint32_t *codes = malloc(treelist_glyph_info_len * sizeof *codes);
        for (int i = 0; i < treelist_glyph_info_len; i++)
            codes[i] = begin + i;
        // allocate double the glyphs for regular and bold
        treelist_glyph_info = malloc(2 * treelist_glyph_info_len * sizeof *treelist_glyph_info);
        FontAtlasFillState fill_state = {0};
        font_atlas_fill(
            width, height, atlas_data,
            treelist_glyph_info_len, codes,
            face,
            treelist_glyph_info,
            &fill_state);
        font_delete_face(face);
        face = font_create_face("C:\\Windows\\Fonts\\segoeuib.ttf");
        font_atlas_fill(
            width, height, atlas_data,
            treelist_glyph_info_len, codes,
            face,
            treelist_glyph_info + treelist_glyph_info_len,
            &fill_state);
        font_delete_face(face);
        free(codes);

        // also need separate boxes for regular and bold
        Rect *boxes = malloc(2 * treelist_glyph_info_len * sizeof *boxes);
        for (int i = 0; i < 2 * treelist_glyph_info_len; i++)
            boxes[i] = treelist_glyph_info[i].position;
        treelist_atlas = render_init_texture_atlas(width, height, atlas_data, 2 * treelist_glyph_info_len, boxes);
        free(boxes);
    }

    treelist_item_offset_y = 0.0f;
}

void ui_treelist_end(void)
{
}

bool ui_treelist_item(int depth, String text, bool bold, int id)
{
    const float baseline_padding = 12;
    const float depth_distance = 24;
    const float width = container_stack[container_stack_height - 1].local_rect.width;
    const float height = 48;

    bool was_activated = false;

    FRect mask = compute_mask(container_stack_height, container_stack);
    FRect where = frect_transformed((FRect) {0, treelist_item_offset_y, width, height});
    Vec2 mouse_pos = (Vec2) {mouse_pos_x, mouse_pos_y};

    treelist_item_offset_y += height;

    if (frect_contains_point(where, mouse_pos) && frect_contains_point(mask, mouse_pos))
    {
        hot = id;

        if (!active && mouse_button_down)
        {
            active = id;
        }
        else if (active == id && !mouse_button_down)
        {
            was_activated = true;
        }
    }

    if (active == id)
    {
        render_push_colored_quad(where, COLOR_RGB(0x808080), 1, &mask);
    }
    else if (hot == id)
    {
        render_push_colored_quad(where, COLOR_RGB(0x404040), 1, &mask);
    }

    Vec2 offset = {
        where.x + baseline_padding + depth_distance * (depth - 1),
        where.y + where.height - 12,
    };

    GlyphInfo *glyph_buffer = treelist_glyph_info;
    if (bold)
        glyph_buffer += treelist_glyph_info_len;

    for (int i = 0; i < text.length; i++)
    {
        char c = text.data[i];
        Vec2 with_bearing = v2_add(offset, glyph_buffer[c - ' '].bearing);

        render_push_textured_quad(
            treelist_atlas,
            (int)(c - ' ') + (bold ? treelist_glyph_info_len : 0),
            with_bearing,
            1,
            &mask
		);

        offset = v2_add(offset, glyph_buffer[c - ' '].advance);
    }

    return was_activated;
}

bool ui_button(FRect where, int id)
{
    FRect mask = compute_mask(container_stack_height, container_stack);
    where = frect_transformed(where);
    Vec2 mouse_pos = (Vec2) {mouse_pos_x, mouse_pos_y};

    if (frect_contains_point(where, mouse_pos) && frect_contains_point(mask, mouse_pos))
    {
        hot = id;

        if (!active && mouse_button_down)
        {
            active = id;
        }
    }

    if (active == id)
    {
        render_push_colored_quad(where, COLOR_RGB(0x00FF00), 0, &mask);
    }
    else if (hot == id)
    {
        render_push_colored_quad(where, COLOR_RGB(0xFF0000), 0, &mask);
    }
    else
    {
        render_push_colored_quad(where, COLOR_RGB(0x0000FF), 0, &mask);
    }
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

void ui_scroll(Vec2 scroll)
{
    mouse_scroll = scroll;
}
