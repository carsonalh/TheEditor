#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t Color;

#define RGB(x) (Color)((((Color)x) << 8) | 0xff)
#define RGBA(x) (Color)((((Color)x) << 8) | 0xff)

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

#endif // THE_EDITOR_H
