#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#include <d2d1.h>

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
    ID2D1Factory *factory;
    ID2D1HwndRenderTarget *render_target;
    HWND hwnd;
} RenderData;

void direct2d_init(RenderData *render_data);
void direct2d_uninit(RenderData *render_data);

void direct2d_paint(RenderData *render_data);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THE_EDITOR_H
