/*
 * jpeg_metadata_size Copyright (c) 2015 Rurik Bugdanov
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

int main(int argc, char **argv) {

  if (argc!=2 || (argc==2 && !strcmp(argv[1],"-h"))) {
    fprintf(stderr,"usage: jpeg_metadata_size <jpeg_file>\n");
    exit(1);
  }

  FILE *f=fopen(argv[1],"r");
  if (!f) exit(errno);

  int c; 
  int lsb=0;
  while ((c=fgetc(f))!=EOF) {

    if (lsb) {

      int pos=ftell(f)-2;
      int length;
      int d;

      if (c!=0xd8) {

        if ((c&0xf0)!=0xe0) {
          printf("%d %s\n",ftell(f)-2,argv[1]);
          exit(0);
        }

        length=0;

        if ((d=fgetc(f))==EOF) break;
        length+=d<<8;

        if ((d=fgetc(f))==EOF) break;
        length+=d;

        if (length<2) exit(1);

        if (length && fseek(f,length-2,SEEK_CUR)<0) exit(errno);

      }
         
      lsb=0;

    } else if (c==0xff) {
      lsb=1;
    }
  }

  fclose(f);
  return 0;
}

