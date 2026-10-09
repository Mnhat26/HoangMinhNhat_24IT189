
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "options.h"

/*
 * Parse command-line options for myls.
 */
int parse_options(int argc, char *argv[],
                  Options *opts, int *first_path)
{
    int i, j;

    memset(opts, 0, sizeof(*opts));

    /*
     * NetBSD ls enables -A by default for superuser.
     */
    if (geteuid() == 0) {
        opts->almost_all = 1;
    }

    for (i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        }

        if (argv[i][0] != '-' ||
            argv[i][1] == '\0') {
            break;
        }

        for (j = 1; argv[i][j] != '\0'; j++) {

            switch (argv[i][j]) {

            case 'a':
                opts->show_all = 1;
                opts->almost_all = 0;
                break;

            case 'A':
                opts->almost_all = 1;
                opts->show_all = 0;
                break;

            case 'c':
                opts->time_change = 1;
                opts->time_access = 0;
                break;

            case 'u':
                opts->time_access = 1;
                opts->time_change = 0;
                break;

            case 'd':
                opts->directory_only = 1;
                opts->recursive = 0;
                break;

            case 'R':
                opts->recursive = 1;
                opts->directory_only = 0;
                break;

            case 'F':
                opts->classify = 1;
                break;

            case 'f':
                opts->no_sort = 1;
                opts->sort_size = 0;
                opts->sort_time = 0;
                break;

            case 'r':
                opts->reverse = 1;
                break;

            case 'S':
                opts->sort_size = 1;
                opts->sort_time = 0;
                opts->no_sort = 0;
                break;

            case 't':
                opts->sort_time = 1;
                opts->sort_size = 0;
                opts->no_sort = 0;
                break;

            case 'l':
                opts->long_format = 1;
                opts->numeric_ids = 0;
                break;

            case 'n':
                opts->long_format = 1;
                opts->numeric_ids = 1;
                break;

            case 'i':
                opts->show_inode = 1;
                break;

            case 's':
                opts->show_blocks = 1;
                break;

            case 'h':
                opts->human_readable = 1;
                opts->kilobytes = 0;
                break;

            case 'k':
                opts->kilobytes = 1;
                opts->human_readable = 0;
                break;

            case 'q':
                opts->printable = 1;
                opts->raw_names = 0;
                break;

            case 'w':
                opts->raw_names = 1;
                opts->printable = 0;
                break;

            default:
                fprintf(stderr,
                        "myls: unknown option -- %c\n",
                        argv[i][j]);
                return 1;
            }
        }
    }

    *first_path = i;
    return 0;
}
