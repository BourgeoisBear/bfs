// Copyright © Tavian Barnes <tavianator@tavianator.com>
// SPDX-License-Identifier: 0BSD

/**
 * Standard library wrappers and polyfills.
 */

#ifndef BFS_BFSTD_H
#define BFS_BFSTD_H

#include "config.h"
#include <stddef.h>

// #include <errno.h>

/**
 * Return whether an error code is due to a path not existing.
 */
bool is_nonexistence_error(int error);

#include <fcntl.h>

#ifndef O_EXEC
#  ifdef O_PATH
#    define O_EXEC O_PATH
#  else
#    define O_EXEC O_RDONLY
#  endif
#endif

#ifndef O_SEARCH
#  ifdef O_PATH
#    define O_SEARCH O_PATH
#  else
#    define O_SEARCH O_RDONLY
#  endif
#endif

#ifndef O_DIRECTORY
#  define O_DIRECTORY 0
#endif

#include <fnmatch.h>

#if !defined(FNM_CASEFOLD) && defined(FNM_IGNORECASE)
#  define FNM_CASEFOLD FNM_IGNORECASE
#endif

// #include <libgen.h>

/**
 * Re-entrant dirname() variant that always allocates a copy.
 *
 * @param path
 *         The path in question.
 * @return
 *         The parent directory of the path.
 */
char *xdirname(const char *path);

/**
 * Re-entrant basename() variant that always allocates a copy.
 *
 * @param path
 *         The path in question.
 * @return
 *         The final component of the path.
 */
char *xbasename(const char *path);

/**
 * Find the offset of the final component of a path.
 *
 * @param path
 *         The path in question.
 * @return
 *         The offset of the basename.
 */
size_t xbaseoff(const char *path);

#include <stdio.h>

/**
 * fopen() variant that takes open() style flags.
 *
 * @param path
 *         The path to open.
 * @param flags
 *         Flags to pass to open().
 */
FILE *xfopen(const char *path, int flags);

/**
 * Convenience wrapper for getdelim().
 *
 * @param file
 *         The file to read.
 * @param delim
 *         The delimiter character to split on.
 * @return
 *         The read chunk (without the delimiter), allocated with malloc().
 *         NULL is returned on error (errno != 0) or end of file (errno == 0).
 */
char *xgetdelim(FILE *file, char delim);

// #include <stdlib.h>

/**
 * Process a yes/no prompt.
 *
 * @return 1 for yes, 0 for no, and -1 for unknown.
 */
int ynprompt(void);

// #include <string.h>

/**
 * Format a mode like ls -l (e.g. -rw-r--r--).
 *
 * @param mode
 *         The mode to format.
 * @param str
 *         The string to hold the formatted mode.
 */
void xstrmode(mode_t mode, char str[11]);

#include <sys/types.h>

/**
 * Portable version of makedev().
 */
dev_t xmakedev(int ma, int mi);

/**
 * Portable version of major().
 */
int xmajor(dev_t dev);

/**
 * Portable version of minor().
 */
int xminor(dev_t dev);

// #include <sys/stat.h>

#if __APPLE__
#  define st_atim st_atimespec
#  define st_ctim st_ctimespec
#  define st_mtim st_mtimespec
#  define st_birthtim st_birthtimespec
#endif

// #include <unistd.h>

/**
 * Like dup(), but set the FD_CLOEXEC flag.
 *
 * @param fd
 *         The file descriptor to duplicate.
 * @return
 *         A duplicated file descriptor, or -1 on failure.
 */
int dup_cloexec(int fd);

/**
 * Like pipe(), but set the FD_CLOEXEC flag.
 *
 * @param pipefd
 *         The array to hold the two file descriptors.
 * @return
 *         0 on success, -1 on failure.
 */
int pipe_cloexec(int pipefd[2]);

/**
 * A safe version of read() that handles interrupted system calls and partial
 * reads.
 *
 * @return
 *         The number of bytes read.  A value != nbytes indicates an error
 *         (errno != 0) or end of file (errno == 0).
 */
size_t xread(int fd, void *buf, size_t nbytes);

/**
 * A safe version of write() that handles interrupted system calls and partial
 * writes.
 *
 * @return
           The number of bytes written.  A value != nbytes indicates an error.
 */
size_t xwrite(int fd, const void *buf, size_t nbytes);

/**
 * close() variant that preserves errno.
 *
 * @param fd
 *         The file descriptor to close.
 */
void close_quietly(int fd);

/**
 * close() wrapper that asserts the file descriptor is valid.
 *
 * @param fd
 *         The file descriptor to close.
 * @return
 *         0 on success, or -1 on error.
 */
int xclose(int fd);

/**
 * Wrapper for faccessat() that handles some portability issues.
 */
int xfaccessat(int fd, const char *path, int amode);

/**
 * readlinkat() wrapper that dynamically allocates the result.
 *
 * @param fd
 *         The base directory descriptor.
 * @param path
 *         The path to the link, relative to fd.
 * @param size
 *         An estimate for the size of the link name (pass 0 if unknown).
 * @return
 *         The target of the link, allocated with malloc(), or NULL on failure.
 */
char *xreadlinkat(int fd, const char *path, size_t size);

/**
 * Wrapper for confstr() that allocates with malloc().
 *
 * @param name
 *         The ID of the confstr to look up.
 * @return
 *         The value of the confstr, or NULL on failure.
 */
char *xconfstr(int name);

/**
 * Portability wrapper for strtofflags().
 *
 * @param str
 *         The string to parse.  The pointee will be advanced to the first
 *         invalid position on error.
 * @param set
 *         The flags that are set in the string.
 * @param clear
 *         The flags that are cleared in the string.
 * @return
 *         0 on success, -1 on failure.
 */
int xstrtofflags(const char **str, unsigned long long *set, unsigned long long *clear);

// #include <wchar.h>

/**
 * wcswidth() variant that works on narrow strings.
 *
 * @param str
 *         The string to measure.
 * @return
 *         The likely width of that string in a terminal.
 */
size_t xstrwidth(const char *str);

#endif // BFS_BFSTD_H
