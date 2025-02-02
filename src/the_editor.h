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

enum
{
    LENGTH_TYPE_FIXED,
    LENGTH_TYPE_RELATIVE,
} LengthType;

#define LENGTH_FIXED(px) ((uint64_t)(((uint64_t)LENGTH_TYPE_FIXED << 32) | (uint32_t)(px)))
#define LENGTH_RELATIVE(scale) ((uint64_t)(((uint64_t)LENGTH_TYPE_RELATIVE << 32) | *(uint32_t*)&(float){(scale)}))

enum
{
    // how many pixels from the beginning
    OFFSET_TYPE_FIXED_BEGIN = 0,
    // how many pixels from the end
    OFFSET_TYPE_FIXED_END,
    // parameter is a float; 0 is all the way at the beginning and 1 at the end
    OFFSET_TYPE_RELATIVE,
} OffsetType;

#define OFFSET_FROM_BEGIN(px) ((uint64_t)((uint64_t)OFFSET_TYPE_FIXED_BEGIN << 32) | (uint32_t)(px))
#define OFFSET_FROM_END(px) ((uint64_t)((uint64_t)OFFSET_TYPE_FIXED_END << 32) | (uint32_t)(px))
#define OFFSET_BEGIN() OFFSET_FROM_BEGIN(0)
#define OFFSET_END() OFFSET_FROM_END(0)
#define OFFSET_RELATIVE(scale) ((uint64_t)((uint64_t)OFFSET_TYPE_RELATIVE << 32) | *(uint32_t*)&(float){(scale)})
#define OFFSET_CENTER() OFFSET_RELATIVE(0.5f)

// All container/element structs must begin with these values, so we can position them
typedef struct
{
    uint64_t offset_x, offset_y;
    uint64_t length_x, length_y;
} RectSpec;

typedef struct
{
    uint64_t offset_x, offset_y;
    uint64_t length_x, length_y;
    uint32_t color;
} BoxContainer;

typedef enum
{
    LINEAR_CONTAINER_LEFT_TO_RIGHT,
    LINEAR_CONTAINER_TOP_TO_BOTTOM,
    LINEAR_CONTAINER_RIGHT_TO_LEFT,
    LINEAR_CONTAINER_BOTTOM_TO_TOP,
} LinearContainerDirection;

typedef struct
{
    uint64_t offset_x, offset_y;
    uint64_t length_x, length_y;
    unsigned item_length_px;
    LinearContainerDirection direction;
} LinearContainer;

typedef struct
{
    uint64_t offset_x, offset_y;
    uint64_t length_x, length_y;
    uint32_t color;
} Rect;

typedef enum
{
    COMPONENT_TYPE_RECT,
    COMPONENT_TYPE_BOX_CONTAINER,
    COMPONENT_TYPE_LINEAR_CONTAINER,
} ComponentType;

typedef struct
{
    ComponentType type;
    union {
        Rect rect;
        BoxContainer box_container;
        LinearContainer linear_container;
    };
} Component;

typedef struct
{
    int depth;
    Component component;
} ComponentNode;

typedef struct
{
    int hot;
    int active;
    bool mouse_down;
    unsigned mouse_x;
    unsigned mouse_y;
    ComponentNode *components;
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
void ui_linear_container(const LinearContainer *);
void ui_box_container(const BoxContainer *);
bool ui_rect(const Rect *);

typedef void RenderData;

RenderData *direct2d_init(HWND hwnd);
void direct2d_uninit(RenderData *render_data);
void direct2d_paint(RenderData *render_data, const UiData *ui_data);
void direct2d_resize(RenderData *rd, unsigned width, unsigned height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THE_EDITOR_H
