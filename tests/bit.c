// Copyright © Tavian Barnes <tavianator@tavianator.com>
// SPDX-License-Identifier: 0BSD

#include "../src/bit.h"
#include "../src/diag.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

bfs_static_assert(UMAX_WIDTH(0x1) == 1);
bfs_static_assert(UMAX_WIDTH(0x3) == 2);
bfs_static_assert(UMAX_WIDTH(0x7) == 3);
bfs_static_assert(UMAX_WIDTH(0xF) == 4);
bfs_static_assert(UMAX_WIDTH(0xFF) == 8);
bfs_static_assert(UMAX_WIDTH(0xFFF) == 12);
bfs_static_assert(UMAX_WIDTH(0xFFFF) == 16);

#define UWIDTH_MAX(n) (2 * ((UINTMAX_C(1) << ((n) - 1)) - 1) + 1)
#define IWIDTH_MAX(n) UWIDTH_MAX((n) - 1)
#define IWIDTH_MIN(n) (-(intmax_t)IWIDTH_MAX(n) - 1)

bfs_static_assert(UCHAR_MAX == UWIDTH_MAX(UCHAR_WIDTH));
bfs_static_assert(SCHAR_MIN == IWIDTH_MIN(SCHAR_WIDTH));
bfs_static_assert(SCHAR_MAX == IWIDTH_MAX(SCHAR_WIDTH));

bfs_static_assert(USHRT_MAX == UWIDTH_MAX(USHRT_WIDTH));
bfs_static_assert(SHRT_MIN == IWIDTH_MIN(SHRT_WIDTH));
bfs_static_assert(SHRT_MAX == IWIDTH_MAX(SHRT_WIDTH));

bfs_static_assert(UINT_MAX == UWIDTH_MAX(UINT_WIDTH));
bfs_static_assert(INT_MIN == IWIDTH_MIN(INT_WIDTH));
bfs_static_assert(INT_MAX == IWIDTH_MAX(INT_WIDTH));

bfs_static_assert(ULONG_MAX == UWIDTH_MAX(ULONG_WIDTH));
bfs_static_assert(LONG_MIN == IWIDTH_MIN(LONG_WIDTH));
bfs_static_assert(LONG_MAX == IWIDTH_MAX(LONG_WIDTH));

bfs_static_assert(ULLONG_MAX == UWIDTH_MAX(ULLONG_WIDTH));
bfs_static_assert(LLONG_MIN == IWIDTH_MIN(LLONG_WIDTH));
bfs_static_assert(LLONG_MAX == IWIDTH_MAX(LLONG_WIDTH));

bfs_static_assert(SIZE_MAX == UWIDTH_MAX(SIZE_WIDTH));
bfs_static_assert(PTRDIFF_MIN == IWIDTH_MIN(PTRDIFF_WIDTH));
bfs_static_assert(PTRDIFF_MAX == IWIDTH_MAX(PTRDIFF_WIDTH));

bfs_static_assert(UINTPTR_MAX == UWIDTH_MAX(UINTPTR_WIDTH));
bfs_static_assert(INTPTR_MIN == IWIDTH_MIN(INTPTR_WIDTH));
bfs_static_assert(INTPTR_MAX == IWIDTH_MAX(INTPTR_WIDTH));

bfs_static_assert(UINTMAX_MAX == UWIDTH_MAX(UINTMAX_WIDTH));
bfs_static_assert(INTMAX_MIN == IWIDTH_MIN(INTMAX_WIDTH));
bfs_static_assert(INTMAX_MAX == IWIDTH_MAX(INTMAX_WIDTH));

#define verify_eq(a, b) \
	bfs_verify((a) == (b), "(0x%jX) %s != %s (0x%jX)", (uintmax_t)(a), #a, #b, (uintmax_t)(b))

int main(void) {
	verify_eq(bswap((uint8_t)0x12), 0x12);
	verify_eq(bswap((uint16_t)0x1234), 0x3412);
	verify_eq(bswap((uint32_t)0x12345678), 0x78563412);
	verify_eq(bswap((uint64_t)0x1234567812345678), 0x7856341278563412);

	verify_eq(count_ones(0x0), 0);
	verify_eq(count_ones(0x1), 1);
	verify_eq(count_ones(0x2), 1);
	verify_eq(count_ones(0x3), 2);
	verify_eq(count_ones(0x137F), 10);

	verify_eq(count_zeros(0), INT_WIDTH);
	verify_eq(count_zeros(0L), LONG_WIDTH);
	verify_eq(count_zeros(0LL), LLONG_WIDTH);
	verify_eq(count_zeros((uint8_t)0), 8);
	verify_eq(count_zeros((uint16_t)0), 16);
	verify_eq(count_zeros((uint32_t)0), 32);
	verify_eq(count_zeros((uint64_t)0), 64);

	verify_eq(rotate_left((uint8_t)0xA1, 4), 0x1A);
	verify_eq(rotate_left((uint16_t)0x1234, 12), 0x4123);
	verify_eq(rotate_left((uint32_t)0x12345678, 20), 0x67812345);
	verify_eq(rotate_left((uint32_t)0x12345678, 0), 0x12345678);

	verify_eq(rotate_right((uint8_t)0xA1, 4), 0x1A);
	verify_eq(rotate_right((uint16_t)0x1234, 12), 0x2341);
	verify_eq(rotate_right((uint32_t)0x12345678, 20), 0x45678123);
	verify_eq(rotate_right((uint32_t)0x12345678, 0), 0x12345678);

	for (int i = 0; i < 16; ++i) {
		uint16_t n = (uint16_t)1 << i;
		for (int j = i; j < 16; ++j) {
			uint16_t m = (uint16_t)1 << j;
			uint16_t nm = n | m;
			verify_eq(count_ones(nm), 1 + (n != m));
			verify_eq(count_zeros(nm), 15 - (n != m));
			verify_eq(leading_zeros(nm), 15 - j);
			verify_eq(trailing_zeros(nm), i);
			verify_eq(first_leading_one(nm), j + 1);
			verify_eq(first_trailing_one(nm), i + 1);
			verify_eq(bit_width(nm), j + 1);
			verify_eq(bit_floor(nm), m);
			if (n == m) {
				verify_eq(bit_ceil(nm), m);
				bfs_verify(has_single_bit(nm));
			} else {
				if (j < 15) {
					verify_eq(bit_ceil(nm), (m << 1));
				}
				bfs_verify(!has_single_bit(nm));
			}
		}
	}

	verify_eq(leading_zeros((uint16_t)0), 16);
	verify_eq(trailing_zeros((uint16_t)0), 16);
	verify_eq(first_leading_one(0), 0);
	verify_eq(first_trailing_one(0), 0);
	verify_eq(bit_width(0), 0);
	verify_eq(bit_floor(0), 0);
	verify_eq(bit_ceil(0), 1);

	return EXIT_SUCCESS;
}
