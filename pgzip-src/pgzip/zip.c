#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "contexts.h"

// Global Context Variables Directly Used / Managed
//      unsigned long crc - current crc checksum for the bytes read in
//      unsigned long long int bytes_in - Total number of bytes currently read in from file
//      _threadpool* pool - pool of worker threads
//      unsigned int number_of_threads - number of threads we want to use
//      unsigned long block_chunk_size - the size to chunk the input file by for each thread 
//      queue* thread_return_queue - queue where threads push back completed zipped blocks
//      queue* output_queue - queue for main io thread to push to output fd
//      sorted_linked_list* processed_blocks - blocks returned by threads which may not be sent to ofd since they may arrive out of order
//      pthread_mutex_t output_block_lock - lock for threads to add their output buffers back to processed_blocks
//      pthread_mutex_t output_fd_lock - lock to push data from this io thread to the output io thread
//      pthread_cond_t take_io_action - This condition variable will be signaled whenever a thread adds to the processed_blocks list
//      pthread_cond_t more_io_output - This condition variable will be signaled whenever we place more output to be sent to ofd
//      unsigned long header_bytes - number of bytes in gzip header
//      unsigned int bytes_out - total number of bytes sent out to ofd
//
//      int ifd - See gzip.c
//      int ofd - See gzip.c
//      char* in_filepath - See gzip.c
//      struct stat istat - See gzip.c

// Thread Context Variables Directly Used / Managed
//      unsigned int full_input_buffer_remaining_bytes   - Number of bytes that are still available from the handle
//      unsigned int full_input_buffer_bytes_read        - Number of bytes read from the pseudo handle
//	    char* full_input_buffer				 - See deflate.c

// Functions Defined
//      int zip(global_context* gc) - Function that does the heavy lifting
//      int thread_read_buf(char *buf, unsigned long size, thread_context* tc) - Read a new buffer from 
//                                                                               the threads store buffer
//      int file_read(char *buf, unsigned long size, global_context* gc) - Read a new buffer from 
//                                      the current input file, perform end-of-line translation, 
//                                      and update the crc and input file size.
//                                      IN assertion: size >= 2 (for end-of-line translation)

// Defines Used
//      GZIP_MAGIC - #define GZIP_MAGIC "\037\213"
//      DEFLATED - #define DEFLATED   8
//      ORIG_NAME - #define ORIG_NAME 0x08 
//      OS_CODE - #define OS_CODE     0x00
//      OK - #define OK               0
//

// Functions Used
//      ulg updcrc(uch* s, unsigned n) - See util.c
//      sorted_linked_list* init_sorted_linked_list() - See threads.c
//      queue* initialize_queue() - See threads.c
//      threadpool* create_threadpool(unsigned int num_threads_in_pool) - See threads.c
//      ulg deflate(global_context* gc) - See deflate.c

int zip(global_context* gc)
{
    char *p = (char*) basename(gc->in_filepath);
    long length = strlen(p);
    long header_length = 4 + 4 + 1 + 1 + length + 1 + 4 + 4;
    char* headerbuffer = (char*)malloc(header_length);
    /* Write the header to the gzip file. See algorithm.doc for the format */
    headerbuffer[0] = GZIP_MAGIC[0];
    headerbuffer[1] = GZIP_MAGIC[1];
    headerbuffer[2] = DEFLATED;
    headerbuffer[3] = ORIG_NAME;

    unsigned int time_stamp = gc->istat.st_mtime;
    headerbuffer[4] = (char)(time_stamp);
    headerbuffer[5] = (char)(time_stamp >> 8);
    headerbuffer[6] = (char)(time_stamp >> 16);
    headerbuffer[7] = (char)(time_stamp >> 24);

    /* Write deflated file to zip file */
    gc->crc = updcrc(0, 0); gc->blocks_read = 0;
    gc->pool = create_threadpool(gc->number_of_threads);
    gc->thread_return_queue = initialize_queue();
    gc->output_queue = initialize_queue();
    gc->processed_blocks = init_sorted_linked_list();
    gc->kill_output_io_thread = 0;
    pthread_mutex_init(&(gc->output_block_lock), NULL);
    pthread_mutex_init(&(gc->output_fd_lock), NULL);
    pthread_cond_init(&(gc->take_io_action), NULL);
    pthread_cond_init(&(gc->more_io_output), NULL);

    headerbuffer[8] = 0;
    headerbuffer[9] = OS_CODE;

    int i = 10;
	do { headerbuffer[i] = *p; i++; } while (*p++);
    headerbuffer[i] = 0; i += 1;

    gc->header_bytes = (long)header_length;
    gc->bytes_out = header_length;
    write(gc->ofd, headerbuffer, header_length - 8);
    gc->next_block_to_output = 1;
    gc->last_block_number = 0;
    gc->block_number = 1;

    //printf("Starting Deflation\n");
    (void)deflate(gc);
    //printf("Completed Deflation\n");
    

    /* Write the crc and uncompressed size */
    headerbuffer[i] = (char)(gc->crc);
    headerbuffer[i + 1] = (char)(gc->crc >> 8);
    headerbuffer[i + 2] = (char)(gc->crc >> 16);
    headerbuffer[i + 3] = (char)(gc->crc >> 24);
    
    headerbuffer[i + 4] = (char)(gc->ifile_size);
    headerbuffer[i + 5] = (char)(gc->ifile_size >> 8);
    headerbuffer[i + 6] = (char)(gc->ifile_size >> 16);
    headerbuffer[i + 7] = (char)(gc->ifile_size >> 24);
    write(gc->ofd, headerbuffer + i, 8);
    return OK;
}

unsigned int thread_read_buf(char *buf, unsigned int size, thread_context* tc)
{
    //printf("Reading %u\n", size);
    
    if(size > tc->full_input_buffer_remaining_bytes)
    { size = tc->full_input_buffer_remaining_bytes; }

    //printf("Trying to Read %u Bytes from Buffer of Size %u\n", size, tc->full_input_buffer_remaining_bytes);

    tc->full_input_buffer_remaining_bytes -= size;
    memcpy(buf, tc->full_input_buffer + tc->full_input_buffer_bytes_read, size);
    tc->full_input_buffer_bytes_read += size;
    return size;
}


unsigned int file_read(char *buf, unsigned int size, global_context* gc)
{
    unsigned int len;
    unsigned int total_bytes_read = 0;
    unsigned int total_bytes_left = size;

    while(total_bytes_left != 0)
    {
        len = read(gc->ifd, buf + total_bytes_read, total_bytes_left);
        if (len == (uin)(-1)) error("read error");
        total_bytes_left -= len;
        total_bytes_read += len;
    }
        
    gc->crc = updcrc((uch*)buf, total_bytes_read);
    //printf("Total_Bytes_Read: %d + CRC: %lu", total_bytes_read, gc->crc);
    gc->bytes_in += (unsigned long long int)size;
    //printf("Total Size: %llu\n", gc->bytes_in);
    return size;
}
