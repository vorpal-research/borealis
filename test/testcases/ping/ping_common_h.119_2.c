#include <bits/wordsize.h>
#include <linux/types.h>

#define	MAX_DUP_CHK	0x10000

#if defined(__WORDSIZE) && __WORDSIZE == 64
# define USE_BITMAP64
#endif

#ifdef USE_BITMAP64
typedef __u64	bitmap_t;
# define BITMAP_SHIFT	6
#else
typedef __u32	bitmap_t;
# define BITMAP_SHIFT	5
#endif

#if ((MAX_DUP_CHK >> (BITMAP_SHIFT + 3)) << (BITMAP_SHIFT + 3)) != MAX_DUP_CHK
# error Please MAX_DUP_CHK and/or BITMAP_SHIFT
#endif

struct rcvd_table {
    bitmap_t bitmap[MAX_DUP_CHK / (sizeof(bitmap_t) * 8)];
} rcvd_tbl;

#define	A(bit)	(rcvd_tbl.bitmap[(bit) >> BITMAP_SHIFT])	           /* identify word in array */
#define	B(bit)	(((bitmap_t)1) << ((bit) & ((1 << BITMAP_SHIFT) - 1))) /* identify bit in word */

static inline bitmap_t rcvd_test(__u16 seq) {
    unsigned bit = seq % MAX_DUP_CHK;
    return A(bit) & B(bit);
}

int main(int argc, char** argv) {
    __u16 seq = (__u16) argc;
    rcvd_test(seq);
    return 0;
}
