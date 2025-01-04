#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int x, y;
    int width, height;
} Rect;

typedef struct {
    size_t length;
    char *data[];
} String;

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

typedef int FontId;
bool font_init(void);
bool font_uninit(void);
/** Returns -1 if failed. */
FontId font_create_face(const char *path);
/** Returns true if there was no overflow. */
bool font_atlas_fill(size_t width, size_t height, uint8_t *atlas, size_t ncodes, const uint32_t *char_codes, FontId face, Rect *out_positions);

#endif // THE_EDITOR_H
