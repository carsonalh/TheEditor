#include "theeditor.h"

#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void ft_init(size_t *len_listing, FileTreeItem **listing)
{
    WIN32_FIND_DATA ffd;
    HANDLE hfind = NULL;
    const char search[] = ".\\*";

    hfind = FindFirstFile(search, &ffd);
    *len_listing = 0;

    if (hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strncmp(ffd.cFileName, ".", sizeof ffd.cFileName)
                && strncmp(ffd.cFileName, "..", sizeof ffd.cFileName))
                (*len_listing)++;
        }
        while (FindNextFile(hfind, &ffd));
    }

    *listing = calloc(*len_listing, sizeof **listing);

    hfind = FindFirstFile(search, &ffd);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        int i = 0;
        do
        {
            if (!strncmp(ffd.cFileName, ".", sizeof ffd.cFileName)
                || !strncmp(ffd.cFileName, "..", sizeof ffd.cFileName))
                continue;

            FileTreeItemFlags type;
            const char *name;

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                type = FTI_DIRECTORY;
            else
                type = FTI_FILE;

            (*listing)[i++] = (FileTreeItem){
                .depth = 1,
                .len_name = strnlen(ffd.cFileName, sizeof ffd.cFileName),
                .name = strdup(ffd.cFileName),
                .flags = type,
            };
        }
        while (FindNextFile(hfind, &ffd));
    }
}

void ft_uninit(size_t len_listing, FileTreeItem *listing)
{
    for (int i = 0; i < len_listing; i++)
    {
        free((char*)listing[i].name);
    }

    free(listing);
}

void ft_expand(size_t *len_listing, FileTreeItem **listing, int index)
{
    assert(0 <= index && index < *len_listing);
    assert((*listing)[index].flags & FTI_DIRECTORY);

    FileTreeItem *node = &(*listing)[index];

    if (node->flags & FTI_EXPLORED)
    {
        node->flags |= FTI_OPEN;
        return;
    }

    size_t num_dirs_in_path;
    char *path;

    { // Build the path
        struct DirectoryName {
            size_t len_name;
            const char *name;
        };

        num_dirs_in_path = node->depth;
        struct DirectoryName *dirs = malloc(num_dirs_in_path * sizeof *dirs);

        size_t len_path = strlen("\\*") + 1;
        for (int i = num_dirs_in_path - 1; i >= 0; i--)
        {
            dirs[i].len_name = node->len_name;
            dirs[i].name = node->name;

            len_path += node->len_name;
            if (i > 0)
                len_path++;

            int depth = node->depth;
            while (node >= *listing && node->depth >= depth)
                node--;

            assert(node < *listing && i == 0 || node >= *listing);
        }

        path = malloc(len_path * sizeof *path);
        char *c = path;

        for (int i = 0; i < num_dirs_in_path; i++)
        {
            strncpy(c, dirs[i].name, dirs[i].len_name);
            c += dirs[i].len_name;
            if (i < num_dirs_in_path - 1)
                *c++ = '\\';
        }

        strncpy(c, "\\*", 3);

        free(dirs);
    }

    WIN32_FIND_DATA ffd;
    HANDLE hfind = NULL;
    size_t len_sub_listing = 0;

    { // Calculate sub listing length
        hfind = FindFirstFile(path, &ffd);

        assert(hfind != INVALID_HANDLE_VALUE);

        do
        {
            if (strncmp(ffd.cFileName, ".", sizeof ffd.cFileName)
                && strncmp(ffd.cFileName, "..", sizeof ffd.cFileName))
                len_sub_listing++;
        }
        while (FindNextFile(hfind, &ffd));
    }

    *len_listing += len_sub_listing;
    *listing = realloc(*listing, *len_listing * sizeof **listing);
    assert(*listing != NULL);
    memmove(&(*listing)[index + 1 + len_sub_listing], &(*listing)[index + 1], len_sub_listing * sizeof **listing);

    hfind = FindFirstFile(path, &ffd);

	do
	{
        if (!strncmp(ffd.cFileName, ".", sizeof ffd.cFileName)
            || !strncmp(ffd.cFileName, "..", sizeof ffd.cFileName))
            continue;

        FileTreeItemFlags type;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            type = FTI_DIRECTORY;
        else
            type = FTI_FILE;

        (*listing)[++index] = (FileTreeItem){
            .depth = num_dirs_in_path + 1,
            .len_name = strnlen(ffd.cFileName, sizeof ffd.cFileName),
            .name = strdup(ffd.cFileName),
            .flags = type
        };
	}
	while (FindNextFile(hfind, &ffd));

    (*listing)[index].flags |= FTI_EXPLORED;
}

void ft_collapse(size_t len_listing, FileTreeItem* listing, int index)
{
    assert(0 <= index && index < len_listing);
    assert(listing[index].flags & FTI_DIRECTORY);

    listing[index].flags &= ~FTI_OPEN;
}
