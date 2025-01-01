#include "theeditor.h"

void color_as_rgb(Color c, float out[3])
{
    out[0] = (float)((c >> 24) & 0xff) / 255.f;
    out[1] = (float)((c >> 16) & 0xff) / 255.f;
    out[2] = (float)((c >> 8) & 0xff) / 255.f;
}

void color_as_rgba(Color c, float out[4])
{
    out[0] = (float)((c >> 24) & 0xff) / 255.f;
    out[1] = (float)((c >> 16) & 0xff) / 255.f;
    out[2] = (float)((c >> 8) & 0xff) / 255.f;
    out[3] = (float)(c & 0xff);
}