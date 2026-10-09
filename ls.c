
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ls.h"
#include "display.h"

/* Store information about one file. */
typedef struct {
    char *name;
    char *fullpath;
    struct stat st;
} FileEntry;

/* Options used by qsort comparator. */
static const Options *sort_opts;

/* Duplicate a string. */
static char *copy_string(const char *src)
{
    size_t len = strlen(src) + 1;
    char *dest = malloc(len);

    if (dest != NULL) {
        memcpy(dest, src, len);
    }

    return dest;
}

/* Create a complete file path. */
static char *join_path(const char *dir, const char *name)
{
    size_t a = strlen(dir);
    size_t b = strlen(name);
    char *result;

    if (a > (size_t)-1 - b - 2) {
        return NULL;
    }

    result = malloc(a + b + 2);

    if (result != NULL) {
        snprintf(result, a + b + 2,
                 "%s/%s", dir, name);
    }

    return result;
}

/* Determine which directory entries are visible. */
static int should_show(const char *name,
                       const Options *opts)
{
    if (opts->show_all) {
        return 1;
    }

    if (opts->almost_all) {
        return strcmp(name, ".") != 0 &&
               strcmp(name, "..") != 0;
    }

    return name[0] != '.';
}

/* Select timestamp for sorting. */
static time_t get_file_time(const struct stat *st,
                            const Options *opts)
{
    if (opts->time_change) {
        return st->st_ctime;
    }

    if (opts->time_access) {
        return st->st_atime;
    }

    return st->st_mtime;
}

/* Compare two file entries for sorting. */
static int compare_entries(const void *a, const void *b)
{
    const FileEntry *fa = a;
    const FileEntry *fb = b;
    int result = 0;

    if (sort_opts->sort_size) {
        if (fa->st.st_size > fb->st.st_size) {
            result = -1;
        } else if (fa->st.st_size < fb->st.st_size) {
            result = 1;
        }
    } else if (sort_opts->sort_time) {
        time_t ta = get_file_time(&fa->st, sort_opts);
        time_t tb = get_file_time(&fb->st, sort_opts);

        if (ta > tb) {
            result = -1;
        } else if (ta < tb) {
            result = 1;
        }
    }

    /* Use filename as the tie-breaker. */
    if (result == 0) {
        result = strcmp(fa->name, fb->name);
    }

    if (sort_opts->reverse) {
        result = -result;
    }

    return result;
}

/* Release memory used by file entries. */
static void free_entries(FileEntry *files, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        free(files[i].name);
        free(files[i].fullpath);
    }

    free(files);
}

/*
 * List the contents of one directory.
 * Supports sorting, displaying details and recursion.
 */
int list_directory(const char *path, const Options *opts)
{
    DIR *dir;
    struct dirent *entry;

    FileEntry *files = NULL;

    size_t count = 0;
    size_t capacity = 0;
    size_t i;

    unsigned long long total_blocks = 0;
    int status = 0;

    dir = opendir(path);

    if (dir == NULL) {
        fprintf(stderr, "myls: %s: %s\n",
                path, strerror(errno));
        return 1;
    }

    /* Read all entries from the directory. */
    for (;;) {
        FileEntry item;
        FileEntry *tmp;
        size_t newcap;

        errno = 0;
        entry = readdir(dir);

        if (entry == NULL) {
            if (errno != 0) {
                perror("myls: readdir");
                status = 1;
            }
            break;
        }

        /* Skip entries based on -a and -A. */
        if (!should_show(entry->d_name, opts)) {
            continue;
        }

        item.name = copy_string(entry->d_name);
        item.fullpath = join_path(path, entry->d_name);

        if (item.name == NULL ||
            item.fullpath == NULL) {

            free(item.name);
            free(item.fullpath);

            fprintf(stderr,
                    "myls: memory allocation failed\n");

            status = 1;
            break;
        }

        /* Get file metadata without following symlinks. */
        if (lstat(item.fullpath, &item.st) != 0) {
            fprintf(stderr, "myls: %s: %s\n",
                    item.fullpath, strerror(errno));

            free(item.name);
            free(item.fullpath);

            status = 1;
            continue;
        }

        /* Expand the array when necessary. */
        if (count == capacity) {
            newcap = capacity == 0 ? 16 : capacity * 2;

            if (newcap < capacity ||
                newcap > (size_t)-1 / sizeof(*files)) {

                fprintf(stderr, "myls: too many files\n");

                free(item.name);
                free(item.fullpath);

                status = 1;
                break;
            }

            tmp = realloc(files,
                          newcap * sizeof(*files));

            if (tmp == NULL) {
                perror("myls: realloc");

                free(item.name);
                free(item.fullpath);

                status = 1;
                break;
            }

            files = tmp;
            capacity = newcap;
        }

        /* Accumulate allocated blocks. */
        if (item.st.st_blocks > 0) {
            total_blocks +=
                (unsigned long long)item.st.st_blocks;
        }

        files[count++] = item;
    }

    /* Sort entries unless -f is selected. */
    if (!opts->no_sort && count > 1) {
        sort_opts = opts;

        qsort(files, count,
              sizeof(*files), compare_entries);
    }

    /* Display block total for -l or -s. */
    display_total(total_blocks, opts);

    /* Display every collected entry. */
    for (i = 0; i < count; i++) {
        if (display_entry(files[i].name,
                          files[i].fullpath,
                          &files[i].st,
                          opts) != 0) {
            status = 1;
        }
    }

    /*
     * Recursively list subdirectories when -R is enabled.
     * Do not follow symbolic links, "." or "..".
     */
    if (opts->recursive) {
        for (i = 0; i < count; i++) {

            if (strcmp(files[i].name, ".") == 0 ||
                strcmp(files[i].name, "..") == 0) {
                continue;
            }

            if (S_ISDIR(files[i].st.st_mode)) {
                printf("\n");

                if (list_path(files[i].fullpath,
                              opts, 1) != 0) {
                    status = 1;
                }
            }
        }
    }

    /* Free allocated memory. */
    free_entries(files, count);

    if (closedir(dir) != 0) {
        perror("myls: closedir");
        status = 1;
    }

    return status;
}

/*
 * Process a file or directory pathname.
 * Handles regular files, symbolic links and directories.
 */
int list_path(const char *path,
              const Options *opts,
              int show_header)
{
    struct stat st;

    /* Read file metadata. */
    if (lstat(path, &st) != 0) {
        fprintf(stderr, "myls: %s: %s\n",
                path, strerror(errno));
        return 1;
    }

    /*
     * -d displays the directory itself.
     * Regular files and symbolic links are also
     * displayed directly.
     */
    if (opts->directory_only || !S_ISDIR(st.st_mode)) {
        return display_entry(path, path, &st, opts);
    }

    /* Print directory name for recursive listings. */
    if (show_header || opts->recursive) {
        printf("%s:\n", path);
    }

    /* Open and list the directory contents. */
    return list_directory(path, opts);
}
