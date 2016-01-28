all: jpeg_metadata_size jpeg_metadata jpeg_sha256

jpeg_metadata_size: jpeg_metadata_size.c  Makefile
	gcc -std=gnu11 jpeg_metadata_size.c -O3 -o jpeg_metadata_size

jpeg_metadata: jpeg_metadata.c  Makefile
	gcc -std=gnu11 jpeg_metadata.c -O3 -o jpeg_metadata 

jpeg_sha256: jpeg_sha256.c  Makefile
	gcc -std=gnu11 jpeg_sha256.c sha256sum.c -O3 -o jpeg_sha256 -lcrypto
