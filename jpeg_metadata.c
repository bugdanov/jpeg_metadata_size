/*
 * jpeg_metadata Copyright (c) 2015 Rurik Bugdanov
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
#include <stdint.h>

#define BUF_INCREMENT 64*1024

char *buf=0;
size_t bufsize=BUF_INCREMENT;

int print_jpeg_metadata(char *filename);

int main(int argc, char **argv) {

  int i;
  int err=0;

  if (argc<2 || !strcmp(argv[1],"-h")) {
    fprintf(stderr,"usage: jpeg_metadata_size [-h] <jpeg_file> [<jpeg_file> ...]\n");
    exit(1);
  }

  buf=malloc(bufsize);
  if (!buf) {
    fprintf(stderr,"out of memory !\n");
    exit(1);
  }

  for (i=1; i<argc; ++i) {
    err|=print_jpeg_metadata(argv[i]);
  }

  return err;

}

int print_jpeg_metadata(char *filename) {

  FILE *f=fopen(filename,"r");
  if (!f) {
    fprintf(stderr,"error: cannot open file %s\n",filename);
    return errno;
  }

  int c; 
  int lsb=0;
  int err=1;
  int offset=0;
  long metadata_size=0;

  while ((c=fgetc(f))!=EOF) {

    buf[offset++]=c;

    if (lsb) {

      int pos=ftell(f)-2;
      int length;
      int d;

      if (c!=0xd8) {

        if ((c&0xf0)!=0xe0) {

          // write results
          metadata_size=offset-2;
          if (metadata_size!=ftell(f)-2 || metadata_size<0) {
            fprintf(stderr,"unexpected error\n");
            break;
          }
          err=0;

          fprintf(stderr,"%d %s\n",metadata_size,filename);
          break;
        }

        length=0;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"error: unexpected end of file %s\n",filename);
          break;
        }
        buf[offset++]=d;
        length+=d<<8;

        if ((d=fgetc(f))==EOF) {
          fprintf(stderr,"error: unexpected end of file %s\n",filename);
          break;
        }
        buf[offset++]=d;
        length+=d;

        if (length<2) {
          fprintf(stderr,"error: invalid segment length in %s\n",filename);
          break;
        }

        length-=2;

        if (offset+length>bufsize) {
          size_t newsize=offset+length+BUF_INCREMENT;
          char *newbuf=realloc(buf,newsize);
          if (!newbuf) {
            fprintf(stderr,"error: out of memory while processing %s\n",filename);
            break;
          }
          buf=newbuf;
          bufsize=newsize;
        }

        uint16_t segment=*((uint16_t*)(buf+offset-4));
        fprintf(stderr,"segment: %02X%02X, length: %d\n",segment&0xff,segment>>8,length);

        if (length) {
          if (fread(buf+offset, 1, length, f)!=length){
           fprintf(stderr,"error: read error while processing %s\n",filename);
           break;
         }
         offset+=length;
        }
/*        if (fseek(f,offset,SEEK_SET)<0) {
          fprintf(stderr,"error: read error while processing %s\n",filename);
          break;
        }
        */

      }
         
      lsb=0;

    } else if (c==0xff) {
      lsb=1;
    }
  }

  fclose(f);

  // write filename
  int len=strlen(filename)+1;
  if (fwrite(filename,1,len,stdout)!=len) {
    fprintf(stderr,"error: write error while processing %s\n",filename);
    exit(1);
  }

  uint32_t size32=(uint32_t)metadata_size;

  // write metadata size
  if (fwrite(&size32,1,sizeof(size32),stdout)!=sizeof(size32)) {
    fprintf(stderr,"error: write error while processing %s\n",filename);
    exit(1);
  }

  if (metadata_size) {
    // write metadata        
    if (fwrite(buf,1,size32,stdout)!=size32){
      fprintf(stderr,"error: write error while processing %s\n",filename);
      exit(1);
    }
  }

  fflush(stdout);

  return err;

}

