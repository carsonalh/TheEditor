#include "TheEditor.hpp"
#include "FileTree.hpp"

#include <tchar.h>

#include <iterator>
#include <cassert>
#include <limits>
#include <algorithm>

using namespace ed;

static constexpr size_t FileTreeInitialSize = 256;
static constexpr size_t StringBufferInitialSize = 2048;
static constexpr size_t SubListingBufferInitialSize = 64;

FileTree::FileTree()
    : items()
    , string_buffer()
{
    items.reserve(FileTreeInitialSize);
    string_buffer.reserve(StringBufferInitialSize);

    WIN32_FIND_DATA ffd;
    HANDLE hfind = nullptr;

    const tstring search = TEXT(".\\*");
    hfind = FindFirstFile(search.c_str(), &ffd);

    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (_tcsncmp(ffd.cFileName, _T("."), std::size(ffd.cFileName))
                && _tcsncmp(ffd.cFileName, _T(".."), std::size(ffd.cFileName)))
            {
                unsigned int flags = 0;

                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    flags |= FileTreeItem::Directory;

                size_t filename_length = _tcsnlen(ffd.cFileName, std::size(ffd.cFileName));

                auto view_begin_index = string_buffer.length();

                string_buffer.append(ffd.cFileName, 0, filename_length);

                items.push_back(FileTreeItem {
                    .name = StringSlice(string_buffer, view_begin_index, filename_length),
                    .depth = 1,
                    .flags = flags,
                });
            }
        }
        while (FindNextFile(hfind, &ffd));
    }
}

void FileTree::expand(size_t index)
{
    assert(index < items.size());
    assert(items[index].flags & FileTreeItem::Directory);

    auto& to_expand = items[index];

    if (to_expand.flags & FileTreeItem::Explored)
    {
        to_expand.flags |= FileTreeItem::Open;
        return;
    }

    std::vector<tstring_view> path_items;
    path_items.reserve(to_expand.depth);

    int least_depth = std::numeric_limits<int>::max();
    size_t search_path_length = strlen(".\\");
    for (int i = index; i >= 0; i--)
    {
        if (items[i].depth < least_depth)
        {
            path_items.push_back(items[i].name);
            search_path_length += items[i].name.length + strlen("\\");
            least_depth = items[i].depth;
        }
    }
    search_path_length += strlen("*");

    tstring search_path{};
    search_path.reserve(search_path_length);
    search_path.append(_T(".\\"));
    for (auto it = path_items.rbegin(); it != path_items.rend(); it++)
    {
        search_path.append(*it);
        search_path.append(_T("\\"));
    }
    search_path.append(_T("*"));

    WIN32_FIND_DATA ffd;
    HANDLE hfind = nullptr;
    size_t len_sub_listing = 0;

    std::vector<FileTreeItem> sub_listing_buffer;
    sub_listing_buffer.reserve(SubListingBufferInitialSize);

    {
        hfind = FindFirstFile(search_path.c_str(), &ffd);

        assert(hfind != INVALID_HANDLE_VALUE);

        do
        {
            if (_tcsncmp(ffd.cFileName, _T("."), std::size(ffd.cFileName))
                && _tcsncmp(ffd.cFileName, _T(".."), std::size(ffd.cFileName)))
            {
                unsigned int flags = 0;

                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    flags |= FileTreeItem::Directory;

                size_t start = string_buffer.length();
                size_t item_length = _tcsnlen(ffd.cFileName, std::size(ffd.cFileName));
                string_buffer.append(ffd.cFileName, item_length);
                sub_listing_buffer.push_back(FileTreeItem {
                    .name = StringSlice(string_buffer, start, item_length),
                    .depth = to_expand.depth + 1,
                    .flags = flags,
                });
            }
        }
        while (FindNextFile(hfind, &ffd));
    }

    items.insert(
        items.begin() + index + 1,
        sub_listing_buffer.begin(),
        sub_listing_buffer.end()
    );

    to_expand.flags |= FileTreeItem::Open | FileTreeItem::Explored;
}

void FileTree::collapse(size_t index)
{
    assert(index < items.size());
    items[index].flags &= ~FileTreeItem::Open;
}

StringSlice::StringSlice(const tstring& string, size_t start, size_t length)
    : ptr(&string)
    , start(start)
    , length(length)
{
}

StringSlice::operator tstring_view() const
{
    return tstring_view(ptr->data() + start, length);
}

tstring_view StringSlice::as_view() const
{
    return tstring_view(ptr->data() + start, length);
}

