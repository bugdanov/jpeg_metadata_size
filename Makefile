all: jpeg_metadata_size jpeg_metadata

jpeg_metadata_size: jpeg_metadata_size.c  Makefile
	gcc jpeg_metadata_size.c -O3 -o jpeg_metadata_size

jpeg_metadata: jpeg_metadata.c  Makefile
	gcc jpeg_metadata.c -g -o jpeg_metadata 
