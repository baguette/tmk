
/*

Copyright (c) 2016, Andre Schalkwyk and Cory Burgett
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TM_CRYPTO_H
#define TM_CRYPTO_H

#define TM_CRYPTO_USAGE_ERROR	0x1
#define TM_CRYPTO_FILE_ERROR	0x2

#define CRYPTO_HASH_SIZE 			20
#define CRYPTO_HASH_STRING_LENGTH	41

void tm_CryptoHashData(const unsigned char* data, unsigned char digest[CRYPTO_HASH_SIZE]);
void tm_CryptoHashFile(const char* file, unsigned char digest[CRYPTO_HASH_SIZE]);
void tm_CryptoHashToString(const unsigned char digest[CRYPTO_HASH_SIZE], char hash[CRYPTO_HASH_STRING_LENGTH]);

#define TM_CRYPTO_HASH_DATA(DATA, DIGEST)       tm_CryptoHashData((const unsigned char*)(DATA), (unsigned char*)(DIGEST))
#define TM_CRYPTO_HASH_FILE(FILE, DIGEST)       tm_CryptoHashFile((const char*)(FILE), (unsigned char*)(DIGEST))
#define TM_CRYPTO_HASH_TO_STRING(DIGEST, HASH)  tm_CryptoHashToString((const unsigned char*)(DIGEST), (char*)(HASH))
#endif
