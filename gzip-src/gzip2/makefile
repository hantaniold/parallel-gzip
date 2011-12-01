all: pgzip

pgzip:		gzip.o zip.o deflate.o trees.o bits.o util.o threads.o
		gcc -g -lpthread gzip.o zip.o deflate.o trees.o bits.o util.o threads.o -o pgzip

gzip.o:		gzip.c
		gcc -g -c gzip.c

zip.o:		zip.c
		gcc -g -c zip.c

deflate.o:	deflate.c
		gcc -g -c deflate.c

trees.o:	trees.c
		gcc -g -c trees.c

bits.o:		bits.c
		gcc -g -c bits.c

util.o:		util.c
		gcc -g -c util.c

threads.o:	threads.c
		gcc -g -c threads.c 

clean:		
		rm *.o
