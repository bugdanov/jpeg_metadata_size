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
#include <sys/stat.h>
#include <sys/wait.h>
#include <libgen.h>

#include "sha256sum.h"

#define VERSION "1.00"

int jpeg_strip(FILE *f);
int print_jpeg_sha256(FILE *f);
int isRegularFile(char *filename);
FILE *isJpegFile(char *filename);
char *appName;

#define BUFSIZE 32767
char *buf;

int mypipe[2];
struct stat sb;
unsigned char header[6];
char *filename;

int main(int argc, char **argv) {

  int err=0;

  appName=basename(argv[0]);

  if (argc<2 || !strcmp(argv[1],"-h")) {
    fprintf(stderr,"usage: %s [-h] <jpeg_file> [<jpeg_file> ...]\n",appName);
    exit(1);
  }

  buf=malloc(BUFSIZE);
  if (!buf) {
    fprintf(stderr,"%s: out of memory !\n",appName);
    exit(1);
  }

  FILE *f;
  for (int i=1; i<argc; ++i) {
    filename=argv[i];
    if ((f=isJpegFile(filename))) {
      err|=print_jpeg_sha256(f);
    } else {
      err=1;
    }
  }
  exit(err);
}

int isRegularFile(char *filename) {
  stat(filename, &sb);

  if (S_ISREG(sb.st_mode)) {
    return 1;
  }

  if (S_ISDIR(sb.st_mode)) {
    fprintf(stderr,"%s: %s: is a directory\n",appName,filename);
  } else {
    fprintf(stderr,"%s: %s: is not a regular file\n",appName,filename);
  }

  return 0;
}

FILE *isJpegFile(char *filename) {

  if (!isRegularFile(filename)) {
    return 0;
  }

  FILE *f=fopen(filename,"r");
  if (!f) {
    fprintf(stderr,"%s: cannot open %s\n",appName,filename);
    return 0;
  }

  size_t count=fread(header,1,6,f);
  if (count!=6 || header[0]!=0xff && header[1]!=0xd8 && header[2]!=0xff && header[3]&0xfe!=0xe0) {
    fprintf(stderr,"%s: %s: not a jpeg\n",appName,filename);
    fclose(f);
    return 0;
  }

  return f;

}


int print_jpeg_sha256(FILE *f) {

  int pid;
  int err=0;

  if (pipe(mypipe)) {
    return errno;
  }

  if (!(pid=fork())) {
    if (dup2(mypipe[1],STDOUT_FILENO)==-1) {
      err=errno;
    }

    close(mypipe[0]);
    close(mypipe[1]);

    if (!err) {
      err=jpeg_strip(f);
    }

    exit(err);

  } else {
    if (dup2(mypipe[0],STDIN_FILENO)==-1) {
      err=errno;
    }

    close(mypipe[0]);
    close(mypipe[1]);

    if (err) {
      return err;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    err=sha256sum(stdin,hash);
    if (err) {
      return err;
    }

    for (int i=0; i<SHA256_DIGEST_LENGTH; ++i) {
      printf("%02x",hash[i]);
    }
    printf(" %s\n", filename);

    return 0;
  }


}

int jpeg_strip(FILE *f) {

  int c;
  unsigned char d;
  int lsb=0;
  int length;
  int err=0;

  const unsigned char *dontStrip="\xc0\xc1\xc2\xc3\xc5\xc6\xc7\xc9\xca\xcb\xcd\xce\xcf\xdd";

  if (putchar(0xff)!=0xff || putchar(0xd8)!=0xd8) {
    fprintf(stderr,"%s: stdout: write error\n",appName);
    return 1;
  }

  // skip first app segment
  length=header[4]<<8;
  length+=header[5];

  if (length<2) {
    fprintf(stderr,"%s: %s: invalid segment length\n",appName,filename);
    return 1;
  }

  if (length>2 && fseek(f,length-2,SEEK_CUR)<0) {
    err=errno;
    fprintf(stderr,"%s: %s: seek failed\n",appName,filename);
    return err;
  }


  while ((c=fgetc(f))!=EOF) {

    if (lsb) {

      if (c==0xd9 || c==0xda) { // EOI or SOS
        // copy
        if (putchar(0xff)!=0xff || putchar(c)!=c) {
          fprintf(stderr,"%s: stdout: write error\n",appName);
          err=1;
          break;
        }
        if (c==0xd9) {
          break;
        }

        if (c==0xda) {  // SOS
          size_t count, wcount, written;

          // copy everything from the SOS 
          while ((count=fread(buf,1,BUFSIZE,f))) {
            d=buf[count-1];
            wcount=0;
            while(wcount<count && ((written=fwrite(buf+wcount,1,count-wcount,stdout)))) {
              wcount+=written;
            }
            if (wcount!=count) {
              fprintf(stderr,"%s: stdout: write error\n",appName);
              err=1;
              break;
            }
          }

          if (ferror(f)) {
            fprintf(stderr,"%s: %s: read error\n",appName,filename);
            err=1;

          } else {
            if (d!=0xd9) {
              fprintf(stderr,"%s: %s: last byte %02x is not EOI\n",appName,filename,d);
              err=1;
            }
          }
          break;
        }

      } else {

        // get segment length
        length=0;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"%s: %s: unexpected end of file\n",appName,filename);
          err=1;
          break;
        }
        length+=d<<8;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"%s: %s: unexpected end of file\n",appName,filename);
          err=1;
          break;
        }
        length+=d;

        if (length<2) {
          fprintf(stderr,"%s: %s: invalid segment length\n",appName,filename);
          err=1;
          break;
        }

        // copy everything but jfif, exif, xmp, comments, iptc, and unknown markers
        if (strchr(dontStrip,c)) {
          if (putchar(0xff)!=0xff || putchar(c)!=c || putchar(length>>8)!=length>>8 || putchar(d)!=d) {
            fprintf(stderr,"%s: stdout: write error\n",appName);
            err=1;
            break;
          }

          length-=2;
          while(length && ((c=fgetc(f))!=EOF)) {
            if (putchar(c)!=c) {
              fprintf(stderr,"%s: stdout: write error\n",appName);
              err=1;
              break;
            }
            --length;
          }
          if (length && !err) {
            fprintf(stderr,"%s: %s: unexpected end of file\n",appName,filename);
            err=1;
          }
          if (err) {
            break;
          }

        } else {
          // skip segment
          if (length>2 && fseek(f,length-2,SEEK_CUR)<0) {
            fprintf(stderr,"%s: %s: seek failed\n",appName,filename);
            err=errno;
            break;
          }
        }
      }
      lsb=0;

    } else if (c==0xff) {
      lsb=1;

    } else {
      fprintf(stderr,"%s: %s: not a jpeg\n",appName,filename);
      err=1;
      break;
    }

  }

  fclose(f);
  return err;

}

