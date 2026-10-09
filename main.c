
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ls.h"
#include "options.h"

/*
 * Store information about each command-line operand.
 */
typedef struct {
    const char *path;
    struct stat st;
    int valid;
    int is_directory;
} Operand;

/*
 * Compare operands in lexicographical order.
 */
static int compare_operands(const void *a, const void *b)
{
    const Operand *oa = a;
    const Operand *ob = b;

    if (oa->is_directory != ob->is_directory)
        return oa->is_directory - ob->is_directory;

    return strcmp(oa->path, ob->path);
}

/*
 * Main entry point of the program.
 */
int main(int argc, char *argv[])
{
    Options opts;
    int first_path;
    int result = 0;
    int i;
    int count;
    int valid_count = 0;
    int printed = 0;
    Operand *operands;

    if (parse_options(argc, argv,
                      &opts, &first_path) != 0) {
        return 1;
    }

    /*
     * If no path is supplied, list the current directory.
     */
    if (first_path >= argc) {
        return list_path(".", &opts, 0);
    }

    count = argc - first_path;

    operands = calloc((size_t)count, sizeof(*operands));

    if (operands == NULL) {
        perror("myls: calloc");
        return 1;
    }

    /*
     * Collect and validate all paths.
     */
    for (i = 0; i < count; i++) {
        operands[i].path = argv[first_path + i];

        if (lstat(operands[i].path,
                  &operands[i].st) != 0) {
            fprintf(stderr, "myls: %s: %s\n",
                    operands[i].path, strerror(errno));

            result = 1;
            continue;
        }

        operands[i].valid = 1;
        operands[i].is_directory =
            !opts.directory_only &&
            S_ISDIR(operands[i].st.st_mode);

        valid_count++;
    }

    /*
     * Sort regular files before directories.
     * Keep input order if -f is requested.
     */
    if (!opts.no_sort) {
        qsort(operands, (size_t)count,
              sizeof(*operands), compare_operands);
    }

    /*
     * Display each valid operand.
     */
    for (i = 0; i < count; i++) {
        if (!operands[i].valid)
            continue;

        if (printed)
            printf("\n");

        if (list_path(operands[i].path, &opts,
                      operands[i].is_directory &&
                      valid_count > 1) != 0) {
            result = 1;
        }

        printed = 1;
    }

    free(operands);
    return result;
}
