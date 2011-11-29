#include <stdio.h>
#include "contexts.h"

// Thread Context Variables Directly Used / Managed
//      unsigned short bi_buf - Output buffer. bits are inserted starting at the bottom (least significant bits).
//      int bi_valid - Number of valid bits in bi_buf.  All bits above the last valid bit are always zero.
//      unsigned long bits_sent - bit length of the compressed data
//      unsigned char[] outbuf - Buffer for flushing out all of our data
//
//      unsigned outcnt        - See deflate.c

// Functions Defined
//      void bi_init(thread_context* tc) - Initialize the BitString Routines
//      void send_bits(int value, int length, thread_context* tc) - Send a value on a given number of bits.
//                                                                  IN assertion: length <= 16 and value fits in length bits.
//                                                                  value - value to send, length - number of bits
//      unsigned bi_reverse(unsigned code, int len) - Reverse the first len bits of a code, using straightforward code 
//                                                    (a faster method would use a table)
//                                                    IN assertion: 1 <= len <= 15
//                                                    code - the value to invert, len - its bit length
//      void bi_windup(thread_context* tc) - Write out any remaining bits in an incomplete byte.
//      void copy_block(char* buf, unsigned len, int header, thread_context* tc) -  Copy a stored block to the zip file, 
//                                                                                  storing first the length and its one's
//                                                                                  complement if requested.

// Defines Used
//      put_short -  #define put_short(w) \
//                   { if (tc->outcnt < OUTBUFSIZ-2) { \
//                     tc->outbuf[tc->outcnt++] = (uch) ((w) & 0xff); \
//                     tc->outbuf[tc->outcnt++] = (uch) ((ush)(w) >> 8); \
//                   } else { \
//                   put_byte((uch)((w) & 0xff)); \
//                   put_byte((uch)((ush)(w) >> 8)); \
//                   } }
//      put_byte - #define put_byte(c) {tc->outbuf[tc->outcnt++]=(uch)(c); if (tc->outcnt==OUTBUFSIZ) flush_outbuf(tc);}
//      Buf_size - #define Buf_size (8 * 2*sizeof(char))
//      OUTBUFSIZ - #define OUTBUFSIZ  16384

// Functions Used
//      flush_outbuf - See util.c


void bi_init(thread_context* tc)
{
    tc->bi_buf = 0;
    tc->bi_valid = 0;
    tc->bits_sent = 0L;
}

void send_bits(int value, int length, thread_context* tc)
{
    tc->bits_sent += (unsigned long)length;
    
    /* If not enough room in bi_buf, use (valid) bits from bi_buf and
     * (16 - bi_valid) bits from value, leaving (width - (16-bi_valid))
     * unused bits in value.
     */
    if (tc->bi_valid > (int)Buf_size - length) {
        tc->bi_buf |= (value << tc->bi_valid);
        put_short(tc->bi_buf);
        tc->bi_buf = (ush)value >> (Buf_size - tc->bi_valid);
        tc->bi_valid += length - Buf_size;
    } else {
        tc->bi_buf |= value << tc->bi_valid;
        tc->bi_valid += length;
    }
}

unsigned bi_reverse(unsigned code, int len)
{
    register unsigned res = 0;
    do {
        res |= code & 1;
        code >>= 1, res <<= 1;
    } while (--len > 0);
    return res >> 1;
}

void bi_windup(thread_context* tc)
{
    if (tc->bi_valid > 8) { put_short(tc->bi_buf); }
    else { put_byte(tc->bi_buf); }
    tc->bi_buf = 0; tc->bi_valid = 0;
    tc->bits_sent = (tc->bits_sent + 7) & ~7;
}

void copy_block(char* buf, unsigned len, int header, thread_context* tc)
{
    bi_windup(tc);              /* align on byte boundary */

    if (header)
    {
        put_short((ush)len);  
        put_short((ush)~len);
        tc->bits_sent += 2*16;
    }
    tc->bits_sent += (unsigned long)len<<3;
    while (len--) { put_byte(*buf++); }
}
