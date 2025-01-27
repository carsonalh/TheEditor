#pragma once

#include "TheEditor.hpp"

#include <vector>

namespace ed
{

struct FileTreeItem
{
public:
    static constexpr size_t MaxNameLen = 256;
    static constexpr unsigned int Directory = 1 << 0;
    static constexpr unsigned int Open      = 1 << 1;
    static constexpr unsigned int Explored  = 1 << 2;

public:
    tstring name;
    unsigned int depth;
    unsigned int flags;
};

struct FileTree
{
    std::vector<FileTreeItem> items;

public:
    FileTree();
    ~FileTree() = default;

    /* Returns how many elements were expanded */
    void expand(size_t index);
    void collapse(size_t index);
};

}
