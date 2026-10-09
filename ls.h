#ifndef LS_H
#define LS_H

#include "options.h"

/*
 * List the contents of a directory.
 * Return 0 on success, 1 on error.
 */
int list_directory(const char *path, const Options *opts);
int list_path(const char *path,
              const Options *opts,
              int show_header);

#endif