#ifndef CONTEXTS
#define CONTEXTS
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include "threads.h"


// Typedefs

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned int   uin;
typedef unsigned long  ulg;

typedef uin IPos;
typedef ush Pos;


// Defines

// gzip.c
#define O_BINARY    0
#define RW_USER     (S_IRUSR | S_IWUSR)  /* creation mode for open() */
#define INBUF_EXTRA  64     /* required by unlzw() */
#define OUTBUF_EXTRA 2048   /* required by unlzw() */


// zip.c
#define GZIP_MAGIC  "\037\213"
#define DEFLATED    8
#define ORIG_NAME   0x08 
#define OS_CODE     0x00
#define OK          0

// deflate.c
#define WSIZE       0x8000
#define HASH_BITS   14
#define HASH_SIZE   (uin)(1<<HASH_BITS)
#define HASH_MASK   (HASH_SIZE-1)
#define WMASK       (WSIZE-1)
#define NIL         0
#define FAST        4
#define SLOW        2
#define TOO_FAR     4096
#define EQUAL       0
#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1) 
#define MIN_MATCH   3
#define MAX_MATCH   258
#define MAX_DIST    (WSIZE-MIN_LOOKAHEAD)
#define FLUSH_BLOCK(eof) flush_block(tc->block_start >= 0L ? (char*)&(tc->window[(uin)(tc->block_start)]) : (char*)NULL, (long)tc->strstart - tc->block_start, (eof), tc)
#define Assert(cond,msg) {if(!(cond)) error(msg);}
#define check_match(start, match, length, tc)
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(tc->ins_h, tc->window[(s) + MIN_MATCH-1]), \
    tc->prev[(s) & WMASK] = match_head = tc->head[tc->ins_h], \
    tc->head[tc->ins_h] = (s))
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)
#define LIT_BUFSIZE  0x8000
#define DIST_BUFSIZE 0x8000 /* buffer for distances, see trees.c */
#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)


// trees.c
#define MAX_BITS 15
#define MAX_BL_BITS 7
#define LENGTH_CODES 29
#define LITERALS  256
#define END_BLOCK 256
#define L_CODES (LITERALS+1+LENGTH_CODES)
#define D_CODES   30
#define BL_CODES  19
#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
#define REP_3_6      16
#define REPZ_3_10    17
#define REPZ_11_138  18
#define SMALLEST 1
#define l_buf inbuf
#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len
#define HEAP_SIZE (2*L_CODES+1)
#define send_code(c, tree) send_bits(tree[c].Code, tree[c].Len, tc)
#define d_code(dist) ((dist) < 256 ? tc->dist_code[dist] : tc->dist_code[256+((dist)>>7)])
#define MAX(a,b) (a >= b ? a : b)
#define pqremove(tree, top) \
                 { top = tc->heap[SMALLEST]; \
                  tc->heap[SMALLEST] = tc->heap[tc->heap_len--]; \
                  pqdownheap(tree, SMALLEST, tc); }
#define smaller(tree, n, m) \
               (tree[n].Freq < tree[m].Freq || \
               (tree[n].Freq == tree[m].Freq && tc->depth[n] <= tc->depth[m]))
#define L_CODES (LITERALS+1+LENGTH_CODES)
#define UNKNOWN 0xffff
#define BINARY 0
#define ASCII 1

#define STORED 0
#define COMPRESSED 1


// util.c
#define INBUFSIZ  0x8000


// bits.c
#define put_short(w) \
        { if (tc->outcnt < OUTBUFSIZ-2) { \
          tc->outbuf[tc->outcnt++] = (uch) ((w) & 0xff); \
          tc->outbuf[tc->outcnt++] = (uch) ((ush)(w) >> 8); \
        } else { \
          put_byte((uch)((w) & 0xff)); \
          put_byte((uch)((ush)(w) >> 8)); \
        } }
#define put_byte(c) {tc->outbuf[tc->outcnt++]=(uch)(c); if (tc->outcnt==OUTBUFSIZ) flush_outbuf(tc);}
#define Buf_size (8 * 2*sizeof(char))
#define OUTBUFSIZ  16384

// Corrections
#define Trace(x) {if(verbose) fprintf x ;}  
#define Tracev(x) {if (verbose) fprintf x ;}
#define Tracevv(x) {if (verbose) fprintf x; }
#define verbose 0
#define seekable() 0









// Structures

 typedef struct config {
    ush good_length; /* reduce lazy search above this match length */
    ush max_lazy;    /* do not perform lazy search above this match length */
    ush nice_length; /* quit search above this match length */
    ush max_chain;
 } config;

/* Data structure describing a single value and its code string. */
typedef struct ct_data {
    union {
        ush  freq;       /* frequency count */
        ush  code;       /* bit string */
    } fc;
    union {
        ush  dad;        /* father node in Huffman tree */
        ush  len;        /* length of bit string */
    } dl;
} ct_data;

typedef struct tree_desc {
    ct_data *dyn_tree;      /* the dynamic tree */
    ct_data *static_tree;   /* corresponding static tree or NULL */
    int     *extra_bits;    /* extra bits for each code or NULL */
    int     extra_base;          /* base index for extra_bits */
    int     elems;               /* max number of elements in the tree */
    int     max_length;          /* max bit length for the codes */
    int     max_code;            /* largest code with non zero frequency */
} tree_desc;

typedef struct global_context
{
    // Assigned in gzip.c
    int level;                              /* Compression Level */

    // Assigned in gzip.c
    int  ifd;                               /* input file descriptor */

    // Assigned in gzip.c
    int  ofd;                               /* output file descriptor */

    // Assigned in gzip.c
    char* in_filepath;                      /* full path of input file */

    // Assigned in gzip.c
    char* out_filepath;                     /* full path of output file */

    // Assigned in gzip.c
    struct stat istat;                      /* status for input file */

    // Assigned in gzip.c
    unsigned int decompress;                /* 1 if we are decompressing, 0 if compressing */

    // Assigned in gzip.c
    unsigned long long int ifile_size;      /* Complete input file size in bytes */

    // Assigned in gzip.c
    unsigned long long int bytes_to_read;   /* total number of bytes available to be read (effectively (ifile_size - bytes_in)) */

    // Assigned in gzip.c
    unsigned long long int bytes_in;        /* total number of bytes read in from the input file */

    // Assigned in gzip.c
    unsigned long long int bytes_out;       /* total number of bytes sent out to the output file */

    // Assigned in zip.c
    unsigned long header_bytes;             /* number of bytes in gzip header */

    // Assigned in zip.c
    unsigned int last_block_number;         /* the last block number to flush out, 0 if not computed yet */

    // Assigned in zip.c
    unsigned int block_number;              /* the next block read in will be this index */

    // Assigned in gzip.c
    unsigned long block_chunk_size;         /* the size to chunk the input file by for each thread */ 

    // Assigned in gzip.c
    unsigned int number_of_threads;         /* number of threads we want to use */

    // Assigned in zip.c
    unsigned int next_block_to_output;      /* next chunk that needs to be sent to the ofd */

    // Assigned in zip.c
    threadpool* pool;                      /* pool of worker threads */

    // Assigned in zip.c
    unsigned int blocks_read;               /* number of input block reads currently with access to a thread
                                               or that hasn't been flushed out yet, if this number is greater
                                               than 2*number_of_threads, we stop reading in more information */

    // Assigned in zip.c
    pthread_mutex_t output_block_lock;      /* lock for threads to add their output buffers back to processed_blocks */

    // Assigned in zip.c
    queue* thread_return_queue;

    // Assigned in zip.c
    pthread_cond_t take_io_action;          /* This condition variable will be signaled whenever a thread adds to the processed_blocks list */

    // Assigned in zip.c
    sorted_linked_list* processed_blocks;   /* blocks returned by threads (they should be accessed with the lock) */

    // Assigned in zip.c
    pthread_mutex_t output_fd_lock;

    // Assigned in zip.c
    queue* output_queue;

    // Assigned in zip.c
    pthread_cond_t more_io_output;

    // Assigned in gzip.c
    int (*work) (struct global_context*);   /* function to do the zipping or unzipping */

    // Assigned in zip.c
    unsigned long crc;                      /* current crc checksum for the bytes read in */

	int kill_output_io_thread;				/* Set this to one to signal the output io thread to exit */
} global_context;

typedef struct quick_data
{
    unsigned int length;
    char* buffer;
} quick_data;


typedef struct thread_context
{
    // zip.c
    unsigned int full_input_buffer_remaining_bytes;
    unsigned int full_input_buffer_bytes_read;
    
    // deflate.c
    ush attr;
    int method;
    ush deflate_flags;
    char* full_input_buffer;
    unsigned int full_input_buffer_size;
	vector* full_output_vector;
    unsigned int full_output_buffer_length;
    unsigned int block_number;
    int last_block;
    uin lookahead;
    uin strstart;          /* window offset of current string */
    unsigned int prev_length;
    unsigned int match_start;       /* window offset of current string */
    unsigned int max_lazy_match;
    uch window[2L*WSIZE];    /* Sliding window and suffix table (unlzw) */
    long block_start;
    int           eofile;        /* flag set at end of input file */
    unsigned good_match;
    ulg window_size;
    int nice_match;
    ush prev[WSIZE];
    ush head[(1<<HASH_BITS)];
    unsigned max_chain_length;
    int compr_level;
    unsigned ins_h;
    
    // trees.c
    ush* file_type;
    int* file_method;
    unsigned int compressed_len;
    unsigned int input_len;
    ct_data static_ltree[L_CODES + 2];
    ct_data static_dtree[D_CODES];
    int base_length[LENGTH_CODES];
    uch length_code[MAX_MATCH-MIN_MATCH+1];
    int base_dist[D_CODES];
    uch dist_code[512];
    ush bl_count[MAX_BITS+1];
    ct_data dyn_ltree[HEAP_SIZE];   /* literal and length tree */
    ct_data dyn_dtree[2*D_CODES+1]; /* distance tree */
    ct_data bl_tree[2*BL_CODES+1];
    ulg opt_len;        /* bit length of current block with optimal trees */
    ulg static_len;     /* bit length of current block with static trees */
    unsigned last_lit;    /* running index in l_buf */
    unsigned last_dist;   /* running index in d_buf */
    unsigned last_flags;  /* running index in flag_buf */
    ush flags;            /* current flags not yet saved in flag_buf */
    uch flag_bit;         /* current bit used in flags */
    uch flag_buf[(LIT_BUFSIZE/8)];
    int heap_len;               /* number of elements in the heap */
    int heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
    int heap_max;               /* element of largest frequency */
    tree_desc l_desc;
    tree_desc d_desc;
    tree_desc bl_desc;
    uch inbuf[INBUFSIZ + INBUF_EXTRA];     /* input buffer */
    ush d_buf[DIST_BUFSIZE];     /* buffer for distances, see trees.c */
    uch depth[2*L_CODES+1];
    
    // util.c
    unsigned outcnt;
    unsigned insize;
    unsigned inptr;
    long bytes_in;
    long bytes_out;
    
    // bits.c
    unsigned short bi_buf;
    int bi_valid;
    unsigned long bits_sent;
    unsigned char outbuf[OUTBUFSIZ+OUTBUF_EXTRA];
        
} thread_context;












// External Functions and Vars


// gzip.c
extern int create_outfile(global_context* gc);
extern void treatfile(global_context* gc);
extern int abort_gzip();
extern int main(int argc, char **argv);

// zip.c
extern int zip(global_context* gc);
extern unsigned int thread_read_buf(char *buf, unsigned int size, thread_context* tc); 
extern unsigned int file_read(char *buf, unsigned int size, global_context* gc);

// deflate.c
extern void lm_init (int pack_level, ush* flags, thread_context* tc);
extern void fill_window(thread_context* tc);
extern int longest_match(IPos cur_match, thread_context* tc);
extern void* deflate_work(void* arg);
extern void thread_context_init(thread_context* tc);
extern thread_context* grab_another_block(global_context* gc, thread_context* tc);
extern void* io_out_function(void* arg);
extern ulg deflate(global_context* gc);
extern config configuration_table[10];
thread_context* new_thread_context(global_context* gc);
thread_context* clean_old_thread_context(global_context* gc, thread_context* tc);

// trees.c
extern void ct_init(ush* attr, int* methodp, thread_context* tc);
extern void init_block     (thread_context* tc);
extern void pqdownheap     (ct_data *tree, int k, thread_context* tc);
extern void gen_bitlen     (tree_desc *desc, thread_context* tc);
extern void gen_codes      (ct_data *tree, int max_code, thread_context* tc);
extern void build_tree     (tree_desc *desc, thread_context* tc);
extern void scan_tree      (ct_data *tree, int max_code, thread_context* tc);
extern void send_tree      (ct_data *tree, int max_code, thread_context* tc);
extern int  build_bl_tree  (thread_context* tc);
extern void send_all_trees (int lcodes, int dcodes, int blcodes, thread_context* tc);
extern void compress_block (ct_data *ltree, ct_data *dtree, thread_context* tc);
extern void set_file_type  (thread_context* tc);
extern ulg flush_block(char* buf, ulg stored_len, int eof, thread_context* tc);
extern int ct_tally (int dist, int lc, thread_context* tc);

extern int extra_lbits[LENGTH_CODES];
extern int extra_dbits[D_CODES];
extern int extra_blbits[BL_CODES];
extern uch bl_order[BL_CODES];

// util.c
extern void error(char *m);
extern void warn(char* a, char *b);
extern void read_error();
extern void write_error();
extern void write_buf(void* buf, unsigned cnt, thread_context* tc);
extern ulg updcrc(uch *s, unsigned int n);
extern void clear_bufs(thread_context* tc);
extern int fill_inbuf(int eof_ok, thread_context* tc);
extern void flush_outbuf();
extern char *strlwr(char* s);
extern void* xmalloc(unsigned size);
extern ulg crc_32_tab[];

// bits.c
extern void bi_init(thread_context* tc);
extern void send_bits(int value, int length, thread_context* tc);
extern unsigned bi_reverse(unsigned code, int len);
extern void bi_windup(thread_context* tc);
extern void copy_block(char* buf, unsigned len, int header, thread_context* tc);


#endif
