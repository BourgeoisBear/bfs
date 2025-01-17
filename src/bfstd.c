// Copyright © Tavian Barnes <tavianator@tavianator.com>
// SPDX-License-Identifier: 0BSD

#include "bfstd.h"
#include "bit.h"
#include "config.h"
#include "diag.h"
#include "xregex.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <langinfo.h>
#include <nl_types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#if BFS_USE_SYS_SYSMACROS_H
#  include <sys/sysmacros.h>
#elif BFS_USE_SYS_MKDEV_H
#  include <sys/mkdev.h>
#endif

#if BFS_USE_UTIL_H
#  include <util.h>
#endif

bool is_nonexistence_error(int error) {
	return error == ENOENT || errno == ENOTDIR;
}

char *xdirname(const char *path) {
	size_t i = xbaseoff(path);

	// Skip trailing slashes
	while (i > 0 && path[i - 1] == '/') {
		--i;
	}

	if (i > 0) {
		return strndup(path, i);
	} else if (path[i] == '/') {
		return strdup("/");
	} else {
		return strdup(".");
	}
}

char *xbasename(const char *path) {
	size_t i = xbaseoff(path);
	size_t len = strcspn(path + i, "/");
	if (len > 0) {
		return strndup(path + i, len);
	} else if (path[i] == '/') {
		return strdup("/");
	} else {
		return strdup(".");
	}
}

size_t xbaseoff(const char *path) {
	size_t i = strlen(path);

	// Skip trailing slashes
	while (i > 0 && path[i - 1] == '/') {
		--i;
	}

	// Find the beginning of the name
	while (i > 0 && path[i - 1] != '/') {
		--i;
	}

	// Skip leading slashes
	while (path[i] == '/' && path[i + 1]) {
		++i;
	}

	return i;
}

FILE *xfopen(const char *path, int flags) {
	char mode[4];

	switch (flags & O_ACCMODE) {
	case O_RDONLY:
		strcpy(mode, "rb");
		break;
	case O_WRONLY:
		strcpy(mode, "wb");
		break;
	case O_RDWR:
		strcpy(mode, "r+b");
		break;
	default:
		bfs_bug("Invalid access mode");
		errno = EINVAL;
		return NULL;
	}

	if (flags & O_APPEND) {
		mode[0] = 'a';
	}

	int fd;
	if (flags & O_CREAT) {
		fd = open(path, flags, 0666);
	} else {
		fd = open(path, flags);
	}

	if (fd < 0) {
		return NULL;
	}

	FILE *ret = fdopen(fd, mode);
	if (!ret) {
		close_quietly(fd);
		return NULL;
	}

	return ret;
}

char *xgetdelim(FILE *file, char delim) {
	char *chunk = NULL;
	size_t n = 0;
	ssize_t len = getdelim(&chunk, &n, delim, file);
	if (len >= 0) {
		if (chunk[len] == delim) {
			chunk[len] = '\0';
		}
		return chunk;
	} else {
		free(chunk);
		if (!ferror(file)) {
			errno = 0;
		}
		return NULL;
	}
}

const char *xgetprogname(void) {
	const char *cmd = NULL;
#if __GLIBC__
	cmd = program_invocation_short_name;
#elif BSD
	cmd = getprogname();
#endif

	if (!cmd) {
		cmd = BFS_COMMAND;
	}

	return cmd;
}

/** Compile and execute a regular expression for xrpmatch(). */
static int xrpregex(nl_item item, const char *response) {
	const char *pattern = nl_langinfo(item);
	if (!pattern) {
		return -1;
	}

	struct bfs_regex *regex;
	int ret = bfs_regcomp(&regex, pattern, BFS_REGEX_POSIX_EXTENDED, 0);
	if (ret == 0) {
		ret = bfs_regexec(regex, response, 0);
	}

	bfs_regfree(regex);
	return ret;
}

/** Check if a response is affirmative or negative. */
static int xrpmatch(const char *response) {
	int ret = xrpregex(NOEXPR, response);
	if (ret > 0) {
		return 0;
	} else if (ret < 0) {
		return -1;
	}

	ret = xrpregex(YESEXPR, response);
	if (ret > 0) {
		return 1;
	} else if (ret < 0) {
		return -1;
	}

	// Failsafe: always handle y/n
	char c = response[0];
	if (c == 'n' || c == 'N') {
		return 0;
	} else if (c == 'y' || c == 'Y') {
		return 1;
	} else {
		return -1;
	}
}

int ynprompt(void) {
	fflush(stderr);

	char *line = xgetdelim(stdin, '\n');
	int ret = line ? xrpmatch(line) : -1;
	free(line);
	return ret;
}

/** Get the single character describing the given file type. */
static char type_char(mode_t mode) {
	switch (mode & S_IFMT) {
	case S_IFREG:
		return '-';
	case S_IFBLK:
		return 'b';
	case S_IFCHR:
		return 'c';
	case S_IFDIR:
		return 'd';
	case S_IFLNK:
		return 'l';
	case S_IFIFO:
		return 'p';
	case S_IFSOCK:
		return 's';
#ifdef S_IFDOOR
	case S_IFDOOR:
		return 'D';
#endif
#ifdef S_IFPORT
	case S_IFPORT:
		return 'P';
#endif
#ifdef S_IFWHT
	case S_IFWHT:
		return 'w';
#endif
	}

	return '?';
}

void *xmemdup(const void *src, size_t size) {
	void *ret = malloc(size);
	if (ret) {
		memcpy(ret, src, size);
	}
	return ret;
}

char *xstpecpy(char *dest, char *end, const char *src) {
	return xstpencpy(dest, end, src, SIZE_MAX);
}

char *xstpencpy(char *dest, char *end, const char *src, size_t n) {
	size_t space = end - dest;
	n = space < n ? space : n;
	n = strnlen(src, n);
	memcpy(dest, src, n);
	if (n < space) {
		dest[n] = '\0';
		return dest + n;
	} else {
		end[-1] = '\0';
		return end;
	}
}

void xstrmode(mode_t mode, char str[11]) {
	strcpy(str, "----------");

	str[0] = type_char(mode);

	if (mode & 00400) {
		str[1] = 'r';
	}
	if (mode & 00200) {
		str[2] = 'w';
	}
	if ((mode & 04100) == 04000) {
		str[3] = 'S';
	} else if (mode & 04000) {
		str[3] = 's';
	} else if (mode & 00100) {
		str[3] = 'x';
	}

	if (mode & 00040) {
		str[4] = 'r';
	}
	if (mode & 00020) {
		str[5] = 'w';
	}
	if ((mode & 02010) == 02000) {
		str[6] = 'S';
	} else if (mode & 02000) {
		str[6] = 's';
	} else if (mode & 00010) {
		str[6] = 'x';
	}

	if (mode & 00004) {
		str[7] = 'r';
	}
	if (mode & 00002) {
		str[8] = 'w';
	}
	if ((mode & 01001) == 01000) {
		str[9] = 'T';
	} else if (mode & 01000) {
		str[9] = 't';
	} else if (mode & 00001) {
		str[9] = 'x';
	}
}

dev_t xmakedev(int ma, int mi) {
#ifdef makedev
	return makedev(ma, mi);
#else
	return (ma << 8) | mi;
#endif
}

int xmajor(dev_t dev) {
#ifdef major
	return major(dev);
#else
	return dev >> 8;
#endif
}

int xminor(dev_t dev) {
#ifdef minor
	return minor(dev);
#else
	return dev & 0xFF;
#endif
}

int dup_cloexec(int fd) {
#ifdef F_DUPFD_CLOEXEC
	return fcntl(fd, F_DUPFD_CLOEXEC, 0);
#else
	int ret = dup(fd);
	if (ret < 0) {
		return -1;
	}

	if (fcntl(ret, F_SETFD, FD_CLOEXEC) == -1) {
		close_quietly(ret);
		return -1;
	}

	return ret;
#endif
}

int pipe_cloexec(int pipefd[2]) {
#if __linux__ || (BSD && !__APPLE__)
	return pipe2(pipefd, O_CLOEXEC);
#else
	if (pipe(pipefd) != 0) {
		return -1;
	}

	if (fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) == -1 || fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) == -1) {
		close_quietly(pipefd[1]);
		close_quietly(pipefd[0]);
		return -1;
	}

	return 0;
#endif
}

size_t xread(int fd, void *buf, size_t nbytes) {
	size_t count = 0;

	while (count < nbytes) {
		ssize_t ret = read(fd, (char *)buf + count, nbytes - count);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				break;
			}
		} else if (ret == 0) {
			// EOF
			errno = 0;
			break;
		} else {
			count += ret;
		}
	}

	return count;
}

size_t xwrite(int fd, const void *buf, size_t nbytes) {
	size_t count = 0;

	while (count < nbytes) {
		ssize_t ret = write(fd, (const char *)buf + count, nbytes - count);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				break;
			}
		} else if (ret == 0) {
			// EOF?
			errno = 0;
			break;
		} else {
			count += ret;
		}
	}

	return count;
}

void close_quietly(int fd) {
	int error = errno;
	xclose(fd);
	errno = error;
}

int xclose(int fd) {
	int ret = close(fd);
	if (ret != 0) {
		bfs_verify(errno != EBADF);
	}
	return ret;
}

int xfaccessat(int fd, const char *path, int amode) {
	int ret = faccessat(fd, path, amode, 0);

#ifdef AT_EACCESS
	// Some platforms, like Hurd, only support AT_EACCESS.  Other platforms,
	// like Android, don't support AT_EACCESS at all.
	if (ret != 0 && (errno == EINVAL || errno == ENOTSUP)) {
		ret = faccessat(fd, path, amode, AT_EACCESS);
	}
#endif

	return ret;
}

char *xconfstr(int name) {
#if __ANDROID__
	errno = ENOTSUP;
	return NULL;
#else
	size_t len = confstr(name, NULL, 0);
	if (len == 0) {
		return NULL;
	}

	char *str = malloc(len);
	if (!str) {
		return NULL;
	}

	if (confstr(name, str, len) != len) {
		free(str);
		return NULL;
	}

	return str;
#endif // !__ANDROID__
}

char *xreadlinkat(int fd, const char *path, size_t size) {
	ssize_t len;
	char *name = NULL;

	if (size == 0) {
		size = 64;
	} else {
		++size; // NUL terminator
	}

	while (true) {
		char *new_name = realloc(name, size);
		if (!new_name) {
			goto error;
		}
		name = new_name;

		len = readlinkat(fd, path, name, size);
		if (len < 0) {
			goto error;
		} else if ((size_t)len >= size) {
			size *= 2;
		} else {
			break;
		}
	}

	name[len] = '\0';
	return name;

error:
	free(name);
	return NULL;
}

int xstrtofflags(const char **str, unsigned long long *set, unsigned long long *clear) {
#if BSD && !__GNU__
	char *str_arg = (char *)*str;
	unsigned long set_arg = 0;
	unsigned long clear_arg = 0;

#if __NetBSD__
	int ret = string_to_flags(&str_arg, &set_arg, &clear_arg);
#else
	int ret = strtofflags(&str_arg, &set_arg, &clear_arg);
#endif

	*str = str_arg;
	*set = set_arg;
	*clear = clear_arg;

	if (ret != 0) {
		errno = EINVAL;
	}
	return ret;
#else // !BSD
	errno = ENOTSUP;
	return -1;
#endif
}

/** mbrtowc() wrapper. */
static int xmbrtowc(wchar_t *wc, size_t *i, const char *str, size_t len, mbstate_t *mb) {
	size_t mblen = mbrtowc(wc, str + *i, len - *i, mb);
	switch (mblen) {
	case -1:
		// Invalid byte sequence
		*i += 1;
		memset(mb, 0, sizeof(*mb));
		return -1;
	case -2:
		// Incomplete byte sequence
		*i += len;
		return -1;
	default:
		*i += mblen;
		return 0;
	}
}

size_t xstrwidth(const char *str) {
	size_t len = strlen(str);
	size_t ret = 0;

	mbstate_t mb;
	memset(&mb, 0, sizeof(mb));

	for (size_t i = 0; i < len;) {
		wchar_t wc;
		if (xmbrtowc(&wc, &i, str, len, &mb) == 0) {
			ret += wcwidth(wc);
		} else {
			// Assume a single-width '?'
			++ret;
		}
	}

	return ret;
}

/** Check if a character is printable. */
static bool xisprint(unsigned char c, enum wesc_flags flags) {
	if (isprint(c)) {
		return true;
	}

	// Technically a literal newline is safe inside single quotes, but $'\n'
	// is much nicer than '
	// '
	if (!(flags & WESC_SHELL) && isspace(c)) {
		return true;
	}

	return false;
}

/** Check if a wide character is printable. */
static bool xiswprint(wchar_t c, enum wesc_flags flags) {
	if (iswprint(c)) {
		return true;
	}

	if (!(flags & WESC_SHELL) && iswspace(c)) {
		return true;
	}

	return false;
}

/** Get the length of the longest printable prefix of a string. */
static size_t printable_len(const char *str, size_t len, enum wesc_flags flags) {
	// Fast path: avoid multibyte checks
	size_t i;
	for (i = 0; i < len; ++i) {
		unsigned char c = str[i];
		if (!isascii(c)) {
			break;
		}
		if (!xisprint(c, flags)) {
			return i;
		}
	}

	mbstate_t mb;
	memset(&mb, 0, sizeof(mb));

	while (i < len) {
		wchar_t wc;
		if (xmbrtowc(&wc, &i, str, len, &mb) != 0) {
			break;
		}
		if (!xiswprint(wc, flags)) {
			break;
		}
	}

	return i;
}

/** Convert a special char into a well-known escape sequence like "\n". */
static const char *dollar_esc(char c) {
	// https://www.gnu.org/software/bash/manual/html_node/ANSI_002dC-Quoting.html
	switch (c) {
	case '\a':
		return "\\a";
	case '\b':
		return "\\b";
	case '\033':
		return "\\e";
	case '\f':
		return "\\f";
	case '\n':
		return "\\n";
	case '\r':
		return "\\r";
	case '\t':
		return "\\t";
	case '\v':
		return "\\v";
	case '\'':
		return "\\'";
	case '\\':
		return "\\\\";
	default:
		return NULL;
	}
}

/** $'Quote' a string for the shell. */
static char *dollar_quote(char *dest, char *end, const char *str, size_t len, enum wesc_flags flags) {
	dest = xstpecpy(dest, end, "$'");

	mbstate_t mb;
	memset(&mb, 0, sizeof(mb));

	for (size_t i = 0; i < len;) {
		size_t start = i;
		bool safe = false;

		wchar_t wc;
		if (xmbrtowc(&wc, &i, str, len, &mb) == 0) {
			safe = xiswprint(wc, flags);
		}

		for (size_t j = start; j < i; ++j) {
			if (str[j] == '\'' || str[j] == '\\') {
				safe = false;
				break;
			}
		}

		if (safe) {
			dest = xstpencpy(dest, end, str + start, i - start);
		} else {
			for (size_t j = start; j < i; ++j) {
				unsigned char byte = str[j];
				const char *esc = dollar_esc(byte);
				if (esc) {
					dest = xstpecpy(dest, end, esc);
				} else {
					static const char *hex[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};
					dest = xstpecpy(dest, end, "\\x");
					dest = xstpecpy(dest, end, hex[byte / 0x10]);
					dest = xstpecpy(dest, end, hex[byte % 0x10]);
				}
			}
		}
	}

	return xstpecpy(dest, end, "'");
}

/** How much of this string is safe as a bare word? */
static size_t bare_len(const char *str, size_t len) {
	// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02
	size_t ret = strcspn(str, "|&;<>()$`\\\"' *?[#˜=%!");
	return ret < len ? ret : len;
}

/** How much of this string is safe to double-quote? */
static size_t quotable_len(const char *str, size_t len) {
	// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02_03
	size_t ret = strcspn(str, "`$\\\"!");
	return ret < len ? ret : len;
}

/** "Quote" a string for the shell. */
static char *double_quote(char *dest, char *end, const char *str, size_t len) {
	dest = xstpecpy(dest, end, "\"");
	dest = xstpencpy(dest, end, str, len);
	return xstpecpy(dest, end, "\"");
}

/** 'Quote' a string for the shell. */
static char *single_quote(char *dest, char *end, const char *str, size_t len) {
	bool open = false;

	while (len > 0) {
		size_t chunk = strcspn(str, "'");
		chunk = chunk < len ? chunk : len;
		if (chunk > 0) {
			if (!open) {
				dest = xstpecpy(dest, end, "'");
				open = true;
			}
			dest = xstpencpy(dest, end, str, chunk);
			str += chunk;
			len -= chunk;
		}

		while (len > 0 && *str == '\'') {
			if (open) {
				dest = xstpecpy(dest, end, "'");
				open = false;
			}
			dest = xstpecpy(dest, end, "\\'");
			++str;
			--len;
		}
	}

	if (open) {
		dest = xstpecpy(dest, end, "'");
	}

	return dest;
}

char *wordesc(char *dest, char *end, const char *str, enum wesc_flags flags) {
	return wordnesc(dest, end, str, SIZE_MAX, flags);
}

char *wordnesc(char *dest, char *end, const char *str, size_t n, enum wesc_flags flags) {
	size_t len = strnlen(str, n);
	char *start = dest;

	if (printable_len(str, len, flags) < len) {
		// String contains unprintable chars, use $'this\x7Fsyntax'
		dest = dollar_quote(dest, end, str, len, flags);
	} else if (!(flags & WESC_SHELL) || bare_len(str, len) == len) {
		// Whole string is safe as a bare word
		dest = xstpencpy(dest, end, str, len);
	} else if (quotable_len(str, len) == len) {
		// Whole string is safe to double-quote
		dest = double_quote(dest, end, str, len);
	} else {
		// Single-quote the whole string
		dest = single_quote(dest, end, str, len);
	}

	if (dest == start) {
		dest = xstpecpy(dest, end, "\"\"");
	}

	return dest;
}
