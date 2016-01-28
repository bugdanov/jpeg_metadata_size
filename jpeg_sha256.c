/*
 * jpeg_sha256 Copyright (c) 2016 Luc Deschenaux
 *
 * Compute a sha256 hash, skipping app0, app1, comments, iptc and unknown
 * markers, up to the sos marker
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional Terms:
 *
 *      You are required to preserve legal notices and author attributions in
 *      that material or in the Appropriate Legal Notices displayed by works
 *      containing it.
 *
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "sha256sum.h"

#define VERSION "1.00"

int jpeg_strip(char *filename);

char *filename;

int mypipe[2];

int main(int argc, char **argv) {

  int pid;
  int err=0;

  if (argc<2 || !strcmp(argv[1],"-h")) {
    fprintf(stderr,"usage: jpeg_sha256 [-h] <jpeg_file>\n");
    exit(1);
  }

  filename=argv[1];
  pipe(mypipe);

  if (!(pid=fork())) {
    dup2(mypipe[1],STDOUT_FILENO);
    close(mypipe[0]);
    close(mypipe[1]);

    err=jpeg_strip(filename);
    exit(err);

  } else {
    dup2(mypipe[0],STDIN_FILENO);
    close(mypipe[0]);
    close(mypipe[1]);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    err=sha256sum(stdin,hash);
    if (!err) {
      for (int i=0; i<SHA256_DIGEST_LENGTH; ++i) {
        printf("%02x",hash[i]);
      }
      printf("\n");
    }
    exit(err);
  }

}

int jpeg_strip(char *filename) {

  FILE *f=fopen(filename,"r");
  if (!f) {
    fprintf(stderr,"error: cannot open file %s\n",filename);
    return errno;
  }

  int c; 
  int lsb=0;
  int err=0;
  int first=1;

  const unsigned char *dontStrip="\xc0\xc1\xc2\xc3\xc5\xc6\xc7\xc9\xca\xcb\xcd\xce\xcf\xd8\xd9\xda\xdd";

  while ((c=fgetc(f))!=EOF) {

    if (lsb) {

      int length;
      int d;

      if (c==0xd8 || c==0xd9 || c==0xda) { // SOI or EOI or SOS
        // copy
        if (putchar(0xff)!=0xff || putchar(c)!=c) {
          fprintf(stderr,"stdout: write error\n");
          err=1;
          break;
        }
        if (c==0xd9) {
          break;
        }

        if (c==0xda) {  // SOS
          while ((c=fgetc(f))!=EOF) {
            d=c;
            if (putchar(d)!=d) {
              fprintf(stderr,"stdout: write error\n");
              err=1;
              break;
            }
          }
          if (d!=0xd9) {
            fprintf(stderr,"%s: last byte is not EOI\n",filename);
            err=1;
          }
          break;
        }

      } else {
        // get segment length
        length=0;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"%s: unexpected end of file\n",filename);
          err=1;
          break;
        }
        length+=d<<8;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"%s: unexpected end of file\n",filename);
          err=1;
          break;
        }
        length+=d;

        if (length<2) {
          fprintf(stderr,"%s: invalid segment length\n",filename);
          err=1;
          break;
        }

        // copy everything but jfif, exif, xmp, comments, iptc, and unknown markers
        if (strchr(dontStrip,c)) {
          if (putchar(0xff)!=0xff || putchar(c)!=c || putchar(d>>8)!=d>>8 || putchar(d&0xff)!=d&0xff) {
            fprintf(stderr,"stdout: write error\n");
            err=1;
            break;
          }

          length-=2;
          while(length && ((c=fgetc(f))!=EOF)) {
            if (putchar(c)!=c) {
              fprintf(stderr,"stdout: write error\n");
              err=1;
              break;
            }
            --length;
          }
          if (length && !err) {
            fprintf(stderr,"%s: unexpected end of file\n",filename);
            err=1;
          }
          if (err) {
            break;
          }

        } else {
          // skip segment
          if (length>2 && fseek(f,length-2,SEEK_CUR)<0) {
            fprintf(stderr,"%s: seek failed\n",filename);
            err=errno;                         
            break;                             
          }
        }                                    
      }
      lsb=0;

    } else if (c==0xff) {
      lsb=1;
    }
  }

  fclose(f);
  return err;

}

