#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum
{
    FILETREE_DIRECTORY = 1 << 0,
    FILETREE_OPEN      = 1 << 1,
    FILETREE_EXPLORED  = 1 << 2,
} FileTreeFlags;

typedef struct
{
    unsigned name_offset;
    unsigned name_len;
    FileTreeFlags flags;
    unsigned depth;
} FileTreeItem;

typedef struct
{
    unsigned strbuffer_len;
    unsigned strbuffer_cap;
    char *strbuffer;
    unsigned len;
    unsigned cap;
    FileTreeItem *items;
} FileTree;

void filetree_init(FileTree *filetree);
void filetree_uninit(FileTree *filetree);
void filetree_expand(FileTree *filetree, size_t index);
void filetree_collapse(FileTree *filetree, size_t index);

typedef struct
{
    unsigned depth;
    // RGBA encoded; but we ignore the alpha channel
    uint32_t color;
    uint64_t width;
    uint64_t height;
    uint64_t offset_x;
    uint64_t offset_y;
    // TODO some options to specify text
} UiComponent;

typedef struct
{
    int hot;
    int active;
    bool mouse_down;
    unsigned mouse_x;
    unsigned mouse_y;
    UiComponent *components;
    size_t components_len;
    size_t components_cap;
} UiData;

void ui_begin(UiData *ui);
void ui_end(void);
/* Increments the depth; use this before specifying the children of an element
 * (i.e. the last element which was called).
 */
void ui_depth_push(void);
/* Used to end the children of the last element.  See `ui_depth_push`.
 */
void ui_depth_pop(void);
bool ui_rect(int x, int y, int width, int height, uint32_t color);

typedef void RenderData;

RenderData *direct2d_init(HWND hwnd);
void direct2d_uninit(RenderData *render_data);
void direct2d_paint(RenderData *render_data, const UiData *ui_data);
void direct2d_resize(RenderData *rd, unsigned width, unsigned height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THE_EDITOR_H
