#include "theeditor.h"

static int id = 0;
static int hot = 0;
static int active = 0;
static float mouse_pos_x = 0;
static float mouse_pos_y = 0;
static bool mouse_button_down = false;

// container stack, relative to parent
// should be able to make a "mask" out of this stack, for child elements
// that should not be visible when they exceed the borders of their parent(s)

// IMPLEMENTATION GOALS
// * file tree (hideable)
// * text editor (mostly freeform ui, do not need much else than freeform display of text and rectangles)
// * console??? need to make an emulator which is a couple weeks for anything decent

void ui_begin(void)
{
    id = 0;
}

void ui_end(void)
{
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
