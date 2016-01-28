#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

#define BUFSIZE 32767
int sha256sum(FILE *f, unsigned char *md);
