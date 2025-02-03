#ifndef THE_EDITOR_H
#define THE_EDITOR_H

#include <windows.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void RenderData;
RenderData *direct2d_init(HWND hwnd);
void direct2d_uninit(RenderData *render_data);
void direct2d_paint(RenderData *render_data);
void direct2d_resize(RenderData *rd, unsigned width, unsigned height);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THE_EDITOR_H
