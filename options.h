
#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct {
    int show_all;
    int almost_all;
    int reverse;
    int sort_size;
    int sort_time;
    int no_sort;

    int long_format;
    int numeric_ids;
    int show_inode;
    int show_blocks;
    int human_readable;
    int kilobytes;

    int time_change;
    int time_access;
    int directory_only;
    int classify;
    int recursive;
    int printable;
    int raw_names;
} Options;

int parse_options(int argc, char *argv[],
                  Options *opts, int *first_path);

#endif
