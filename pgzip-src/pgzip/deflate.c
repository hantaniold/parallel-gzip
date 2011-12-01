#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "contexts.h"

	// Global Context Variables Directly Used / Managed
	//  int level - Compression Level to Use
	//  unsigned long block_chunk_size;         /* the size to chunk the input file by for each thread */ 
	//  unsigned long long int bytes_to_read;   /* total number of bytes available to be read (effectively (ifile_size - bytes_in)) */
	//  unsigned int last_block_number;         /* the last block number to flush out, 0 if not computed yet */
	//  unsigned int block_number;              /* the next block read in will be this index */
	//  unsigned int blocks_read;               /* number of input block reads currently with access to a thread
	//                                             or that hasn't been flushed out yet, if this number is greater
	//                                             than 2*number_of_threads, we stop reading in more information
	//  pthread_mutex_t output_fd_lock;
	//  queue* output_queue;
	//  pthread_cond_t more_io_output;
	//  int  ofd;
	//  unsigned int number_of_threads;         /* number of threads we want to use */
	//  _threadpool* pool;                      /* pool of worker threads */
	//  pthread_mutex_t output_block_lock;      /* lock for threads to add their output buffers back to processed_blocks */
	//  queue* thread_return_queue;
	//  sorted_linked_list* processed_blocks;   /* blocks returned by threads (they should be accessed with the lock) */
	//  unsigned int next_block_to_output;      /* next chunk that needs to be sent to the ofd */


	// Thread Context Variables Directly Used / Managed
	//  int attr;
	//  int method;
	//  ush deflate_flags;
	//  char* full_input_buffer;
	//  unsigned int full_input_buffer_size;
	//  char* full_output_buffer;
	//  unsigned int full_output_block_length;
	//  unsigned int block_number;
	//  int last_block;
	//  unsigned      lookahead;
	//  unsigned strstart;          /* window offset of current string */
	//  unsigned int prev_length;
	//  unsigned match_start;       /* window offset of current string */
	//  unsigned int max_lazy_match;
	//  uch* window;    /* Sliding window and suffix table (unlzw) */
	//  long block_start;
	//  int           eofile;        /* flag set at end of input file */
	//  unsigned good_match;
	//  ulg window_size;
	//  int nice_match;
	//  ush prev[WSIZE];
	//  unsigned max_chain_length;
	//  int compr_level;
	//  unsigned ins_h;

	// Functions Defined
	//      void lm_init (int pack_level, ush* flags, thread_context* tc) - Initialize the "longest match" 
	//                                                                      routines for a new worker thread
	//      void fill_window(thread_context* tc) - Fill the window when the lookahead becomes insufficient.
	//                                             Updates strstart and lookahead, and sets eofile if end of input file.
	//                                             IN assertion: lookahead < MIN_LOOKAHEAD && strstart + lookahead > 0
	//                                             OUT assertions: at least one byte has been read, or eofile is set;
	//                                             file reads are performed for at least two bytes (required for the
	//                                             translate_eol option).
	//      int longest_match(IPos cur_match, thread_context* tc) - return its length. Matches shorter or equal to prev_length 
	//                                                              are discarded, in which case the result is equal to prev_length
	//                                                              and match_start is garbage.
	//                                                              IN assertions: cur_match is the head of the hash chain for the current
	//                                                              string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
	//      void* deflate_work(void* arg) - This is the item that we give to the threadpool
	//      void thread_context_init(global_context* gc, thread_context* tc) - Initialize BitString and Trees Routines for Thread Context
	//      thread_context* grab_another_block(global_context* gc, thread_context* tc) - This will grab a block from the main file
	//                                                                                 and place it into the threads input buffers.
	//      void* io_out_function(void* arg) - This is the thread that outputs blocks to ofd
	//      ulg deflate(global_context* gc) - main entry point to setup threadpool and do core dispatching and maintenance
	//      config configuration_table[10] - Configuration Table For Different Compression Levels


	// Defines Used
	//      WSIZE - #define WSIZE 0x8000
	//      HASH_BITS - #define HASH_BITS  14
	//      HASH_SIZE - #define HASH_SIZE (unsigned)(1<<HASH_BITS)
	//      HASH_MASK - #define HASH_MASK (HASH_SIZE-1)
	//      WMASK - #define WMASK (WSIZE-1)
	//      NIL - #define NIL 0
	//      FAST - #define FAST 4
	//      SLOW - #define SLOW 2
	//      TOO_FAR - #define TOO_FAR 4096
	//      EQUAL - #define EQUAL 0
	//      MIN_LOOKAHEAD - #define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1) 
	//      MIN_MATCH - #define MIN_MATCH  3
	//      MAX_MATCH - #define MAX_MATCH  258
	//      MAX_DIST - #define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
	//      FLUSH_BLOCK - #define FLUSH_BLOCK(eof) \
	//                    flush_block(tc->block_start >= 0L ? (char*)&(tc->window[(unsigned)(tc->block_start)]) : (char*)NULL, (long)(tc->strstart - tc->block_start), (eof), tc)
	//      Assert - #define Assert(cond,msg) {if(!(cond)) error(msg);}
	//      IPos - typedef unsigned IPos
	//      check_match - #define check_match(start, match, length, tc)
	//      INSERT_STRING - #define INSERT_STRING(s, match_head) \
	//                      (UPDATE_HASH(tc->ins_h, tc->window[(s) + MIN_MATCH-1]), \
	//                      tc->prev[(s) & WMASK] = match_head = (tc->prev + WSIZE)[tc->ins_h], \
	//                      (tc->prev + WSIZE)[tc->ins_h] = (s))
	//      UPDATE_HASH - #define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

	// Functions Used
	//      void error(char *m) - See util.c
	//      int thread_read_buf(char *buf, unsigned long size, thread_context* tc) - See zip.c 
	//      int ct_tally (int dist, int lc, thread_context* tc) - See trees.c
	//      void bi_init(thread_context* tc) - See bits.c
	//      void ct_init(int* attr, int* methodp, thread_context* tc) - See trees.c
	//      int file_read(char *buf, unsigned long size, global_context* gc) - See zip.c
	//      queue* initialize_queue() - See threads.c 
	//      int queue_empty(const queue* q) - See threads.c
	//      void enqueue(queue* q, void* object) - See threads.c
	//      void* dequeue(queue* q) - See threads.c
	//      void write_error() - See util.c
	//      void dispatch(_threadpool* from_me, void* (*dispatch_to_here) (void*), void *arg) - See threads.c
	//      void insert_into_sorted_linked_list(sorted_linked_list* list_to_insert_into, int index, void* data) - See threads.c 
	//      void* pop_top(sorted_linked_list* list) - See threads.c
	//      ulg flush_block(char* buf, ulg stored_len, int eof, thread_context* tc) - See trees.c


	void thread_context_init(thread_context* tc)
	{
		bi_init(tc);
		ct_init(&(tc->attr), &(tc->method), tc);
		lm_init(tc->compr_level, &(tc->deflate_flags), tc);
	}

	thread_context* new_thread_context(global_context* gc)
	{
		thread_context* tc = (thread_context*) malloc(sizeof(thread_context));
		memset((char*)tc, 0, sizeof(thread_context));
		tc->full_input_buffer = (char*) malloc(gc->block_chunk_size);
		tc->full_output_vector = init_vector(gc->block_chunk_size, sizeof(char));
		tc->full_output_buffer_length = 0;
		tc->eofile = 0; tc->compressed_len = 0;
		tc->compr_level = gc->level;
		return tc;
	}

	thread_context* clean_old_thread_context(global_context* gc, thread_context* tc)
	{
		tc->full_output_buffer_length = 0;
		tc->eofile = 0; tc->compressed_len = 0;
		tc->full_output_vector->occupied_elements = 0;
		tc->full_output_buffer_length = 0;
		tc->eofile = 0; tc->compressed_len = 0;
		tc->compr_level = gc->level;
		return tc;
	}

	thread_context* grab_another_block(global_context* gc, thread_context* tc)
	{
		if(tc == NULL) tc = new_thread_context(gc);
		else tc = clean_old_thread_context(gc, tc);

		if(gc->bytes_to_read == 0 || gc->bytes_to_read <= gc->block_chunk_size)
			gc->last_block_number = gc->block_number;

		tc->block_number = gc->block_number;
		gc->block_number += 1;
		gc->blocks_read += 1;
		
		tc->l_desc.dyn_tree = tc->dyn_ltree;
		tc->l_desc.static_tree = tc->static_ltree;
		tc->l_desc.extra_bits = extra_lbits;
		tc->l_desc.extra_base = LITERALS+1;
		tc->l_desc.elems = L_CODES;
		tc->l_desc.max_length = MAX_BITS;
		tc->l_desc.max_code = 0;
		
		tc->d_desc.dyn_tree = tc->dyn_dtree;
		tc->d_desc.static_tree = tc->static_dtree;
		tc->d_desc.extra_bits = extra_dbits;
		tc->d_desc.extra_base = 0;
		tc->d_desc.elems = D_CODES;
		tc->d_desc.max_length = MAX_BITS;
		tc->d_desc.max_code = 0;
		
		tc->bl_desc.dyn_tree = tc->bl_tree;
		tc->bl_desc.static_tree = (ct_data*) 0;
		tc->bl_desc.extra_bits = extra_blbits;
		tc->bl_desc.extra_base = 0;
		tc->bl_desc.elems = BL_CODES;
		tc->bl_desc.max_length = MAX_BITS;
		tc->bl_desc.max_code = 0;
		
		tc->window_size = (ulg)2*WSIZE;
		
		//printf("Hello World!\n");
    if(gc->bytes_to_read > gc->block_chunk_size)
    {
        tc->full_input_buffer_size = gc->block_chunk_size;
        tc->last_block = 0;
        file_read(tc->full_input_buffer, gc->block_chunk_size, gc);
        gc->bytes_in += gc->block_chunk_size;
        gc->bytes_to_read -= gc->block_chunk_size;
    }

    else
    {
        tc->full_input_buffer_size = gc->bytes_to_read;        
        tc->last_block = 1;
        file_read(tc->full_input_buffer, gc->bytes_to_read, gc);
        gc->bytes_in += gc->bytes_to_read;
        gc->bytes_to_read = 0; 
    }
	
	tc->full_input_buffer_bytes_read = 0;
	tc->full_input_buffer_remaining_bytes = tc->full_input_buffer_size;

    return tc;
}


void* io_out_function(void* arg)
{
    global_context* gc = (global_context*) arg;
    queue* q = initialize_queue();

    while(1)
    {
        pthread_mutex_lock(&(gc->output_fd_lock));
        while(queue_empty(gc->output_queue) && gc->kill_output_io_thread != 1)
            pthread_cond_wait(&(gc->more_io_output), &(gc->output_fd_lock));

		if(gc->kill_output_io_thread == 1 && queue_empty(gc->output_queue))
		{
			pthread_mutex_unlock(&(gc->output_fd_lock));
			break;
		}
		
        while(!queue_empty(gc->output_queue))
            enqueue(q, dequeue(gc->output_queue));

        pthread_mutex_unlock(&(gc->output_fd_lock));
        
        while(!queue_empty(q))
        {
            quick_data* data = (quick_data*) dequeue(q);

            unsigned int written_bytes = 0;
            unsigned int total_bytes = data->length; 
            unsigned int remaining_bytes = total_bytes;

            while(remaining_bytes != 0)
            {
                unsigned int n = write(gc->ofd, data->buffer + written_bytes, remaining_bytes);
	            if (n == (unsigned)-1) write_error();
                remaining_bytes -= n; written_bytes += n;
	        }
           // printf("Flushed a Block!\n");

            //free(data->buffer); free(data);
        }
        
        if(gc->kill_output_io_thread == 1) { break; }
    }
}

ulg deflate(global_context* gc)
{
    pthread_t io_out_thread;
    pthread_create(&io_out_thread, NULL, io_out_function, (void*)gc);
    int i; int first_pass = 0; int quit_flag = 0;
    
    for(i = 0; i != gc->number_of_threads; i++)
    {
		if(gc->bytes_to_read > 0)
        {	
            //printf("GC->bytes_to_read = %llu\n", gc->bytes_to_read);
			thread_context* tc = grab_another_block(gc, NULL);
            thread_context_init(tc);
			if(tc == NULL) { break; }
            //printf("HSITTTTTTTTTTGAFD\n");
			dispatch(gc->pool, deflate_work, (void*)tc);
		}
		else break;
    }

    //printf("Done Dispatching Initial Batches\n");
    queue* temp = initialize_queue();
    while(1)
    {
        //printf("Entered While Loop\n");
        pthread_mutex_lock(&(gc->pool->pending_job_requests_lock));
        while(queue_empty(gc->pool->pending_job_requests) && queue_empty(gc->pool->completed_threads) && !(gc->pool->shutdown))
            pthread_cond_wait(&(gc->pool->pending_job_requests_cond), &(gc->pool->pending_job_requests_lock));
            
       // printf("Got Lock!\n");
        while(!queue_empty(gc->pool->pending_job_requests) && !queue_empty(gc->pool->free_threads))
        {
            spec_thread* th = dequeue(gc->pool->free_threads);
            work_t* work = dequeue(gc->pool->pending_job_requests);
           // printf("Thread dequeued as free thread %d\n", th->thread_id);
            Assert(th->busy == 0, "Why is thread Busy?");
            pthread_mutex_lock(&th->thread_lock);
           // printf("Got internal lock!\n");
            th->work = (void*) work; th->busy = 1;
            spec_thread** p = ((spec_thread**) gc->pool->busy_threads);
            p[th->thread_id] = th;
            pthread_mutex_unlock(&th->thread_lock);
           // printf("Can I acquire Lock?\n");
            pthread_cond_signal(&th->thread_cond);
        }
        
        pthread_mutex_unlock(&(gc->pool->pending_job_requests_lock));
       // printf("Freed Lock\n");

        if(!queue_empty(gc->pool->completed_threads))
        {
            pthread_mutex_lock(&(gc->pool->completed_threads_lock));
            while(!queue_empty(gc->pool->completed_threads))
            {
               // printf("Grabbed A completed Block\n");
                int* it = (int*) dequeue(gc->pool->completed_threads);
               // printf("Thread Number: %d", *it);
                spec_thread** s = (spec_thread**) (gc->pool->busy_threads);
               // printf("Successful Cast\n");
                spec_thread* mythread = (spec_thread*) s[*it];
//                printf("mythread->thread_id: %d\n", mythread->thread_id);
                work_t* work = (work_t*) mythread->work;
                //if(work == NULL) { gc->pool->shutdown = 1; printf("Null Work\n"); break; }
               // printf("Hi1\n");
                thread_context* tc = (thread_context*) (work->arg);
               // printf("Hi2 Block Number %d\n", tc->block_number);                
                quick_data* q = (quick_data*) malloc(sizeof(quick_data));
                q->buffer = (char*)malloc(tc->full_output_vector->total_elements);
                memcpy(q->buffer, tc->full_output_vector->elements, tc->full_output_vector->occupied_elements * tc->full_output_vector->element_size);
                q->length = tc->full_output_vector->occupied_elements;
              //  printf("Length: %d    Block Number: %d\n", q->length, tc->block_number);
               // printf("Dispatched Another Block\n");
                
                insert_into_sorted_linked_list(gc->processed_blocks, tc->block_number, (void*)q);
                //printf("Inserted Block into linked list\n");
                
                
                enqueue(gc->pool->free_threads, mythread);
                
                if(gc->bytes_to_read != 0)
                {        
                    tc = grab_another_block(gc, tc);
                    thread_context_init(tc);
                    dispatch(gc->pool, deflate_work, (void*)tc);
                }
                
                else { gc->pool->shutdown = 1; }
                
                
               // printf("Queued back free thread\n");
            }
            pthread_mutex_unlock(&(gc->pool->completed_threads_lock));
        }
    
        
        first_pass = 0;
        if(gc->processed_blocks->head != NULL)
        {
           // printf("Handling IO %d : %d\n", gc->processed_blocks->head->index, gc->next_block_to_output);
            print_sorted_linked_list(gc->processed_blocks);
            while(gc->processed_blocks->head->index == gc->next_block_to_output)
            {
               // printf("%d : %d\n", gc->processed_blocks->head->index, gc->next_block_to_output);
                if(gc->last_block_number == gc->processed_blocks->head->index) { quit_flag = 1; }
                if(first_pass == 0)
                {   pthread_mutex_lock(&(gc->output_block_lock));
                    first_pass = 1; }

                gc->next_block_to_output += 1;
                gc->blocks_read -= 1;
                void* block = (void*) pop_top(gc->processed_blocks);
                enqueue(gc->output_queue, block);
                if(gc->processed_blocks->head == NULL) { break; }
            }
            
            if(first_pass == 1) { pthread_mutex_unlock(&gc->output_block_lock); }
        }
        
       // printf("Remaing Bytes: %llu\n", gc->bytes_to_read);
        if(gc->pool->shutdown == 1 && gc->pool->free_threads->size == gc->number_of_threads) break;
    }

	void* status;
	gc->kill_output_io_thread = 1;
	pthread_cond_signal(&(gc->more_io_output));
	pthread_join(io_out_thread, &status);
}




















/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
void* deflate_work(void* arg)
{
   // printf("Started Deflate Work\n");
    thread_context* tc = (thread_context*)arg;
    IPos hash_head = 0;          /* head of hash chain */
    IPos prev_match;         /* previous match */
    int flush;               /* set if current block must be flushed */
    int match_available = 0; /* set if previous match exists */
    register unsigned match_length = MIN_MATCH-1; /* length of best match */

    /* Process the input block. */
    while (tc->lookahead != 0) {
        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        //printf("Lookahead: %d + Remaining Bytes: %u\n", tc->lookahead, tc->full_input_buffer_remaining_bytes);
        INSERT_STRING(tc->strstart, hash_head);


        /* Find the longest match, discarding those <= prev_length.
         */
        tc->prev_length = match_length, prev_match = tc->match_start;
        match_length = MIN_MATCH-1;

        if (hash_head != NIL && tc->prev_length < tc->max_lazy_match &&
            tc->strstart - hash_head <= MAX_DIST) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            match_length = longest_match(hash_head, tc);
            /* longest_match() sets match_start */
            if (match_length > tc->lookahead) match_length = tc->lookahead;

            /* Ignore a length 3 match if it is too distant: */
            if (match_length == MIN_MATCH && tc->strstart-tc->match_start > TOO_FAR){
                /* If prev_match is also MIN_MATCH, match_start is garbage
                 * but we will ignore the current match anyway.
                 */
                match_length--;
            }
        }
        /* If there was a match at the previous step and the current
         * match is not better, output the previous match:
         */
        //abort_gzip();
        if (tc->prev_length >= MIN_MATCH && match_length <= tc->prev_length) {

            check_match(tc->strstart-1, prev_match, tc->prev_length, tc);

            flush = ct_tally(tc->strstart-1-prev_match, tc->prev_length - MIN_MATCH, tc);

            /* Insert in hash table all strings up to the end of the match.
             * strstart-1 and strstart are already inserted.
             */
            tc->lookahead -= tc->prev_length-1;
            tc->prev_length -= 2;
            do {
                tc->strstart++;
                INSERT_STRING(tc->strstart, hash_head);
                /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                 * always MIN_MATCH bytes ahead. If lookahead < MIN_MATCH
                 * these bytes are garbage, but it does not matter since the
                 * next lookahead bytes will always be emitted as literals.
                 */
            } while (--tc->prev_length != 0);
            match_available = 0;
            match_length = MIN_MATCH-1;
            tc->strstart++;
            if (flush) FLUSH_BLOCK(0), tc->block_start = tc->strstart;

        } else if (match_available) {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            //Tracevv((stderr,"%c", tc->window[tc->strstart-1]));
            if (ct_tally(0, tc->window[tc->strstart-1], tc)) {
                FLUSH_BLOCK(0), tc->block_start = tc->strstart;
            }

            tc->strstart++;
            tc->lookahead--;
        } else {
            /* There is no previous match to compare with, wait for
             * the next step to decide.
             */
            match_available = 1;
            tc->strstart++;
            tc->lookahead--;
        }
        //Assert (tc->strstart <= isize && lookahead <= isize, "a bit too far");

        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        //printf("Above While\n");
        while (tc->lookahead < MIN_LOOKAHEAD && !tc->eofile) fill_window(tc);
        //printf("below while\n");
    }
    //printf("Exited While\n");
    if(match_available) ct_tally (0, tc->window[tc->strstart-1], tc);
    //printf("Done Tally\n");

    //printf("Last Block: %d", tc->last_block);
    if(tc->last_block) FLUSH_BLOCK(1); /* eof */
    else FLUSH_BLOCK(0);
    flush_outbuf(tc);

    //printf("Completeing Deflate Work");	
    return NULL;
}








int longest_match(IPos cur_match, thread_context* tc)
{
    unsigned chain_length = tc->max_chain_length;       /* max hash chain length */
    register uch *scan = tc->window + tc->strstart;     /* current string */
    register uch *match;                                /* matched string */
    register int len;                                   /* length of current match */
    int best_len = tc->prev_length;                     /* best match length so far */
    IPos limit = tc->strstart > (IPos)MAX_DIST ? tc->strstart - (IPos)MAX_DIST : NIL;

    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */

/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
 * It is easy to get rid of this optimization if necessary.
 */


    register uch *strend = tc->window + tc->strstart + MAX_MATCH - 1;
    register ush scan_start = *(ush*)scan;
    register ush scan_end   = *(ush*)(scan+best_len-1);

    /* Do not waste too much time if we already have a good match: */
    if (tc->prev_length >= tc->good_match) { chain_length >>= 2; }
    Assert(tc->strstart <= tc->window_size-MIN_LOOKAHEAD, "insufficient lookahead");

    do {
        Assert(cur_match < tc->strstart, "no future");
        match = tc->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2:
         */

        /* This code assumes sizeof(unsigned short) == 2. Do not use
         * UNALIGNED_OK if your compiler uses a different size.
         */
        if (*(ush*)(match+best_len-1) != scan_end ||
            *(ush*)match != scan_start) continue;

        /* It is not necessary to compare scan[2] and match[2] since they are
         * always equal when the other bytes match, given that the hash keys
         * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
         * strstart+3, +5, ... up to strstart+257. We check for insufficient
         * lookahead only every 4th comparison; the 128th check will be made
         * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
         * necessary to put more guard bytes at the end of the window, or
         * to check more often for insufficient lookahead.
         */
        scan++, match++;
        do {
        } while (*(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 *(ush*)(scan+=2) == *(ush*)(match+=2) &&
                 scan < strend);
        /* The funny "do {}" generates better code on most compilers */

        /* Here, scan <= window+strstart+257 */
        //Assert(scan <= tc->window + (unsigned)(tc->window_size-1), "wild scan");
        if (*scan == *match) scan++;

        len = (MAX_MATCH - 1) - (int)(strend-scan);
        scan = strend - (MAX_MATCH-1);


        if (len > best_len) {
            tc->match_start = cur_match;
            best_len = len;
            if (len >= tc->nice_match) break;
            scan_end = *(ush*)(scan+best_len-1);

        }
    } while ((cur_match = tc->prev[cur_match & WMASK]) > limit
	     && --chain_length != 0);

    return best_len;
}







void lm_init (int pack_level, ush* flags, thread_context* tc)
{
    unsigned int j;

    if (pack_level < 1 || pack_level > 9) error("bad pack level");
    tc->compr_level = pack_level;


    /* Initialize the hash table. */
    for (j = 0;  j < HASH_SIZE; j++) tc->head[j] = NIL;

    /* Set the default configuration parameters:
     */
    tc->max_lazy_match   = configuration_table[pack_level].max_lazy;
    tc->good_match       = configuration_table[pack_level].good_length;
    tc->nice_match       = configuration_table[pack_level].nice_length;
    tc->max_chain_length = configuration_table[pack_level].max_chain;
    if (pack_level == 1) {
       *flags |= FAST;
    } else if (pack_level == 9) {
       *flags |= SLOW;
    }
    /* ??? reduce max_chain_length for binary files */

    tc->strstart = 0;
    tc->block_start = 0L;

    tc->lookahead = thread_read_buf((char*)(tc->window), 2*WSIZE, tc);

    if(tc->lookahead == 0 || tc->lookahead == (unsigned)EOF) {
       tc->eofile = 1, tc->lookahead = 0;
       return;
    }

    tc->eofile = 0;
    /* Make sure that we always have enough lookahead. This is important
     * if input comes from a device such as a tty.
     */
    while (tc->lookahead < MIN_LOOKAHEAD && !(tc->eofile)) fill_window(tc);

    tc->ins_h = 0;
    for (j=0; j<MIN_MATCH-1; j++) UPDATE_HASH(tc->ins_h, tc->window[j]);
    /* If lookahead < MIN_MATCH, ins_h is garbage, but this is
     * not important since only literal bytes will be emitted.
     */
}

void fill_window(thread_context* tc)
{
    register unsigned n, m;
    unsigned more = (unsigned)((tc->window_size) - (ulg)(tc->lookahead) - (ulg)(tc->strstart));
    /* Amount of free space at the end of the window. */

    /* If the window is almost full and there is insufficient lookahead,
     * move the upper half to the lower one to make room in the upper half.
     */
    if (more == (unsigned)EOF) {
        /* Very unlikely, but possible on 16 bit machine if strstart == 0
         * and lookahead == 1 (input done one byte at time)
         */
        more--;
    } else if (tc->strstart >= WSIZE+MAX_DIST) {
        /* By the IN assertion, the window is not empty so we can't confuse
         * more == 0 with more == 64K on a 16 bit machine.
         */
        //printf("Window Size: %lu, %lu", tc->window_size, (ulg)2*WSIZE);
        Assert(tc->window_size == (ulg)2*WSIZE, "no sliding with BIG_MEM");

        memcpy((char*)(tc->window), (char*)tc->window+WSIZE, (unsigned)WSIZE);
        tc->match_start -= WSIZE;
        tc->strstart    -= WSIZE; /* we now have strstart >= MAX_DIST: */

        tc->block_start -= (long) WSIZE;

        for (n = 0; n < HASH_SIZE; n++) {
            m = tc->head[n];
            tc->head[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
        }
        for (n = 0; n < WSIZE; n++) {
            m = tc->prev[n];
            tc->prev[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
            /* If n is not on any hash chain, prev[n] is garbage but
             * its value will never be used.
             */
        }
        more += WSIZE;
    }
    /* At this point, more >= 2 */
    if (!tc->eofile) {
        n = thread_read_buf((char*)tc->window + tc->strstart + tc->lookahead, more, tc);
        if (n == 0 || n == (unsigned)EOF) {
            tc->eofile = 1;
        } else {
            tc->lookahead += n;
        }
    }
}


config configuration_table[10] = {
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0},  /* store only */
/* 1 */ {4,    4,  8,    4},  /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,    8},
/* 3 */ {4,    6, 32,   32},

/* 4 */ {4,    4, 16,   16},  /* lazy matches */
/* 5 */ {8,   16, 32,   32},
/* 6 */ {8,   16, 128, 128},
/* 7 */ {8,   32, 128, 256},
/* 8 */ {32, 128, 258, 1024},
/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */
