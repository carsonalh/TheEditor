#include "the_editor.h"

#include <assert.h>

static int depth;
static UiData *ui;

void ui_begin(UiData *ui_data)
{
    depth = 1;
    ui = ui_data;
    ui->components_len = 0;
}

void ui_end(void)
{
    // TODO compute the rectangle sizes which need to be drawn???? or not???

#ifndef NDEBUG
    ui = NULL;
#endif
}

void ui_depth_push(void)
{
    assert(depth == ui->components[ui->components_len - 1].depth
            && "cannot push depth without a parent element");
    depth++;
}

void ui_depth_pop(void)
{
    depth--;
    assert(depth >= 1 && "cannot render below depth 1 (0 is a reserved root element)");
}

void ui_rect(int x, int y, int width, int height, uint32_t color)
{
    if (ui->components_len >= ui->components_cap) {
        ui->components_cap *= 2;
        ui->components = realloc(ui->components, ui->components_cap);
    }

    const int id = (int)ui->components_len;
    ui->components[ui->components_len] = (UiComponent) {
        .width = width,
        .height = height,
        .offset_x = x,
        .offset_y = y,
        .depth = depth,
        .color = color,
    };
    ui->components_len++;

    return false;
}
