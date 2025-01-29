#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
    int hot;
    int active;
    unsigned mouse_x;
    unsigned mouse_y;
    FileTree filetree;
    unsigned filetree_scroll_y;
    unsigned filetree_width;
} UiData;

typedef void RenderData;

RenderData *direct2d_init(HWND hwnd);
void direct2d_uninit(RenderData *render_data);
void direct2d_paint(RenderData *render_data, const UiData *ui_data);
void direct2d_resize(RenderData *rd, unsigned width, unsigned height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THE_EDITOR_H
