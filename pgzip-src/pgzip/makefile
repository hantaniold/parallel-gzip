all: pgzip

debug:		gzip.c zip.c bits.c deflate.c util.c trees.c threads.c
		gcc -g -lpthread gzip.c bits.c deflate.c util.c trees.c threads.c zip.c -o debug_pgzip

pgzip:		gzip.c zip.c deflate.c trees.c bits.c util.c threads.c
		gcc -O3 -lpthread gzip.c zip.c deflate.c trees.c bits.c util.c threads.c -o pgzip

clean:		
		rm *.o
