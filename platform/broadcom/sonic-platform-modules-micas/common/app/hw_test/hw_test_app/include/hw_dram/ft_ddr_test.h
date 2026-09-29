#ifndef __FT_DDR_TEST_H__
#define __FT_DDR_TEST_H__

#include <sys/types.h>

#define FT_DDR_SUCCESS      0
#define FT_DDR_ERR          1
#define SPINSZ              0x800000
#define MOD_SZ              20
#define MAX_MEM_SEGMENTS    32
#define BLK_SIZE_NUM_MAX    9
#define BADRAM_MAXPATNS     10
#define LOW_TEST_ADR        0x00002000        /* Final adrs for test code */
#define HIGH_TEST_ADR       0x00180000        /* Relocation base address */
#define USE_DMA
#define TETS_TOTAL_NUMBER   14

#define FT_SHOW_PROCESS
#ifdef FT_SHOW_PROCESS
#define FT_DDR_SHOW()           do {printf("."); fflush(stdout);}while (0)
#define FT_DDR_SHOW_END()       do {printf("\n"); fflush(stdout);}while (0)
#else
#define FT_DDR_SHOW()           do {}while (0)
#define FT_DDR_SHOW_END()       do {}while (0)
#endif

typedef struct data_seg_s {
    int     seq;
    ulong   start_addr;
    ulong   end_addr;
    ulong   size;
    int     use_dma;
} data_seg_t;

struct pair {
       ulong adr;
       ulong mask;
};

struct mmap {
    ulong pbase_addr;
    unsigned int *start;
    unsigned int *end;
};

struct pmap {
    ulong start;
    ulong end;
};

/* Define common variables accross relocations of memtest86 */
struct vars {
    int test;
    int pass;
    unsigned long *eadr;
    unsigned long exor;
    int msg_line;
    int ecount;
    int ecc_ecount;
    int msegs;           /* The number of memory areas to test */
    int testsel;
    int scroll_start;
    int rdtsc;
    int pae;
    int pass_ticks;
    int total_ticks;
    int pptr;
    int tptr;
    struct pmap pmap[MAX_MEM_SEGMENTS];
    struct mmap map[MAX_MEM_SEGMENTS];
    ulong plim_lower;
    ulong plim_upper;
    ulong clks_msec;
    ulong starth;
    ulong startl;
    ulong snaph;
    ulong snapl;
    int printmode;
    int numpatn;
    struct pair patn [BADRAM_MAXPATNS];
    ulong test_pages;
    ulong selected_pages;
    ulong reserved_pages;
};

struct tseq {
    short cache;
    short pat;
    short iter;
    short ticks;
    short errors;
    char *msg;
};

typedef struct ft_ddr_test_interface_s {
    int always;                             /* Copy machine test 0: No test 1: test */
    int (*ddr_test)(char *desc);
    char name[64];
} ft_ddr_test_interface_t;

extern void ft_ddr_test_init(void *start, size_t size);
extern ft_ddr_test_interface_t ft_ddr_test_fun[TETS_TOTAL_NUMBER];

#endif /* __FT_DDR_TEST_H__ */
