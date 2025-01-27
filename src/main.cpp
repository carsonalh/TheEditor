#ifdef UNICODE
#undef UNICODE
#endif // UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include <memory>
#include <iostream>

#include "FileTree.hpp"
#include "Application.hpp"

int main(void)
{
#ifndef _NDEBUG
    // Is this necessary with ASAN?
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif

    {
        Application app;

        if (!SUCCEEDED(app.Initialize()))
        {
            fprintf(stderr, "failed to initialise the application\n");
            exit(1);
        }

        app.RunMessageLoop();
    }
}
