#pragma once

#include "TheEditor.hpp"

#include <vector>

namespace ed
{

/* Since string_views can be invalidated with realloc of a std::string, we double
   reference our string, so the 'slice' is as valid as a pointer to the string */
struct StringSlice
{
    const tstring* ptr;
    size_t start;
    size_t length;

public:
    StringSlice(const tstring& string, size_t start, size_t length);
    ~StringSlice() = default;

    operator tstring_view() const;
    tstring_view as_view() const;

#ifndef _NDEBUG
    inline bool valid() const
    {
        return start + length <= ptr->length();
    }
#endif
};

struct FileTreeItem
{
public:
    static constexpr size_t MaxNameLen = 256;
    static constexpr unsigned int Directory = 1 << 0;
    static constexpr unsigned int Open      = 1 << 1;
    static constexpr unsigned int Explored  = 1 << 2;

public:
    StringSlice name;
    unsigned int depth;
    unsigned int flags;
};

struct FileTree
{
    std::vector<FileTreeItem> items;
    tstring string_buffer;

public:
    FileTree();
    ~FileTree() = default;

    /* Returns how many elements were expanded */
    void expand(size_t index);
    void collapse(size_t index);
};

}
