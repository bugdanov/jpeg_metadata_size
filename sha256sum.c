#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 32767

#include <openssl/sha.h>

int sha256sum(FILE *f, unsigned char *md) {
  int err=0;
  SHA256_CTX c;
  char *buf;
  char *vbuf;

  buf=malloc(BUFSIZE);
  if (!buf) {
    fprintf(stderr,"could not allocate buffer\n");
    return 1;
  }

  vbuf=malloc(BUFSIZE);
  if (!vbuf) {
    fprintf(stderr,"could not allocate buffer\n");
    return 1;
  }

  if (setvbuf(f, vbuf, _IOFBF, BUFSIZE)) {
    fprintf(stderr,"setvbuf failed\n");
    return 1;
  }

  err=!SHA256_Init(&c);
  if (err) {
    fprintf(stderr,"could not init sha256 context\n");
    return err;
  }

  size_t count;
  while((count=fread(buf,1,BUFSIZE,f))) {
    if ((err=!SHA256_Update(&c, buf, count))) {
      fprintf(stderr,"could not update sha256\n");
      return err;
    }
  }

  if (ferror(f)) {
    fprintf(stderr,"could not read from file\n");
    return 1;
  }

  err=!SHA256_Final(md,&c);
  if (err) {
    fprintf(stderr,"could not get the hash\n");
  }
  return err;
}

