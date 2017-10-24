PREFIX?=/usr/local

all: jpeg_metadata_size jpeg_metadata jpeg_sha256

install: all
	install -D jpeg_metadata_size ${PREFIX}/bin
	install -D jpeg_metadata ${PREFIX}/bin
	install -D jpeg_sha256 ${PREFIX}/bin

uninstall:
	rm ${PREFIX}/bin/jpeg_metadata_size
	rm ${PREFIX}/bin/jpeg_metadata
	rm ${PREFIX}/bin/jpeg_sha256

jpeg_metadata_size: jpeg_metadata_size.c  Makefile
	gcc -std=gnu11 jpeg_metadata_size.c -O9 -o jpeg_metadata_size

jpeg_metadata: jpeg_metadata.c  Makefile
	gcc -std=gnu11 jpeg_metadata.c -O9 -o jpeg_metadata 

jpeg_sha256: jpeg_sha256.c  Makefile
	gcc -std=gnu11 jpeg_sha256.c sha256sum.c -O9 -o jpeg_sha256 -lcrypto

.PHONY: all install uninstall
