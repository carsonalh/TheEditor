#include "the_editor.h"

#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_PATH_LEN 1024

void filetree_init(FileTree *filetree)
{
    WIN32_FIND_DATA ffd;
    HANDLE hfind = NULL;
    const char search[] = ".\\*";

    const unsigned capacity = 1024;
    const unsigned strbuffer_capacity = 2048;

    *filetree = (FileTree) {
        .len = 0,
        .cap = capacity,
        .items = calloc(capacity, sizeof (FileTreeItem)),
        .strbuffer = calloc(strbuffer_capacity, sizeof (char)),
        .strbuffer_cap = strbuffer_capacity,
        .strbuffer_len = 0,
    };

    hfind = FindFirstFile(search, &ffd);
    if (hfind != INVALID_HANDLE_VALUE) {
        do {
            if (!strncmp(ffd.cFileName, ".", sizeof ffd.cFileName)
                || !strncmp(ffd.cFileName, "..", sizeof ffd.cFileName))
                continue;

            FileTreeFlags flags = 0;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                flags = FILETREE_DIRECTORY;

            unsigned name_len = (unsigned)strnlen(ffd.cFileName, sizeof ffd.cFileName);
            if (name_len + filetree->strbuffer_len > filetree->strbuffer_cap) {
                filetree->strbuffer_cap *= 2;
                filetree->strbuffer = realloc(filetree->strbuffer, filetree->strbuffer_cap * sizeof (char));
                assert(filetree->strbuffer);
            }

            // strings of differing char sizes, no strncpy here :(
            for (unsigned i = 0; i < name_len; i++) {
                filetree->strbuffer[filetree->strbuffer_len + i] = (char)ffd.cFileName[i];
            }

            unsigned name_offset = filetree->strbuffer_len;
            filetree->strbuffer_len += name_len;

            if (filetree->len + 1 > filetree->cap) {
                filetree->cap *= 2;
                filetree->items = realloc(filetree->items, filetree->cap * sizeof *filetree->items);
                assert(filetree->items);
            }

            filetree->items[filetree->len++] = (FileTreeItem) {
                .depth = 0,
                .flags = flags,
                .name_offset = name_offset,
                .name_len = name_len,
            };
        }
        while (FindNextFile(hfind, &ffd));
    }
}

void filetree_uninit(FileTree *filetree)
{
    free(filetree->items);
    free(filetree->strbuffer);
}

void filetree_expand(FileTree *filetree, size_t index)
{
    assert(index < filetree->len);
    assert(filetree->items[index].flags & FILETREE_DIRECTORY);

    FileTreeItem *item;

    item = &filetree->items[index];
    if (item->flags & FILETREE_EXPLORED) {
        item->flags |= FILETREE_OPEN;
        return;
    }

    size_t path_len = 0;
    TCHAR path[MAX_PATH_LEN] = {0};

    { // Build the path
        // path will be of the form .\a\b\c\*
        // we build it in reverse because that's how our filetree listing is structured
        path[0] = _T('*');
        path_len++;

        do {
            if (path_len + item->name_len + 1 > MAX_PATH_LEN) {
                fprintf(stderr, "buffer overrun appending path segment\n");
                abort();
            }
            path[path_len] = '\\';
            for (int i = item->name_len - 1; i >= 0; i--) {
                path[path_len + item->name_len - i] = filetree->strbuffer[item->name_offset + i];
            }
            path_len += item->name_len + 1;

            unsigned depth = item->depth;
            while (item >= filetree->items && item->depth >= depth)
                item--;
        } while (item >= filetree->items);

        if (path_len + 2 > MAX_PATH_LEN) {
            fprintf(stderr, "buffer overrun trying to append \\.\n");
            abort();
        }
        _tcsncpy(&path[path_len], "\\.", 2);
        path_len += 2;

        for (int i = 0; i < path_len / 2; i++) {
            TCHAR tmp = path[i];
            path[i] = path[path_len - i - 1];
            path[path_len - i - 1] = tmp;
        }

        if (path_len >= MAX_PATH_LEN) {
            fprintf(stderr, "maximum path length is %d (it must be null terminated)\n", MAX_PATH_LEN - 1);
            abort();
        }
        assert(!path[path_len]);
    }

    size_t sub_listing_len = 0;
    size_t sub_listing_cap = 128;
    FileTreeItem *sub_listing = calloc(sub_listing_cap, sizeof *sub_listing);

    { // Build the sub listing
        WIN32_FIND_DATA ffd;
        HANDLE hfind = FindFirstFile(path, &ffd);
        assert(hfind);
        const FileTreeItem *parent = &filetree->items[index];
        do {
            if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, ".."))
                continue;

            if (sub_listing_len + 1 > sub_listing_cap) {
                sub_listing_cap *= 2;
                sub_listing = realloc(sub_listing, sub_listing_cap * sizeof *sub_listing);
            }

            unsigned name_len = (unsigned)_tcsnlen(ffd.cFileName, sizeof ffd.cFileName / sizeof ffd.cFileName[0]);
            if (filetree->strbuffer_len + name_len > filetree->strbuffer_cap) {
                filetree->strbuffer_cap *= 2;
                filetree->strbuffer = realloc(
                        filetree->strbuffer,
                        filetree->strbuffer_cap * sizeof *filetree->strbuffer);
            }
            unsigned name_offset = filetree->strbuffer_len;
            for (unsigned i = 0; i < name_len; i++) {
                filetree->strbuffer[name_offset + i] = (char)ffd.cFileName[i];
            }
            filetree->strbuffer_len += name_len;

            unsigned flags = 0;
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                flags |= FILETREE_DIRECTORY;

            item = &sub_listing[sub_listing_len];
            *item = (FileTreeItem) {
                .depth = parent->depth + 1,
                .flags = flags,
                .name_len = name_len,
                .name_offset = name_offset,
            };
            sub_listing_len++;
        } while (FindNextFile(hfind, &ffd));
    }

    while (filetree->len + sub_listing_len > filetree->cap) {
        filetree->cap *= 2;
        filetree->items = realloc(filetree->items, filetree->cap * sizeof *filetree->items);
    }

    item = &filetree->items[index];
    memmove(item + 1 + sub_listing_len, item + 1, sub_listing_len * sizeof *item);
    memcpy(item + 1, sub_listing, sub_listing_len * sizeof *item);
    free(sub_listing);
    filetree->len += sub_listing_len;
}

void filetree_collapse(FileTree *filetree, size_t index)
{
    assert(index < filetree->len);
    assert(filetree->items[index].flags & FILETREE_DIRECTORY);
    assert(filetree->items[index].flags & FILETREE_EXPLORED && "invalid state");

    filetree->items[index].flags &= ~FILETREE_OPEN;
}
