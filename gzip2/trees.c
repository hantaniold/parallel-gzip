#include <ctype.h>
#include <stdio.h>
#include "contexts.h"

// Thread Context Variables Directly Used / Managed
//      int* file_type
//      int* file_method
//      unsigned int compressed_len
//      unsigned int input_len
//      ct_data static_ltree
//      ct_data static_dtree
//      int base_length[LENGTH_CODES];
//      uch length_code[MAX_MATCH-MIN_MATCH+1];
//      int base_dist[D_CODES];
//      uch dist_code[512];
//      ush bl_count[MAX_BITS+1];
//      ct_data dyn_ltree[HEAP_SIZE];   /* literal and length tree */
//      ct_data dyn_dtree[2*D_CODES+1]; /* distance tree */
//      ct_data bl_tree[2*BL_CODES+1];
//      ulg opt_len;        /* bit length of current block with optimal trees */
//      ulg static_len;     /* bit length of current block with static trees */
//      unsigned last_lit;    /* running index in l_buf */
//      unsigned last_dist;   /* running index in d_buf */
//      unsigned last_flags;  /* running index in flag_buf */
//      uch flags;            /* current flags not yet saved in flag_buf */
//      uch flag_bit;         /* current bit used in flags */
//      uch flag_buf[(LIT_BUFSIZE/8)];
//      int heap_len;               /* number of elements in the heap */
//      int heap[2*L_CODES+1]; /* heap used to build the Huffman trees */
//      int heap_max;               /* element of largest frequency */
//      tree_desc l_desc;// = {dyn_ltree, static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS, 0};
//      tree_desc d_desc;
//      uch* inbuf;     /* input buffer */
//      ush* d_buf;     /* buffer for distances, see trees.c */
//      uch depth[2*L_CODES+1];


// Functions Defined
//      void ct_init(int* attr, int* methodp, thread_context* tc) - Allocate the match buffer, initialize the 
//                                      various tables and save the location of the internal file attribute
//                                      (ascii/binary) and method (DEFLATE/STORE).
//      void init_block     (thread_context* tc) - Initialize a new block.
//      void pqdownheap     (ct_data *tree, int k, thread_context* tc) - Restore the heap property by moving down 
//                                          the tree starting at node k, exchanging a node with the smallest of its
//                                          two sons if necessary, stopping when the heap property is re-established
//                                          (each father smaller than its two sons).
//      void gen_bitlen     (tree_desc *desc, thread_context* tc) - Compute the optimal bit lengths for a tree and
//                                                               update the total bit length for the current block.
//                                                IN assertion: the fields freq and dad are set, heap[heap_max] and
//                                                above are the tree nodes sorted by increasing frequency.
//                                                OUT assertions: the field len is set to the optimal bit length, the
//                                                array bl_count contains the frequencies for each bit length.
//                                                The length opt_len is updated; static_len is also updated if stree is not null.
//      void gen_codes      (ct_data *tree, int max_code, thread_context* tc) - Generate the codes for a given tree
//                                               and bit counts (which need not be optimal).
//                                               IN assertion: the array bl_count contains the bit length statistics for
//                                                             the given tree and the field len is set for all tree elements.
//                                               OUT assertion: the field code is set for all tree elements of non zero code length.
//      void build_tree     (tree_desc *desc, thread_context* tc) - Construct one Huffman tree and assigns the code bit
//                                               strings and lengths. Update the total bit length for the current block.
//                                               IN assertion: the field freq is set for all tree elements.
//                                               OUT assertions: the fields len and code are set to the optimal bit length
//                                               and corresponding code. The length opt_len is updated; static_len is
//                                               also updated if stree is not null. The field max_code is set.
//      void scan_tree      (ct_data *tree, int max_code, thread_context* tc) - Scan a literal or distance tree to determine the
//                                               frequencies of the codes in the bit length tree. Updates opt_len to take into
//                                               account the repeat counts. (The contribution of the bit length codes will be 
//                                               added later during the construction of bl_tree.)
//      void send_tree      (ct_data *tree, int max_code, thread_context* tc) - Send a literal or distance tree in compressed
//                                               form, using the codes in bl_tree.
//      int  build_bl_tree  (thread_context* tc) - Construct the Huffman tree for the bit lengths and return the index in
//                                                 bl_order of the last bit length code to send.
//      void send_all_trees (int lcodes, int dcodes, int blcodes, thread_context* tc) - Send the header for a block using 
//                                               dynamic Huffman trees: the counts, the lengths of the bit length codes,
//                                               the literal tree and the distance tree.
//                                               IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
//      void compress_block (ct_data *ltree, ct_data *dtree, thread_context* tc) - Send the block data compressed using the given Huffman trees
//      void set_file_type  (thread_context* tc) - Set the file type to ASCII or BINARY, using a crude approximation:
//                                                 binary if more than 20% of the bytes are <= 6 or >= 128, ascii otherwise.
//                                                 IN assertion: the fields freq of dyn_ltree are set and the total of all
//                                                 frequencies does not exceed 64K (to fit in an int on 16 bit machines).
//      ulg flush_block(char* buf, ulg stored_len, int eof, thread_context* tc) - Determine the best encoding for the current 
//                                              block: dynamic trees, static trees or store, and output the encoded block to
//                                              the zip file. This function returns the total compressed length for the file so far.
//      int ct_tally (int dist, int lc, thread_context* tc) - Save the match info and tally the frequency counts. Return true if
//                                                            the current block must be flushed.
//

// Defines Used
//      MAX_BITS - #define MAX_BITS 15
//      MAX_BL_BITS - #define MAX_BL_BITS 7
//      LENGTH_CODES - #define LENGTH_CODES 29
//      LITERALS - #define LITERALS  256
//      END_BLOCK - #define END_BLOCK 256
//      L_CODES - #define L_CODES (LITERALS+1+LENGTH_CODES)
//      D_CODES - #define D_CODES   30
//      BL_CODES - #define BL_CODES  19
//      STORED_BLOCK - #define STORED_BLOCK 0
//      STATIC_TREES - #define STATIC_TREES 1
//      DYN_TREES - #define DYN_TREES    2
//      REP_3_6 - #define REP_3_6      16
//      REPZ_3_10 - #define REPZ_3_10    17
//      REPZ_11_138 - #define REPZ_11_138  18
//      SMALLEST - #define SMALLEST 1
//      l_buf - #define l_buf inbuf
//      Freq - #define Freq fc.freq
//      Code - #define Code fc.code
//      Dad - #define Dad  dl.dad
//      Len - #define Len  dl.len
//      HEAP_SIZE - #define HEAP_SIZE (2*L_CODES+1)
//      send_code - #define send_code(c, tree) send_bits(tree[c].Code, tree[c].Len, tc)
//      d_code - #define d_code(dist) ((dist) < 256 ? tc->dist_code[dist] : tc->dist_code[256+((dist)>>7)])
//      MAX - #define MAX(a,b) (a >= b ? a : b)
//      pqremove - #define pqremove(tree, top) \
//                 {\ top = tc->heap[SMALLEST]; \
//                    tc->heap[SMALLEST] = tc->heap[tc->heap_len--]; \
//                    pqdownheap(tree, SMALLEST, tc); }
//      smaller - #define smaller(tree, n, m) \
//                (tree[n].Freq < tree[m].Freq || \
//                (tree[n].Freq == tree[m].Freq && tc->depth[n] <= tc->depth[m]))



void ct_init(int* attr, int* methodp, thread_context* tc)
{
    int n;        /* iterates over tree elements */
    int bits;     /* bit counter */
    int length;   /* length value */
    int code;     /* code value */
    int dist;     /* distance index */

    tc->file_type = attr;
    tc->file_method = methodp;
    tc->compressed_len = tc->input_len = 0L;
        
    //if (tc->static_dtree[0].Len != 0) return; /* ct_init already called */

    /* Initialize the mapping length (0..255) -> length code (0..28) */
    length = 0;
    for (code = 0; code < LENGTH_CODES-1; code++) {
        tc->base_length[code] = length;
        for (n = 0; n < (1<<extra_lbits[code]); n++) {
            tc->length_code[length++] = (uch)code;
        }
    }
    Assert (length == 256, "ct_init: length != 256");
    /* Note that the length 255 (match length 258) can be represented
     * in two different ways: code 284 + 5 bits or code 285, so we
     * overwrite length_code[255] to use the best encoding:
     */

    tc->length_code[length-1] = (uch)code;
    /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
    dist = 0;
    for (code = 0 ; code < 16; code++) {
        tc->base_dist[code] = dist;
        for (n = 0; n < (1<<extra_dbits[code]); n++) {
            tc->dist_code[dist++] = (uch)code;
        }
    }

    Assert (dist == 256, "ct_init: dist != 256");
    dist >>= 7; /* from now on, all distances are divided by 128 */
    for ( ; code < D_CODES; code++) {
        tc->base_dist[code] = dist << 7;
        for (n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
            tc->dist_code[256 + dist++] = (uch)code;
        }
    }

    Assert (dist == 256, "ct_init: 256+dist != 512");

    /* Construct the codes of the static literal tree */
    for (bits = 0; bits <= MAX_BITS; bits++) tc->bl_count[bits] = 0;
    n = 0;
    while (n <= 143) tc->static_ltree[n++].Len = 8, tc->bl_count[8]++;
    while (n <= 255) tc->static_ltree[n++].Len = 9, tc->bl_count[9]++;
    while (n <= 279) tc->static_ltree[n++].Len = 7, tc->bl_count[7]++;
    while (n <= 287) tc->static_ltree[n++].Len = 8, tc->bl_count[8]++;
    /* Codes 286 and 287 do not exist, but we must include them in the
     * tree construction to get a canonical Huffman tree (longest code
     * all ones)
     */

    gen_codes((ct_data *)(tc->static_ltree), L_CODES+1, tc);

    /* The static distance tree is trivial: */
    for (n = 0; n < D_CODES; n++) {
        tc->static_dtree[n].Len = 5;
        tc->static_dtree[n].Code = bi_reverse(n, 5);
    }

    printf("About to Init Block!\n");
    /* Initialize the first block of the first file: */
    init_block(tc);
}

void init_block(thread_context* tc)
{
    int n; /* iterates over tree elements */

    /* Initialize the trees. */
    for (n = 0; n < L_CODES;  n++) tc->dyn_ltree[n].Freq = 0;
    printf("\tFinished init Block\n");
    for (n = 0; n < D_CODES;  n++) tc->dyn_dtree[n].Freq = 0;
    printf("\tFinished init Block\n");
    for (n = 0; n < BL_CODES; n++) tc->bl_tree[n].Freq = 0;
    printf("\tFinished init Block\n");

    tc->dyn_ltree[END_BLOCK].Freq = 1;
    tc->opt_len = tc->static_len = 0L;
    tc->last_lit = tc->last_dist = tc->last_flags = 0;
    tc->flags = 0; tc->flag_bit = 1;
    printf("\tFinished init Block\n");
}

void pqdownheap(ct_data* tree, int k, thread_context* tc)
{
    int v = tc->heap[k];
    int j = k << 1;  /* left son of k */
    while (j <= tc->heap_len) {
        /* Set j to the smallest of the two sons: */
        if (j < tc->heap_len && smaller(tree, tc->heap[j+1], tc->heap[j])) j++;

        /* Exit if v is smaller than both sons */
        if (smaller(tree, v, tc->heap[j])) break;

        /* Exchange v with the smallest son */
        tc->heap[k] = tc->heap[j];  k = j;

        /* And continue down the tree, setting j to the left son of k */
        j <<= 1;
    }
    tc->heap[k] = v;
}

void gen_bitlen(tree_desc* desc, thread_context* tc)
{
    ct_data *tree  = desc->dyn_tree;
    int *extra     = desc->extra_bits;
    int base            = desc->extra_base;
    int max_code        = desc->max_code;
    int max_length      = desc->max_length;
    ct_data *stree = desc->static_tree;
    int h;              /* heap index */
    int n, m;           /* iterate over the tree elements */
    int bits;           /* bit length */
    int xbits;          /* extra bits */
    ush f;              /* frequency */
    int overflow = 0;   /* number of elements with bit length too large */

    for (bits = 0; bits <= MAX_BITS; bits++) tc->bl_count[bits] = 0;

    /* In a first pass, compute the optimal bit lengths (which may
     * overflow in the case of the bit length tree).
     */
    tree[tc->heap[tc->heap_max]].Len = 0; /* root of the heap */

    for (h = tc->heap_max+1; h < HEAP_SIZE; h++) {
        n = tc->heap[h];
        bits = tree[tree[n].Dad].Len + 1;
        if (bits > max_length) bits = max_length, overflow++;
        tree[n].Len = (ush)bits;
        /* We overwrite tree[n].Dad which is no longer needed */

        if (n > max_code) continue; /* not a leaf node */

        tc->bl_count[bits]++;
        xbits = 0;
        if (n >= base) xbits = extra[n-base];
        f = tree[n].Freq;
        tc->opt_len += (ulg)f * (bits + xbits);
        if (stree) tc->static_len += (ulg)f * (stree[n].Len + xbits);
    }
    if (overflow == 0) return;

    Trace((stderr,"\nbit length overflow\n"));
    /* This happens for example on obj2 and pic of the Calgary corpus */

    /* Find the first bit length which could increase: */
    do {
        bits = max_length-1;
        while (tc->bl_count[bits] == 0) bits--;
        tc->bl_count[bits]--;      /* move one leaf down the tree */
        tc->bl_count[bits+1] += 2; /* move one overflow item as its brother */
        tc->bl_count[max_length]--;
        /* The brother of the overflow item also moves one step up,
         * but this does not affect bl_count[max_length]
         */
        overflow -= 2;
    } while (overflow > 0);

    /* Now recompute all bit lengths, scanning in increasing frequency.
     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
     * lengths instead of fixing only the wrong ones. This idea is taken
     * from 'ar' written by Haruhiko Okumura.)
     */
    for (bits = max_length; bits != 0; bits--) {
        n = tc->bl_count[bits];
        while (n != 0) {
            m = tc->heap[--h];
            if (m > max_code) continue;
            if (tree[m].Len != (unsigned) bits) {
                Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
                tc->opt_len += ((long)bits-(long)tree[m].Len)*(long)tree[m].Freq;
                tree[m].Len = (ush)bits;
            }
            n--;
        }
    }
}

void gen_codes(ct_data* tree, int max_code, thread_context* tc)
{
    ush next_code[MAX_BITS+1]; /* next code value for each bit length */
    ush code = 0;              /* running code value */
    int bits;                  /* bit index */
    int n;                     /* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    for (bits = 1; bits <= MAX_BITS; bits++) {
        next_code[bits] = code = (code + tc->bl_count[bits-1]) << 1;
    }
    /* Check that the bit counts in bl_count are consistent. The last code
     * must be all ones.
     */
    Assert (code + tc->bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
            "inconsistent bit counts");
    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

    for (n = 0;  n <= max_code; n++) {
        int len = tree[n].Len;
        if (len == 0) continue;
        /* Now reverse the bits */
        tree[n].Code = bi_reverse(next_code[len]++, len);

        //Tracec(tree != tc->static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
        //     n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
    }
}

void build_tree(tree_desc* desc, thread_context* tc)
{
    ct_data *tree   = desc->dyn_tree;
    ct_data *stree  = desc->static_tree;
    int elems            = desc->elems;
    int n, m;          /* iterate over heap elements */
    int max_code = -1; /* largest code with non zero frequency */
    int node = elems;  /* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
     * heap[0] is not used.
     */
    tc->heap_len = 0, tc->heap_max = HEAP_SIZE;

    for (n = 0; n < elems; n++) {
        if (tree[n].Freq != 0) {
            tc->heap[++tc->heap_len] = max_code = n;
            tc->depth[n] = 0;
        } else {
            tree[n].Len = 0;
        }
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while (tc->heap_len < 2) {
        int new = tc->heap[++(tc->heap_len)] = (max_code < 2 ? ++max_code : 0);
        tree[new].Freq = 1;
        tc->depth[new] = 0;
        tc->opt_len--; if (stree) tc->static_len -= stree[new].Len;
        /* new is 0 or 1 so it does not have extra bits */
    }
    desc->max_code = max_code;

    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for (n = tc->heap_len/2; n >= 1; n--) pqdownheap(tree, n, tc);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do {
        pqremove(tree, n);   /* n = node of least frequency */
        m = tc->heap[SMALLEST];  /* m = node of next least frequency */

        tc->heap[--tc->heap_max] = n; /* keep the nodes sorted by frequency */
        tc->heap[--tc->heap_max] = m;

        /* Create a new node father of n and m */
        tree[node].Freq = tree[n].Freq + tree[m].Freq;
        tc->depth[node] = (uch) (MAX(tc->depth[n], tc->depth[m]) + 1);
        tree[n].Dad = tree[m].Dad = (ush)node;
        /* and insert the new node in the heap */
        tc->heap[SMALLEST] = node++;
        pqdownheap(tree, SMALLEST, tc);

    } while (tc->heap_len >= 2);

    tc->heap[--(tc->heap_max)] = tc->heap[SMALLEST];

    /* At this point, the fields freq and dad are set. We can now
     * generate the bit lengths.
     */
    gen_bitlen((tree_desc *)desc, tc);

    /* The field len is now set, we can generate the bit codes */
    gen_codes ((ct_data *)tree, max_code, tc);
}

void scan_tree (ct_data* tree, int max_code, thread_context* tc)
{
    int n;                     /* iterates over all tree elements */
    int prevlen = -1;          /* last emitted length */
    int curlen;                /* length of current code */
    int nextlen = tree[0].Len; /* length of next code */
    int count = 0;             /* repeat count of the current code */
    int max_count = 7;         /* max repeat count */
    int min_count = 4;         /* min repeat count */

    if (nextlen == 0) max_count = 138, min_count = 3;
    tree[max_code+1].Len = (ush)0xffff; /* guard */

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            tc->bl_tree[curlen].Freq += count;
        } else if (curlen != 0) {
            if (curlen != prevlen) tc->bl_tree[curlen].Freq++;
            tc->bl_tree[REP_3_6].Freq++;
        } else if (count <= 10) {
            tc->bl_tree[REPZ_3_10].Freq++;
        } else {
            tc->bl_tree[REPZ_11_138].Freq++;
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}

void send_tree (ct_data* tree, int max_code, thread_context* tc)
{
    int n;                     /* iterates over all tree elements */
    int prevlen = -1;          /* last emitted length */
    int curlen;                /* length of current code */
    int nextlen = tree[0].Len; /* length of next code */
    int count = 0;             /* repeat count of the current code */
    int max_count = 7;         /* max repeat count */
    int min_count = 4;         /* min repeat count */

    /* tree[max_code+1].Len = -1; */  /* guard already set */
    if (nextlen == 0) max_count = 138, min_count = 3;

    for (n = 0; n <= max_code; n++) {
        curlen = nextlen; nextlen = tree[n+1].Len;
        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
            do { send_code(curlen, tc->bl_tree); } while (--count != 0);

        } else if (curlen != 0) {
            if (curlen != prevlen) {
                send_code(curlen, tc->bl_tree); count--;
            }
            Assert(count >= 3 && count <= 6, " 3_6?");
            send_code(REP_3_6, tc->bl_tree); send_bits(count-3, 2, tc);

        } else if (count <= 10) {
            send_code(REPZ_3_10, tc->bl_tree); send_bits(count-3, 3, tc);

        } else {
            send_code(REPZ_11_138, tc->bl_tree); send_bits(count-11, 7, tc);
        }
        count = 0; prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    }
}

int build_bl_tree(thread_context* tc)
{
    int max_blindex;  /* index of last bit length code of non zero freq */

    /* Determine the bit length frequencies for literal and distance trees */
    scan_tree((ct_data *)(tc->dyn_ltree), tc->l_desc.max_code, tc);
    scan_tree((ct_data *)(tc->dyn_dtree), tc->d_desc.max_code, tc);

    /* Build the bit length tree: */
    build_tree((tree_desc *)&(tc->bl_desc), tc);
    /* opt_len now includes the length of the tree representations, except
     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
     */

    /* Determine the number of bit length codes to send. The pkzip format
     * requires that at least 4 bit length codes be sent. (appnote.txt says
     * 3 but the actual value used is 4.)
     */
    for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
        if (tc->bl_tree[bl_order[max_blindex]].Len != 0) break;
    }
    /* Update opt_len to include the bit length tree and counts */
    tc->opt_len += 3*(max_blindex+1) + 5+5+4;
    Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld", tc->opt_len, tc->static_len));

    return max_blindex;
}

void send_all_trees(int lcodes, int dcodes, int blcodes, thread_context* tc)
{
    int rank;                    /* index in bl_order */

    Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
    Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
            "too many codes");
    Tracev((stderr, "\nbl counts: "));
    send_bits(lcodes-257, 5, tc); /* not +255 as stated in appnote.txt */
    send_bits(dcodes-1,   5, tc);
    send_bits(blcodes-4,  4, tc); /* not -3 as stated in appnote.txt */
    for (rank = 0; rank < blcodes; rank++) {
        Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
        send_bits(tc->bl_tree[bl_order[rank]].Len, 3, tc);
    }
    //Tracev((stderr, "\nbl tree: sent %ld", bits_sent));

    send_tree((ct_data *)(tc->dyn_ltree), lcodes-1, tc); /* send the literal tree */
    //Tracev((stderr, "\nlit tree: sent %ld", bits_sent));

    send_tree((ct_data *)(tc->dyn_dtree), dcodes-1, tc); /* send the distance tree */
    //Tracev((stderr, "\ndist tree: sent %ld", bits_sent));
}

ulg flush_block(char* buf, ulg stored_len, int eof, thread_context* tc)
{
    ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
    int max_blindex;  /* index of last bit length code of non zero freq */

    tc->flag_buf[tc->last_flags] = tc->flags; /* Save the flags for the last 8 items */

     /* Check if the file is ascii or binary */
    if (*(tc->file_type) == (ush)UNKNOWN) set_file_type(tc);

    /* Construct the literal and distance trees */
    build_tree((tree_desc *)(&(tc->l_desc)), tc);
    //Tracev((stderr, "\nlit data: dyn %ld, stat %ld", opt_len, static_len));

    build_tree((tree_desc *)(&(tc->d_desc)), tc);
    //Tracev((stderr, "\ndist data: dyn %ld, stat %ld", opt_len, static_len));
    /* At this point, opt_len and static_len are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in bl_order of the last bit length code to send.
     */
    max_blindex = build_bl_tree(tc);

    /* Determine the best encoding. Compute first the block length in bytes */
    opt_lenb = (tc->opt_len+3+7)>>3;
    static_lenb = (tc->static_len+3+7)>>3;
    tc->input_len += stored_len; /* for debugging only */

    //Trace((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ",
    //        opt_lenb, opt_len, static_lenb, static_len, stored_len,
    //        last_lit, last_dist));

    if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

    /* If compression failed and this is the first and last block,
     * and if the zip file can be seeked (to rewrite the local header),
     * the whole file is transformed into a stored file:
     */
#ifdef FORCE_METHOD
    if (tc->compr_level == 1 && eof && tc->compressed_len == 0L) { /* force stored file */
#else
    if (stored_len <= opt_lenb && eof && tc->compressed_len == 0L) {
#endif
        /* Since LIT_BUFSIZE <= 2*WSIZE, the input data must be there: */
        if (buf == (char*)0) error ("block vanished");

        copy_block(buf, (unsigned)stored_len, 0, tc); /* without header */
        tc->compressed_len = stored_len << 3;
        *(tc->file_method) = STORED;

#ifdef FORCE_METHOD
    } else if (tc->compr_level == 2 && buf != (char*)0) { /* force stored block */
#else
    } else if (stored_len+4 <= opt_lenb && buf != (char*)0) {
                       /* 4: two words for the lengths */
#endif
        /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
         * Otherwise we can't have processed more than WSIZE input bytes since
         * the last block flush, because compression would have been
         * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
         * transform a block into a stored block.
         */
        send_bits((STORED_BLOCK<<1)+eof, 3, tc);  /* send block type */
        tc->compressed_len = (tc->compressed_len + 3 + 7) & ~7L;
        tc->compressed_len += (stored_len + 4) << 3;

        copy_block(buf, (unsigned)stored_len, 1, tc); /* with header */

#ifdef FORCE_METHOD
    } else if (level == 3) { /* force static trees */
#else
    } else if (static_lenb == opt_lenb) {
#endif
        send_bits((STATIC_TREES<<1)+eof, 3, tc);
        compress_block((ct_data *)(tc->static_ltree), (ct_data *)(tc->static_dtree), tc);
        tc->compressed_len += 3 + tc->static_len;
    } else {
        send_bits((DYN_TREES<<1)+eof, 3, tc);
        send_all_trees(tc->l_desc.max_code+1, tc->d_desc.max_code+1, max_blindex+1, tc);
        compress_block((ct_data *)(tc->dyn_ltree), (ct_data *)(tc->dyn_dtree), tc);
        tc->compressed_len += 3 + tc->opt_len;
    }
    //Assert (tc->compressed_len == bits_sent, "bad compressed size");
    init_block(tc);

    if (eof) {
        //Assert (tc->input_len == isize, "bad input size");
        bi_windup(tc);
        tc->compressed_len += 7;  /* align on byte boundary */
    }
    //Tracev((stderr,"\ncomprlen %lu(%lu) ", compressed_len>>3,
    //       compressed_len-7*eof));

    return (tc->compressed_len) >> 3;
}

int ct_tally (int dist, int lc, thread_context* tc)
{
    tc->l_buf[tc->last_lit++] = (uch)lc;
    if (dist == 0) {
        /* lc is the unmatched char */
        tc->dyn_ltree[lc].Freq++;
    } else {
        /* Here, lc is the match length - MIN_MATCH */
        dist--;             /* dist = match distance - 1 */
        Assert((ush)dist < (ush)MAX_DIST &&
               (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
               (ush)d_code(dist) < (ush)D_CODES,  "ct_tally: bad match");

        tc->dyn_ltree[tc->length_code[lc]+LITERALS+1].Freq++;
        tc->dyn_dtree[d_code(dist)].Freq++;

        tc->d_buf[tc->last_dist++] = (ush)dist;
        tc->flags |= tc->flag_bit;
    }
    tc->flag_bit <<= 1;

    /* Output the flags if they fill a byte: */
    if ((tc->last_lit & 7) == 0) {
        tc->flag_buf[tc->last_flags++] = tc->flags;
        tc->flags = 0, tc->flag_bit = 1;
    }
    /* Try to guess if it is profitable to stop the current block here */
    if (tc->compr_level > 2 && (tc->last_lit & 0xfff) == 0) {
        /* Compute an upper bound for the compressed length */
        ulg out_length = (ulg)(tc->last_lit*8L);
        ulg in_length = (ulg)(tc->strstart-tc->block_start);
        int dcode;
        for (dcode = 0; dcode < D_CODES; dcode++) {
            out_length += (ulg)(tc->dyn_dtree[dcode].Freq*(5L+extra_dbits[dcode]));
        }
        out_length >>= 3;
        //Trace((stderr,"\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ",
        //       last_lit, last_dist, in_length, out_length,
        //       100L - out_length*100L/in_length));
        if(tc->last_dist < tc->last_lit/2 && out_length < in_length/2) return 1;
    }
    return (tc->last_lit == LIT_BUFSIZE-1 || tc->last_dist == DIST_BUFSIZE);
    /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
     * on 16 bit machines and because stored blocks are restricted to
     * 64K-1 bytes.
     */
}

void compress_block(ct_data* ltree, ct_data* dtree, thread_context* tc)
{
    unsigned dist;      /* distance of matched string */
    int lc;             /* match length or unmatched char (if dist == 0) */
    unsigned lx = 0;    /* running index in l_buf */
    unsigned dx = 0;    /* running index in d_buf */
    unsigned fx = 0;    /* running index in flag_buf */
    uch flag = 0;       /* current flags */
    unsigned code;      /* the code to send */
    int extra;          /* number of extra bits to send */

    if (tc->last_lit != 0) do {
        if ((lx & 7) == 0) flag = tc->flag_buf[fx++];
        lc = tc->l_buf[lx++];
        if ((flag & 1) == 0) {
            send_code(lc, ltree); /* send a literal byte */
            //Tracecv(isgraph(lc), (stderr," '%c' ", lc));
        } else {
            /* Here, lc is the match length - MIN_MATCH */
            code = tc->length_code[lc];
            send_code(code+LITERALS+1, ltree); /* send the length code */
            extra = extra_lbits[code];
            if (extra != 0) {
                lc -= tc->base_length[code];
                send_bits(lc, extra, tc);        /* send the extra length bits */
            }
            dist = tc->d_buf[dx++];
            /* Here, dist is the match distance - 1 */
            code = d_code(dist);
            Assert (code < D_CODES, "bad d_code");

            send_code(code, dtree);       /* send the distance code */
            extra = extra_dbits[code];
            if (extra != 0) {
                dist -= tc->base_dist[code];
                send_bits(dist, extra, tc);   /* send the extra distance bits */
            }
        } /* literal or match pair ? */
        flag >>= 1;
    } while (lx < tc->last_lit);

    send_code(END_BLOCK, ltree);
}

void set_file_type(thread_context* tc)
{
    int n = 0;
    unsigned ascii_freq = 0;
    unsigned bin_freq = 0;
    while (n < 7)        bin_freq += tc->dyn_ltree[n++].Freq;
    while (n < 128)      ascii_freq += tc->dyn_ltree[n++].Freq;
    while (n < LITERALS) bin_freq += tc->dyn_ltree[n++].Freq;
    *(tc->file_type) = (bin_freq > ascii_freq >> 2) ? BINARY : ASCII;
}



int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

int extra_dbits[D_CODES] /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

int extra_blbits[BL_CODES]/* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

uch bl_order[BL_CODES]
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */