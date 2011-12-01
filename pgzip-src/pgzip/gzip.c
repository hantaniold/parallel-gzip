#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "contexts.h"

char help_msg[] = 
"Usage: pgzip <input file> [FLAGS] \n \
  Flags can be: \n \
  -b <block size (bytes)> \n \
  -f (if output .gz file exists, force overwrite) \n \
  -t <# threads> \n";
					
// Global Context Variables Directly Used / Managed
//      int  ofd - output file descriptor
//      char* out_filepath - full path of output file
//      char* in_filepath - full path of input file
//      struct stat istat - status for input file
//      int compr_level - compression level we are using 
//      unsigned long long int ifile_size - Complete input file size in bytes
//      unsigned long long int bytes_to_read - total number of bytes available to be read (effectively (ifile_size - bytes_in))
//      int  ifd - input file descriptor
//      unsigned int decompress - 1 if we are decompressing, 0 if compressing
//      int (*work) (struct global_context*) - function to do the zipping or unzipping

// Functions Defined
//      int create_outfile(global_context* gc) - Create the Output File
//      void treatfile(global_context* gc) - Get Stats on the Input File
//      int abort_gzip() - Just kill the entire program right then and there
//      int main(int argc, char **argv) - Entry Point

// Defines Used
//      O_BINARY - #define O_BINARY        0
//      RW_USER - #define RW_USER (S_IRUSR | S_IWUSR)  /* creation mode for open() */


// Functions Used
//      int zip(global_context* gc) - See zip.c

int create_outfile(global_context* gc)
{
    struct stat	ostat;
    int flags = O_WRONLY | O_CREAT | O_EXCL | O_BINARY;

    gc->ofd = open(gc->out_filepath, flags, RW_USER);
    if(gc->ofd == -1)
    {
        close(gc->ifd);
        free(gc->out_filepath);
        free(gc->in_filepath);
        free(gc); abort_gzip();
    }
	if(stat(gc->out_filepath, &ostat) != 0)
    { close(gc->ofd); return -1; }

    return 0;
}

void treatfile(global_context* gc)
{
    if (stat(gc->in_filepath, &(gc->istat)) != 0)
    { printf("Could Not Stat File: %s\n", gc->in_filepath); free(gc); abort_gzip(); }

    gc->ifile_size = (gc->istat).st_size;
    gc->bytes_to_read = gc->ifile_size;
    gc->ifd = open(gc->in_filepath, !gc->decompress ? O_RDONLY : O_RDONLY | O_BINARY, RW_USER);
    gc->bytes_in = 0LL; gc->bytes_out = 0LL;    

    if (gc->ifd == -1) { printf("Could Not Open File: %s\n", gc->in_filepath); return; }
    //if (gc->decompress) { if (get_method(gc) < 0) { close(gc->ifd); return; } }
	if(create_outfile(gc) != 0) { close(gc->ifd); return; }

    //printf("Starting Work\n");
    (*(gc->work))(gc);
    //printf("Completeing Work\n");
    close(gc->ifd);
    close(gc->ofd);
}

int main(int argc, char **argv)
{
    if(argc < 2) { printf("%s",help_msg); return -1; }
 
    global_context* gc = (global_context*) malloc(sizeof(global_context)); gc->decompress = 0;
    
    gc->block_chunk_size = 100000;
    gc->number_of_threads = sysconf(_SC_NPROCESSORS_ONLN); 
/* Parse command line args */
	int i = 2;
	int canOverwriteOldOutput = 0; 
	while (i < argc) {
		if (strcmp(argv[i], "-f") == 0) {
			printf("Forced overwriting enabled.\n");
			canOverwriteOldOutput = 1;
		}

		if (strcmp(argv[i], "-b") == 0) {
			i += 1;
			sscanf(argv[i],"%ld",&gc->block_chunk_size);
			printf("Chunk size: %ld bytes. \n", gc->block_chunk_size);
		}

		if (strcmp(argv[i], "-t") == 0) {
			i += 1;
			sscanf(argv[i],"%d",&gc->number_of_threads);
			printf("Number of threads: %d\n",gc->number_of_threads);
		}
		if(strcmp(argv[i], "decompress") == 0) {
		   	gc->decompress = 1; 
			printf("Decompression Not Supported! Use standard GZIP.\n");	
			free(gc); 
			return 0;
		}
		i += 1;
	}

    char cwd[1024]; memset(cwd, 0, 1024);
    if((char*)getcwd(cwd, sizeof(cwd)-1) == NULL)
    { printf("Illegal CWD Result\n"); free(gc); return -1; }

    char* in_filepath = (char*)malloc(strlen(cwd) + 1 + strlen(argv[1]) + 1);
    memcpy(in_filepath, cwd, (int)strlen(cwd)); in_filepath[strlen(cwd)] = '/';
    strcpy(in_filepath + strlen(cwd) + 1, argv[1]);
    gc->in_filepath = in_filepath; gc->level = 6;

    
    gc->work = zip;
    char* out_filepath = (char*)malloc(strlen(in_filepath) + 4);
    memcpy(out_filepath, in_filepath, strlen(in_filepath));
    memcpy(out_filepath + strlen(in_filepath), ".gz\0", 4);
    gc->out_filepath = out_filepath;
	
	FILE * fp = fopen(gc->out_filepath, "r");	
	if (fp && canOverwriteOldOutput) {
		printf("Old file exists, deleting it!\n");
		fclose(fp);
		remove(gc->out_filepath);
	}	else if (fp) {
		char responseBuf[4];
		printf("Output file already exists. Overwrite? (y/n)\n");
		scanf("%s",responseBuf);
		if (strcmp(responseBuf,"y") == 0) {
			printf("Overwriting file...\n");
			remove(gc->out_filepath);
		}	else {
			exit(0);
		}	
	}
		

	
    printf("Input Path: %s\n", gc->in_filepath);
    printf("Output Path: %s\n", gc->out_filepath);
    /* Now we Have the Input Path and Output Path */
    treatfile(gc);
    return 0;
}

int abort_gzip() { exit(0); } /* Poor Man's Soln */
