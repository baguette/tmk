/*********************************************************************
* Filename:   	sha1.c
* Author:     	Brad Conte (brad AT bradconte.com)
* Modified by:	Andre Schalkwyk (avs.aswyk AT gmail.com) 2016-01-05
* Copyright:
* Disclaimer: 	This code is presented "as is" without any guarantees.
* Details:    	Implementation of the SHA1 hashing algorithm.
              	Algorithm specification can be found here:
               	* http://csrc.nist.gov/publications/fips/fips180-2/fips180-2withchangenotice.pdf
              	This implementation uses little endian byte order.
*********************************************************************/

/*************************** HEADER FILES ***************************/
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "tm_crypto.h"
#include "config.h"      /* for WORD */


/****************************** MACROS ******************************/
#define SHA1_BLOCK_LENGTH	64
#define SHA1_DIGEST_LENGTH	20

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;

typedef struct {
	BYTE data[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[5];
	WORD k[4];
} SHA1_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const BYTE data[], size_t len);
void sha1_final(SHA1_CTX *ctx, BYTE hash[]);


/****************** HIGHER LEVEL CRYPTO FUNCTIONS *******************/
void tm_CryptoHashData(const unsigned char* data, unsigned char digest[CRYPTO_HASH_SIZE])
{
    BYTE        buff[SHA1_BLOCK_LENGTH];
	BYTE*		pBuff;
	int			bytesLeft;
	int			len;

    /* Context to hold SHA1 hash */
    SHA1_CTX     sha_ctx;

    if (!data) {
		data = (BYTE *)"";
	}

	pBuff = (unsigned char*)&data[0];
	len = strlen((char*)data);

    /* Initialize SHA1 Context */
    sha1_init(&sha_ctx);

	while(pBuff < data + len)
    {
		bytesLeft = strlen((const char*)pBuff);

		strncpy((char*)buff, (char*)pBuff, bytesLeft < SHA1_BLOCK_LENGTH ? bytesLeft : SHA1_BLOCK_LENGTH);

		sha1_update(&sha_ctx, buff, bytesLeft < SHA1_BLOCK_LENGTH ? bytesLeft : SHA1_BLOCK_LENGTH);

		pBuff += SHA1_BLOCK_LENGTH;
    }

    /* Finalize SHA1 Context */
    sha1_final(&sha_ctx, digest);
}

void tm_CryptoHashFile(const char* file, unsigned char digest[CRYPTO_HASH_SIZE])
{
	/* File Handle */
    FILE*       fp = NULL;
	/* buffer to store unhashed datya */
    BYTE        buff[SHA1_BLOCK_LENGTH];
	/* How many bytes we have read from */
    int         bytesRead;
    /* Context to hold SHA1 hash */
    SHA1_CTX     sha_ctx;

    /* Initialize SHA1 Context */
    sha1_init(&sha_ctx);

    if((fp = fopen(file, "r")) == NULL) {
        printf("File %s could not be opened\n", file);
        exit(TM_CRYPTO_FILE_ERROR);
    }

    while((bytesRead = fread(buff, 1, SHA1_BLOCK_LENGTH, fp)) != 0)
    {
        /* Update SHA1 Context with block of data that was read */
        sha1_update(&sha_ctx, buff, bytesRead);
    }

    fclose(fp);

    /* Finalize SHA1 Context */
    sha1_final(&sha_ctx, digest);
}

void tm_CryptoHashToString(const unsigned char digest[CRYPTO_HASH_SIZE], char hash[CRYPTO_HASH_STRING_LENGTH])
{
	int i;
	char *p = hash;

	for (i = 0; i < 20; i++) {
		p += sprintf(p, "%02x", digest[i]);
	}
}

/****************************** MACROS ******************************/
#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

/*********************** FUNCTION DEFINITIONS ***********************/
void sha1_transform(SHA1_CTX *ctx, const BYTE data[])
{
	WORD a, b, c, d, e, i, j, t, m[80];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) + (data[j + 1] << 16) + (data[j + 2] << 8) + (data[j + 3]);
	for ( ; i < 80; ++i) {
		m[i] = (m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16]);
		m[i] = (m[i] << 1) | (m[i] >> 31);
	}

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];

	for (i = 0; i < 20; ++i) {
		t = ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + ctx->k[0] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 40; ++i) {
		t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[1] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 60; ++i) {
		t = ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d))  + e + ctx->k[2] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}
	for ( ; i < 80; ++i) {
		t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[3] + m[i];
		e = d;
		d = c;
		c = ROTLEFT(b, 30);
		b = a;
		a = t;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
}

void sha1_init(SHA1_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xc3d2e1f0;
	ctx->k[0] = 0x5a827999;
	ctx->k[1] = 0x6ed9eba1;
	ctx->k[2] = 0x8f1bbcdc;
	ctx->k[3] = 0xca62c1d6;
}

void sha1_update(SHA1_CTX *ctx, const BYTE data[], size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha1_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha1_final(SHA1_CTX *ctx, BYTE hash[])
{
	WORD i;

	i = ctx->datalen;

	/* Pad whatever data is left in the buffer. */
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha1_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	/* Append to the padding the total message's length in bits and transform. */
	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha1_transform(ctx, ctx->data);

	/* Since this implementation uses little endian byte ordering and MD uses big endian, */
	/* reverse all the bytes when copying the final state to the output hash. */
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
	}
}
