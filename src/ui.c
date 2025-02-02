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
    ui = NULL;
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

#define RESIZE_COMPONENTS_IF_FULL() do { \
    if (ui->components_len >= ui->components_cap) { \
        ui->components_cap *= 2; \
        ui->components = realloc(ui->components, ui->components_cap); \
    } } while (0)

void ui_box_container(const BoxContainer *box_container)
{
    RESIZE_COMPONENTS_IF_FULL();

    const int id = ui->components_len;
    ui->components[id] = (ComponentNode) {
        .depth = depth,
        .component = {
            .type = COMPONENT_TYPE_BOX_CONTAINER,
            .box_container = *box_container,
        },
    };
    ui->components_len++;

    return false;
}

void ui_linear_container(const LinearContainer *linear_container)
{
    RESIZE_COMPONENTS_IF_FULL();

    const int id = ui->components_len;
    ui->components[id] = (ComponentNode) {
        .depth = depth,
        .component = {
            .type = COMPONENT_TYPE_LINEAR_CONTAINER,
            .linear_container = *linear_container,
        },
    };
    ui->components_len++;

    return false;
}

void ui_rect(const Rect *rect)
{
    RESIZE_COMPONENTS_IF_FULL();

    const int id = (int)ui->components_len;
    ui->components[id] = (ComponentNode) {
        .depth = depth,
        .component = {
            .type = COMPONENT_TYPE_RECT,
            .rect = *rect,
        }
    };
    ui->components_len++;

    return false;
}
