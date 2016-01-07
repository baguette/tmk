/*********************************************************************
* Filename:   	sha1.h
* Author:     	Brad Conte (brad AT bradconte.com)
* Modified by:	Andre Schalkwyk (avs.aswyk AT gmail.com) 2016-01-05
* Copyright:
* Disclaimer: 	This code is presented "as is" without any guarantees.
* Details:    	Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef TM_CRYPTO_H
#define TM_CRYPTO_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************** HIGHER LEVEL CRYPTO FUNCTIONS *******************/

// TODO : Functions that TMake will use to cache dependancies



/****************************** MACROS ******************************/
#define SHA1_BLOCK_LENGTH	64
#define SHA1_DIGEST_LENGTH	20			// SHA1 outputs a 20 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

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

#endif   // SHA1_H
