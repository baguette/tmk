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

#include <stdio.h>
#include <stdlib.h>

#include "../../tm_crypto.h"

#define USAGE_ERROR -1
#define FILE_ERROR -2

void usage(int argc, char **arg);
void printDigestInHex(BYTE   digest[SHA1_DIGEST_LENGTH], const char* file);

void usage(int argc, char **arg)
{
    printf("Usage : %s FILENAME\n\n", arg[0]);
    exit(USAGE_ERROR);
}

void printDigestInHex(BYTE   digest[SHA1_DIGEST_LENGTH], const char* file)
{
    int a;

    for(a = 0;a < SHA1_DIGEST_LENGTH;a++)
    {

        printf("%02x", digest[a]);
    }

    printf("  %s\n", file);
}

int main(int argc, char **argv)
{
    /* File Handle */
    FILE*       fp = NULL;
    BYTE        buff[SHA1_BLOCK_LENGTH];
    int         bytesRead;

    /* Context to hold SHA1 hash */
    SHA1_CTX     sha_ctx;
    BYTE   digest[SHA1_DIGEST_LENGTH];

    /* Check that SHA1 test driver has correct number of arguments */
    if(argc != 2)
    {
        usage(argc, argv);
    }

    /* Initialize SHA1 Context */
    sha1_init(&sha_ctx);

    if((fp = fopen(argv[1], "r")) == NULL) {
        printf("File %s could not be opened\n", argv[1]);
        exit(FILE_ERROR);
    }

    while((bytesRead = fread(buff, 1, SHA1_BLOCK_LENGTH, fp)) != 0)
    {
        /* Update SHA1 Context with block of data that was read */
        sha1_update(&sha_ctx, buff, bytesRead);
    }

    /* Finalize SHA1 Context */
    sha1_final(&sha_ctx, digest);

    /* Print SHA1 Digest */
    printDigestInHex(digest, argv[1]);

    return 0;
}
