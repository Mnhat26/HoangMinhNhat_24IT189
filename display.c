
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <limits.h>

#include "display.h"

/* Convert file type to a character. */
static char file_type(mode_t mode)
{
    if (S_ISDIR(mode))  return 'd';
    if (S_ISLNK(mode))  return 'l';
    if (S_ISCHR(mode))  return 'c';
    if (S_ISBLK(mode))  return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';

#ifdef S_ISWHT
    if (S_ISWHT(mode))  return 'w';
#endif

    return '-';
}

/* Convert permission bits to rwx format. */
static void format_permissions(mode_t mode, char out[11])
{
    out[0] = file_type(mode);

    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';

    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';

    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID)
        out[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        out[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
        out[9] = (mode & S_IXOTH) ? 't' : 'T';

    out[10] = '\0';
}

/* Select timestamp for -c, -u or default display. */
static time_t selected_time(const struct stat *st,
                            const Options *opts)
{
    if (opts->time_change)
        return st->st_ctime;

    if (opts->time_access)
        return st->st_atime;

    return st->st_mtime;
}

/* Convert file size to human-readable units. */
static void format_size(unsigned long long size,
                        char *buffer, size_t length)
{
    const char *units[] = {
        "B", "K", "M", "G", "T", "P", "E"
    };

    double value = (double)size;
    int unit = 0;

    while (value >= 1024.0 && unit < 6) {
        value /= 1024.0;
        unit++;
    }

    if (unit == 0) {
        snprintf(buffer, length, "%llu", size);
    } else if (value < 10.0) {
        snprintf(buffer, length, "%.1f%s",
                 value, units[unit]);
    } else {
        snprintf(buffer, length, "%.0f%s",
                 value, units[unit]);
    }
}

/*
 * Read NetBSD's preferred block size.
 * getbsize() handles the BLOCKSIZE environment variable.
 */
static unsigned long long get_block_unit(const Options *opts)
{
    int header_length = 0;
    long blocksize = 512;

    if (opts->kilobytes)
        return 1024ULL;

    getbsize(&header_length, &blocksize);

    if (blocksize <= 0)
        return 512ULL;

    return (unsigned long long)blocksize;
}

/*
 * Convert a number of 512-byte blocks into bytes,
 * avoiding unsigned integer overflow.
 */
static unsigned long long blocks_to_bytes(
    unsigned long long blocks)
{
    if (blocks > ULLONG_MAX / 512ULL)
        return ULLONG_MAX;

    return blocks * 512ULL;
}

/* Round up a byte count to the selected block unit. */
static unsigned long long round_up_blocks(
    unsigned long long bytes,
    unsigned long long unit)
{
    unsigned long long quotient;
    unsigned long long remainder;

    if (unit == 0)
        return 0;

    quotient = bytes / unit;
    remainder = bytes % unit;

    if (remainder != 0)
        quotient++;

    return quotient;
}

/*
 * Format allocated blocks according to:
 * -h: human-readable allocated size
 * -k: 1024-byte units
 * default: BLOCKSIZE environment setting
 */
static void format_blocks(unsigned long long blocks,
                          const Options *opts,
                          char *buffer, size_t length)
{
    unsigned long long bytes;
    unsigned long long unit;
    unsigned long long value;

    bytes = blocks_to_bytes(blocks);

    if (opts->human_readable) {
        format_size(bytes, buffer, length);
        return;
    }

    unit = get_block_unit(opts);

    value = round_up_blocks(bytes, unit);

    snprintf(buffer, length, "%llu", value);
}

/* Print total allocated blocks in a directory. */
void display_total(unsigned long long blocks,
                   const Options *opts)
{
    char buffer[64];

    if (!opts->long_format && !opts->show_blocks)
        return;

    format_blocks(blocks, opts,
                  buffer, sizeof(buffer));

    printf("total %s\n", buffer);
}

/*
 * Print a filename.
 * -q replaces non-printable characters with '?'.
 * -w prints raw characters.
 */
static void print_name(const char *name,
                       const Options *opts)
{
    const unsigned char *p =
        (const unsigned char *)name;

    int replace = opts->printable ||
                  (!opts->raw_names &&
                   isatty(STDOUT_FILENO));

    while (*p != '\0') {
        if (replace && !isprint(*p))
            putchar('?');
        else
            putchar(*p);

        p++;
    }
}

/* Append a file-type indicator for -F. */
static void print_indicator(mode_t mode,
                            const Options *opts)
{
    if (!opts->classify)
        return;

    if (S_ISDIR(mode)) {
        putchar('/');
    } else if (S_ISLNK(mode)) {
        putchar('@');
    } else if (S_ISSOCK(mode)) {
        putchar('=');
    } else if (S_ISFIFO(mode)) {
        putchar('|');
#ifdef S_ISWHT
    } else if (S_ISWHT(mode)) {
        putchar('%');
#endif
    } else if (S_ISREG(mode) &&
               (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        putchar('*');
    }
}

/* Print the destination of a symbolic link. */
static int print_link_target(const char *fullpath,
                             const Options *opts)
{
    size_t capacity = 256;

    for (;;) {
        char *target;
        ssize_t count;

        target = malloc(capacity);

        if (target == NULL) {
            perror("myls: malloc");
            return 1;
        }

        count = readlink(fullpath, target,
                         capacity - 1);

        if (count < 0) {
            perror("myls: readlink");
            free(target);
            return 1;
        }

        if ((size_t)count < capacity - 1) {
            target[count] = '\0';

            printf(" -> ");
            print_name(target, opts);

            free(target);
            return 0;
        }

        free(target);

        if (capacity > (size_t)-1 / 2) {
            fprintf(stderr, "myls: link too long\n");
            return 1;
        }

        capacity *= 2;
    }
}

/* Display information for one file entry. */
int display_entry(const char *name,
                  const char *fullpath,
                  const struct stat *st,
                  const Options *opts)
{
    char permissions[11];
    char sizebuf[64];
    char blockbuf[64];
    char datebuf[64];
    char ownerbuf[32];
    char groupbuf[32];

    const char *owner;
    const char *group;

    struct passwd *pw;
    struct group *gr;
    struct tm *tm_info;

    time_t file_time;
    time_t now;

    if (opts->show_inode) {
        printf("%ju ", (uintmax_t)st->st_ino);
    }

    if (opts->show_blocks) {
        format_blocks(
            (unsigned long long)st->st_blocks,
            opts,
            blockbuf,
            sizeof(blockbuf)
        );

        printf("%s ", blockbuf);
    }

    /* Short listing without -l or -n. */
    if (!opts->long_format) {
        print_name(name, opts);
        print_indicator(st->st_mode, opts);
        putchar('\n');
        return 0;
    }

    /* Format file permissions. */
    format_permissions(st->st_mode, permissions);

    /* Convert UID and GID to numeric strings. */
    snprintf(ownerbuf, sizeof(ownerbuf),
             "%ju", (uintmax_t)st->st_uid);

    snprintf(groupbuf, sizeof(groupbuf),
             "%ju", (uintmax_t)st->st_gid);

    owner = ownerbuf;
    group = groupbuf;

    /* -l shows names; -n shows numeric IDs. */
    if (!opts->numeric_ids) {
        pw = getpwuid(st->st_uid);

        if (pw != NULL)
            owner = pw->pw_name;

        gr = getgrgid(st->st_gid);

        if (gr != NULL)
            group = gr->gr_name;
    }

    /* Format file size or device number. */
    if (S_ISCHR(st->st_mode) ||
        S_ISBLK(st->st_mode)) {
        snprintf(sizebuf, sizeof(sizebuf),
                 "%ju", (uintmax_t)st->st_rdev);
    } else if (opts->human_readable) {
        format_size(
            (unsigned long long)st->st_size,
            sizebuf,
            sizeof(sizebuf)
        );
    } else {
        snprintf(sizebuf, sizeof(sizebuf),
                 "%jd", (intmax_t)st->st_size);
    }

    /* Choose and format file timestamp. */
    file_time = selected_time(st, opts);
    tm_info = localtime(&file_time);

    if (tm_info != NULL) {
        now = time(NULL);

        if (file_time > now + 3600 ||
            (file_time <= now &&
             now - file_time > 15552000)) {

            strftime(datebuf, sizeof(datebuf),
                     "%b %e  %Y", tm_info);
        } else {
            strftime(datebuf, sizeof(datebuf),
                     "%b %e %H:%M", tm_info);
        }
    } else {
        strcpy(datebuf, "??? ?? ??:??");
    }

    /* Print the long-format file information. */
    printf("%s %ju %-8s %-8s %8s %s ",
           permissions,
           (uintmax_t)st->st_nlink,
           owner,
           group,
           sizebuf,
           datebuf);

    print_name(name, opts);
    print_indicator(st->st_mode, opts);

    /* Symbolic links include their destination. */
    if (S_ISLNK(st->st_mode)) {
        if (print_link_target(fullpath, opts) != 0) {
            putchar('\n');
            return 1;
        }
    }

    putchar('\n');
    return 0;
}
