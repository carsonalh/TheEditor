#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <string_view>

namespace ed
{
using tchar = TCHAR;
using tstring = std::basic_string<tchar>;
using tstring_view = std::basic_string_view<tchar>;
}
