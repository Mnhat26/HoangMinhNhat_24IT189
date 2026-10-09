#ifndef DISPLAY_H
#define DISPLAY_H

#include <sys/types.h>
#include <sys/stat.h>
#include "options.h"

/* Display information for one file. */
int display_entry(const char *name,
                  const char *fullpath,
                  const struct stat *st,
                  const Options *opts);

/* Display directory block total. */
void display_total(unsigned long long blocks,
                   const Options *opts);

#endif