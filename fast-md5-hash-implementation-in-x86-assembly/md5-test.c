/* 
 * MD5 hash in C and x86 assembly
 * 
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/fast-md5-hash-implementation-in-x86-assembly
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* Function prototypes */

static bool self_check(void);
void md5_hash(const uint8_t *message, size_t len, uint32_t hash[4]);

// Link this program with an external C or x86 compression function
extern void md5_compress(uint32_t state[4], const uint8_t block[64]);


/* Main program */

int main(void) {
	if (!self_check()) {
		printf("Self-check failed\n");
		return EXIT_FAILURE;
	}
	printf("Self-check passed\n");
	
	// Benchmark speed
	uint32_t state[4] = {0};
	uint8_t block[64] = {0};
	const int N = 10000000;
	clock_t start_time = clock();
	for (int i = 0; i < N; i++)
		md5_compress(state, block);
	printf("Speed: %.1f MB/s\n", (double)N * sizeof(block) / (clock() - start_time) * CLOCKS_PER_SEC / 1000000);
	
	return EXIT_SUCCESS;
}


/* Self-check */

struct testcase {
	uint32_t answer[4];
	const uint8_t *message;
};

#define TESTCASE(a,b,c,d,msg) {{UINT32_C(a),UINT32_C(b),UINT32_C(c),UINT32_C(d)}, (const uint8_t *)msg}

// Note: The MD5 standard specifies that uint32 are serialized to/from bytes in little endian
static struct testcase testCases[] = {
	TESTCASE(0xD98C1DD4,0x04B2008F,0x980980E9,0x7E42F8EC, ""),
	TESTCASE(0xB975C10C,0xA8B6F1C0,0xE299C331,0x61267769, "a"),
	TESTCASE(0x98500190,0xB04FD23C,0x7D3F96D6,0x727FE128, "abc"),
	TESTCASE(0x7D696BF9,0x8D93B77C,0x312F5A52,0xD061F1AA, "message digest"),
	TESTCASE(0xD7D3FCC3,0x00E49261,0x6C49FB7D,0x3BE167CA, "abcdefghijklmnopqrstuvwxyz"),
	TESTCASE(0x98AB74D1,0xF5D977D2,0x2C1C61A5,0x9F9D419F, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
	TESTCASE(0xA2F4ED57,0x55C9E32B,0x2EDA49AC,0x7AB60721, "12345678901234567890123456789012345678901234567890123456789012345678901234567890"),
};

static bool self_check(void) {
	for (size_t i = 0; i < sizeof(testCases) / sizeof(testCases[i]); i++) {
		struct testcase *tc = &testCases[i];
		uint32_t hash[4];
		md5_hash(tc->message, strlen((const char *)tc->message), hash);
		if (memcmp(hash, tc->answer, sizeof(tc->answer)) != 0)
			return false;
	}
	return true;
}


/* Full message hasher */

void md5_hash(const uint8_t *message, size_t len, uint32_t hash[4]) {
	hash[0] = UINT32_C(0x67452301);
	hash[1] = UINT32_C(0xEFCDAB89);
	hash[2] = UINT32_C(0x98BADCFE);
	hash[3] = UINT32_C(0x10325476);
	
	#define BLOCK_SIZE 64  // In bytes
	#define LENGTH_SIZE 8  // In bytes
	
	size_t off;
	for (off = 0; len - off >= BLOCK_SIZE; off += BLOCK_SIZE)
		md5_compress(hash, &message[off]);
	
	uint8_t block[BLOCK_SIZE] = {0};
	size_t rem = len - off;
	memcpy(block, &message[off], rem);
	
	block[rem] = 0x80;
	rem++;
	if (BLOCK_SIZE - rem < LENGTH_SIZE) {
		md5_compress(hash, block);
		memset(block, 0, sizeof(block));
	}
	
	block[BLOCK_SIZE - LENGTH_SIZE] = (uint8_t)((len & 0x1FU) << 3);
	len >>= 5;
	for (int i = 1; i < LENGTH_SIZE; i++, len >>= 8)
		block[BLOCK_SIZE - LENGTH_SIZE + i] = (uint8_t)(len & 0xFFU);
	md5_compress(hash, block);
}