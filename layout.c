#include "theeditor.h"

Layout layout;

void layout_init(void)
{
    layout.bottom_panel = (BottomPanel) {
        .background = RGB(0x24221f),
        .height = 400,
        .hidden = false,
    };

    layout.side_panel = (SidePanel) {
        .background = RGB(0x363330),
        .width = 450,
        .hidden = false,
    };

    layout.main_panel = (MainPanel) {
        .background = RGB(0x141414),
    };
}