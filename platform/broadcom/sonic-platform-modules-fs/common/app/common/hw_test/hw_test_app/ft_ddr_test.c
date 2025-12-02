#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <hw_dram/fac_common.h>
#include <hw_dram/ft_ddr_test.h>

#undef USE_DMA  /* Disable DMA */
typedef unsigned int uint32;
#define  MEMORY_SIZE_1G        (1024 * 1024 * 1024ul)

volatile uint32 *pp = 0;
uint32   p1 = 0, p2 = 0, p0 = 0;
int     fail = 0;
int     segs = 0; 
struct vars variables = { };
struct vars *v = &variables;
//dma_tx_data_t g_dma_tx_data[MAX_DMA_CHAN][NUM_LINK_DESC];
static unsigned int SEED_X = 521288629;
static unsigned int SEED_Y = 362436069;

/* Memory block size at dma transfer time */
int blk_size_array[BLK_SIZE_NUM_MAX] = { 
    0x2000000, /* 32MB */
    0x1800000, /* 24MB */
    0x1000000, /* 16MB */
    0xA00000,  /* 10MB */
    0x800000,  /* 8MB */  
    0x600000,  /* 6MB */ 
    0x500000,  /* 5MB */  
    0x300000,  /* 3MB */  
    0x200000,  /* 2MB */
};

#if 0
extern int dma_mov_addr(int engine, int from_addr, int to_addr, int size);
#endif
static int ddr_half_mov_inv_random_pat(char *desc);
static int ddr_mov_inv_random_pat(char *desc);
static int ddr_mov_inv_0_1(char *desc);
static int ddr_mov_inv_8_pat(char *desc);
static int ddr_mov_inv_32_pat(char *desc);
static int ddr_mod20_random_pat(char *desc);
static int ddr_mod20_0_1_pat(char *desc);
static int ddr_mod20_8_pat(char *desc);
static int ddr_addr_walking(char *desc);
static int ddr_march_c(char *desc);
static int ddr_march_g(char *desc);
static int ddr_galloping(char *desc);
static int ddr_column_operate(char *desc);
static int ddr_swap_blocks(char *desc);

ft_ddr_test_interface_t ft_ddr_test_fun[TETS_TOTAL_NUMBER] = {
    {0, ddr_half_mov_inv_random_pat, "Half move inversion - random number test"},     /* Half move inversion - random number test, no copying */
    {1, ddr_mov_inv_random_pat, "Move inversion - random number test"},            /* Move inversion - random number test  */
    {0, ddr_mov_inv_0_1, "Move inversion -0&1 test"},                      /* Move inversion -0&1 test, no copying */
    {1, ddr_mov_inv_8_pat, "Move inversion -8bit test"},                   /* Move inversion -8bit test */
    {0, ddr_mov_inv_32_pat, "Move inversion -32bit test"},                 /* Move inversion -32bit test, no copying */
    {0, ddr_mod20_random_pat, "Mode 20- Random number test"},                  /* Mode 20- Random number test, no copying  */
    {1, ddr_mod20_0_1_pat, "Mode 20-0&1 test"},                        /* Mode 20-0&1 test*/
    {1, ddr_mod20_8_pat, "Mode 20-8bit test"},                         /* Mode 20-8bit test */
    {1, ddr_addr_walking, "Go 1 address test"},                          /* Go 1 address test*/
    {0, ddr_march_c, "March C test"},                               /* March Ctest, no copying */
    {1, ddr_march_g, "March G test"},                               /* March G test */
    {1, ddr_galloping, "Galloping test "},                          /* Galloping test */
    {1, ddr_column_operate, "Column operate test"},                 /* Column operate test */
    {0, ddr_swap_blocks, " block move test"},                       /* block move test, no copying */
};

static inline ulong roundup(ulong value, ulong mask)
{
    return (value + mask) & ~mask;
}

#if 0
static inline void set_vars(ulong start, ulong end)
{
    int i = 0;

    for (i = 0; i < v->msegs; i++) {
        v->pmap[i].start = (uint)start;
        v->pmap[i].end = (uint)end;
        v->map[i].start = (ulong *)start;
        v->map[i].end = (ulong *)end;
    }
}
#endif

#if 1
void rand_seed(unsigned int seed1, unsigned int seed2)
{
    if (seed1)
        SEED_X = seed1;         /* use default seeds if parameter is 0 */
    if (seed2)
        SEED_Y = seed2;
}

static inline unsigned int ft_rand()
{
    static unsigned int a = 18000, b = 30903;

    SEED_X = a * (SEED_X & 65535) + (SEED_X >> 16);
    SEED_Y = b * (SEED_Y & 65535) + (SEED_Y >> 16);

    return ((SEED_X << 16) + (SEED_Y & 65535));
}
#else
static inline unsigned long ft_rand()
{
    if (sizeof(ulong) == 8) {
        return ((ulong)rand() << 32) | rand();
    } 

    return rand();
}

#endif
/**
 * addrress_walking
 *
 * Use the "address test" algorithm
 * The address translation rules for writing data are as follows:
 * address = initial address | (mask << 1)
 *
 * No return value
 */ 
static int addrress_walking(char *desc)
{
    int     i, j, k;
    uint32  *pt, *end, mask, bank;
    
    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }
    
    /* Test the global address bits */
    for (p1 = 0, j = 0; j < 2; j++) {
        /* Set the initial address 0x20000 to align */
        pp = (uint32 *)roundup((ulong)v->map[0].start, 0x1ffff);
        *pp = p1;

        /* Now write pattern compliment */
        p1 = ~p1;
        end = v->map[segs - 1].end;
        for (i = 0; i < 1000; i++) {
            mask = 4;
            do {
                pt = (uint32 *) ((ulong) pp | mask);
                if (pt == pp) {
                    mask = mask << 1;
                    continue;
                }
                if (pt >= end) {
                    break;
                }
                *pt = p1;
                if (*pp != ~p1) {
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, ~p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                mask = mask << 1;
            } while (mask);
        }
        FT_DDR_SHOW();
    }

    /* Now check the address bits in each bank
     * If we have more than 8mb of memory then the bank size must be
     * bigger than 256k.  If so use 1mb for the bank size.
     */
    if (v->pmap[v->msegs - 1].end > (0x800000 >> 12)) {
        bank = 0x100000;
    } else {
        bank = 0x40000;
    }
    
    for (p1 = 0, k = 0; k < 2; k++) {
        for (j = 0; j < segs; j++) {
            pp = v->map[j].start;
            /* Set the initial address to 256k alignment */
            pp = (uint32 *) roundup((ulong) pp, bank - 1);
            end = v->map[j].end;
            while (pp < end) {
                *pp = p1;
                p1 = ~p1;
                for (i = 0; i < 200; i++) {
                    mask = 4;
                    do {
                        pt = (uint32 *)((ulong) pp | mask);
                        if (pt == pp) {
                            mask = mask << 1;
                            continue;
                        }
                        if (pt >= end) {
                            break;
                        }
                        *pt = p1;
                        if (*pp != ~p1) {
                            sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, ~p1, *pp);
                            fail = 1;
                            return FT_DDR_ERR;
                        }
                        mask = mask << 1;
                    } while (mask);
                }
                if (pp + bank > pp) {
                    pp += bank;
                } else {
                    pp = end;
                }
                p1 = ~p1;
            }
        }
        p1 = ~p1;
        FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

/**
 * moving_inversions
 * @iter: number of repetitions
 * @p1: The data is written for pattern1
 * @p2: The data is written for pattern2
 *
 * Use the "move inversion" algorithm to write memory addresses from low to high and from high to address;
 * First write p1 data format to the test memory area, and verify that the written data is correct;
 * Write ~p1 data format to the test memory area, and verify that the written data is correct;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int moving_inversions(char * desc, int iter, uint32 p1, uint32 p2)
{
    int     i, j, done;
    uint32  *pe;
    uint32  *start, *end;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }
    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "Iter:%d P1:0x%x P2:0x%x.\n", iter, p1, p2);
    /* From the low address to the high address, write p1 values to the test memory area  */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }

            for (; pp < pe; pp++) {
                *pp = p1;
            }
        } while (!done);
        FT_DDR_SHOW();
    }

    for (i = 0; i < iter; i++) {
        /* Validate write data, from low to high address, write p2 value to test memory area */
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                for (; pp < pe; pp++) {
                    if (*pp != p1) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = p2;
                }
            } while (!done);
            FT_DDR_SHOW();
        }
        
        /* Verify the write data, from the high address to the low address, write p1 value to the test memory area */
        for (j = segs - 1; j >= 0; j--) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = end - 1;
            pp = end - 1;
            done = 0;
            do {
                /* Check for underflow */
                if (pe - SPINSZ < pe) {
                    pe -= SPINSZ;
                } else {
                    pe = start;
                }
                if (pe <= start) {
                    pe = start;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                do {
                    if (*pp != p2) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = p1;
                } while (pp-- > pe);
            } while (!done);
            FT_DDR_SHOW();
        }
    }

    return FT_DDR_SUCCESS;
}

/**
 * moving_inversions32
 * @iter: number of repetitions
 * @p1: indicates the pattern of data written to memory for the first time
 * @low_pattern: indicates the initial data pattern for writing data from low addresses to high addresses
 * @high_pattern: indicates the initial data pattern for writing data from high addresses to low addresses
 * @sval: The value required or on the data pattern for each modification
 * @offset: indicates the bit offset when the memory is written for the first time. low_pattern = p1 >> offset
 *
 * Use the "move inverse-32bit" algorithm, which takes 32 write operations as a cycle, each write operation
 * After doing, for low address to high address write memory, write data pattern needs to move 1 bit left;
 * For high address to low address write memory, write data pattern needs to be shifted 1 bit to the right.
 *
 * Returns 0 on success; Return 1 on failure
 */
static int moving_inversions32(char *desc, int iter, uint32 p1, uint32 low_pattern, uint32 high_pattern, int sval, int offset)
{
    int     i, j, k = 0, done;
    uint32  *pe;
    uint32  *start, *end;
    uint32   pattern = 0;
    uint32   p3 = sval << 31;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "Iter:%d P1:0x%x low_pattern:0x%x high_pattern:0x%x sval:%d offset:%d.\n", 
                    iter, p1, low_pattern, high_pattern, sval, offset);
    /* Writes pattern values to the test memory area from low to high addresses  */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        k = offset;
        pattern = p1;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            while (pp < pe) {
                *pp = pattern;
                if (++k >= 32) {
                    pattern = low_pattern;
                    k = 0;
                } else {
                    pattern = pattern << 1;
                    pattern |= sval;
                }
                pp++;
            }
        } while (!done);
        FT_DDR_SHOW();
    }

    for (i = 0; i < iter; i++) {
        /* Validate write data, from low address to high address, write pattern value to test memory area */
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;
            k = offset;
            pattern = p1;
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                while (pp < pe) {
                    if (*pp != pattern) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, pattern, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, pattern, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = ~pattern;
                    if (++k >= 32) {
                        pattern = low_pattern;
                        k = 0;
                    } else {
                        pattern = pattern << 1;
                        pattern |= sval;
                    }
                    pp++;
                }               
            } while (!done);
            FT_DDR_SHOW();
        }

        /* Validate write data, from high address to low address, write pattern value to test memory area */
        pattern = low_pattern;
        if (0 != (k = (k - 1) & 31)) {
            pattern = (pattern << k);
            if (sval)
                pattern |= ((sval << k) - 1);
        }
        k++;
        for (j = segs - 1; j >= 0; j--) {
            start = v->map[j].start;
            end = v->map[j].end;
            pp = end - 1;
            pe = end - 1;
            done = 0;
            do {
                /* Check for underflow */
                if (pe - SPINSZ < pe) {
                    pe -= SPINSZ;
                } else {
                    pe = start;
                }
                if (pe <= start) {
                    pe = start;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                do {
                    if (*pp != ~pattern) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, ~pattern, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, ~pattern, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = pattern;
                    if (--k <= 0) {
                        pattern = high_pattern;
                        k = 32;
                    } else {
                        pattern = pattern >> 1;
                        pattern |= p3;
                    }
                } while (pp-- > pe);                 
            } while (!done);
            FT_DDR_SHOW();
        }
    }

    return FT_DDR_SUCCESS;
}

/**
 * modtst
 * @offset: indicates the offset address of the P1 value
 * @iter: number of times to write P2
 * @p1: indicates written data 1
 * @p2: indicates written data 2
 *
 * Write p1 data to the offset address of 20 * n + offset in the test memory area; To other
 * Offset address write p2 data, repeat iter times; Verify the offset address of 20 * n + offset
 * Whether the data written is correct.
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int modtst(char *desc, int offset, int iter, uint32 p1, uint32 p2)
{
    int     j, k, l, done;
    uint32  *pe;
    uint32  *start, *end;
    
    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "offset:%d Iter:%d P1:0x%x p2:0x%x.\n", offset, iter, p1, p2);
    /* Write p1 data to the offset address of 20 * n + offset in the test memory area*/
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = (uint32 *) start;
        pp = start + offset;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }

            for (; pp < pe; pp += MOD_SZ) {
                *pp = p1;
            }
             
        } while (!done);
        FT_DDR_SHOW();
    }

    /* Write p2 data to other offset addresses, repeat iter times */
    for (l = 0; l < iter; l++) {
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = (uint32 *) start;
            pp = start;
            done = 0;
            k = 0;
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                for (; pp < pe; pp++) {
                    if (k != offset) {
                        *pp = p2;
                    }
                    if (++k > MOD_SZ - 1) {
                        k = 0;
                    }
                }

            } while (!done);
            FT_DDR_SHOW();
        }
    }

    /* Now check every nth location */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = (uint32 *) start;
        pp = start + offset;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }

            for (; pp < pe; pp += MOD_SZ) {
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
            }
             
        } while (!done);
        FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

/**
 * march_c
 * @iter: number of repetitions
 * @p1: The data is written for pattern1
 * @p2: The data is written for pattern2
 *
 * Low address to high address, read p1, write p2, read p2, write p1
 * High address to low address, read p1, write p2, read p2, write p1
 *
 * No return value
 */ 
static int march_c(char *desc, int iter, uint32 p1, uint32 p2)
{
    int     i, j, done;
    uint32  *pe;
    uint32  *start, *end;
    uint32   p_temp;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "Iter:%d P1:0x%x p2:0x%x.\n", iter, p1, p2);
    /* From low address to high address, write p1  */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                *pp = p1;
            }
        } while (!done);
        FT_DDR_SHOW();
    }
    
    /* From low address to high address, read p1, write p2, read p2 again, write p1 */
    for (i = 0; i < 2; i++) {
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;
            do {
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }
                for (; pp < pe; pp++) {
                    if (*pp != p1) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = p2;
                }
                 
            } while (!done);
            FT_DDR_SHOW();
        }
        /* p1 and p2 values are exchanged */
        p_temp = p1;
        p1 = p2;
        p2 = p_temp;
    }

    /* High address to low address, read p1, write p2, read p2, write p1 */
    for (i = 0; i < 2; i++) {
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = end - 1;
            pp = end - 1;
            done = 0;
            do {
                /* Check for underflow */
                if (pe - SPINSZ < pe) {
                    pe -= SPINSZ;
                } else {
                    pe = start;
                }
                if (pe <= start) {
                    pe = start;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                do {
                    if (*pp != p1) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = p2;
                } while (pp-- > pe);
                 
            } while (!done);
            FT_DDR_SHOW();
        }
        
        /* p1 and p2 values are exchanged */
        p_temp = p1;
        p1 = p2;
        p2 = p_temp;
    }
    
    /* Verify the last p1 value written */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
            }
             
        } while (!done);
        FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

/**
 * march_g
 * @iter: number of repetitions
 * @p1: The data is written for pattern1
 * @p2: The data is written for pattern2
 *
 * From low address to high address, for the same address: read p1, write p2, read p2, write p1
 * From low address to high address, for the same address: read p2, write p1 and then p2
 * High address to low address, for the same address: read p2, write p1 first, write p2, then write p1
 * High address to low address, for the same address: read p1, first write p2, then write p1
 *
 * No return value
 */ 
static int march_g(char *desc, int iter, uint32 p1, uint32 p2)
{
    int     j, done;
    uint32  *pe;
    uint32  *start, *end;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "Iter:%d P1:0x%x p2:0x%x.\n", iter, p1, p2);
    /* From low address to high address, write p1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                *pp = p1;
            }
        } while (!done);
        FT_DDR_SHOW();
    }
    
    /* From low address to high address, read p1, write p2, read p2, and write p1 to the same address */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p2;
                if (*pp != p2) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p1;
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p2;
            }
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From low address to high address, read p2, write p1 and then p2 to the same address */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (*pp != p2) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p1;
                *pp = p2;
            }
        } while (!done);
        FT_DDR_SHOW();
    }
    
    /* From high address to low address, for the same address: read p2, write p1 first, write p2, and then write p1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = end - 1;
        pp = end - 1;
        done = 0;
        do {
            /* Check for underflow */
            if (pe - SPINSZ < pe) {
                pe -= SPINSZ;
            } else {
                pe = start;
            }
            if (pe <= start) {
                pe = start;
                done++;
            }
            if (pp == pe) {
                break;
            }

            do {
                if (*pp != p2) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p1;
                *pp = p2;
                *pp = p1;
            } while (pp-- > pe);
             
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From high address to low address, for the same address: read p1, write p2, and then write p1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = end - 1;
        pp = end - 1;
        done = 0;
        do {
            /* Check for underflow */
            if (pe - SPINSZ < pe) {
                pe -= SPINSZ;
            } else {
                pe = start;
            }
            if (pe <= start) {
                pe = start;
                done++;
            }
            if (pp == pe) {
                break;
            }

            do {
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
                *pp = p2;
                *pp = p1;
            } while (pp-- > pe);
             
        } while (!done);
        FT_DDR_SHOW();
    }
    
    /* From high address to low address, read p1 for the same address */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (*pp != p1) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
            }
             
        } while (!done);
        FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

/**
 * column_operate
 * @iter: number of repetitions
 * @p1: The data is written for pattern1
 * @col_num: indicates the column address offset
 *
 * Adopts the "column operation" algorithm
 * From the low address to the high address, write p1 to the (n * step + col_addr) address
 *
 * No return value
 */ 
static int column_operate(char *desc, int iter, uint32 p1, uint32 col_num)
{
    int     j, k, done;
    uint32  *pe;
    uint32  *start, *end;
    ulong   n;
    ulong   step = 0x2000;
    ulong   addr;
    ulong   col_addr;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }
    
    /* From the low address to the high address, write p1 to the (n * step + col_addr) address */
    for (k = 0; k < iter; k++) {

        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;

            if (((ulong) start % step) <= (col_num & 0x3FF) << 3) {
                n = (ulong) start / step;
            } else {
                n = (ulong) start / step + 1;
            }
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp >= pe) {
                    break;
                }
                do {
                    col_addr = (col_num & 0x3FF) << 3;
                    addr = n * step + col_addr;
                    pp = (uint32 *) addr;
                    if (pp >= end)
                        break;
                    *pp = p1;
                    n++;
                } while (pp < pe);
            } while (!done);
        }
        //FT_DDR_SHOW();

        /* From low address to high address, verify the (n * step + col_addr) address value */
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;
            if (((ulong) start % step) < (col_num & 0x3FF) << 3) {
                n = (ulong) start / step;
            } else {
                n = (ulong) start / step + 1;
            }
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp >= pe) {
                    break;
                }
                do {
                    col_addr = (col_num & 0x3FF) << 3;
                    addr = n * step + col_addr;
                    pp = (uint32 *) addr;
                    if (pp >= end)
                        break;
                    if (*pp != p1) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p1, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    n++;
                } while (pp < pe);  
            } while (!done);
        }
        //FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

/**
 * galloping
 * @p1: The data is written for pattern1
 * @p2: The data is written for pattern2
 *
 * From low address to high address, write p2 to addr, read addr+1, and p1 to addr+1
 * From high address to low address, write p2 to addr, read ADDR-1, read addr, and write p1 to ADDR-1
 *
 * No return value
 */ 
static int galloping(char *desc, uint32 p1, uint32 p2)
{
    int     j, done;
    uint32  *pe;
    uint32  *start, *end;

    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }
    
    /* From low address to high address, write p1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                *pp = p1;
            }
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From low address to high address, write p2 to addr, read addr+1, read addr, and write p1 to addr+1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (pp + 1 >= pe)
                    break;
                *pp = p2;
                *(pp + 1) = p1;
            }
            *pp = p2;
             
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From low address to high address, read p2 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                if (*pp != p2) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
            }   
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From high address to low address, write p2 to addr, read ADDR-1, read addr, and write p1 to ADDR-1 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = end - 1;
        pp = end - 1;
        done = 0;
        do {
            /* Check for underflow */
            if (pe - SPINSZ < pe) {
                pe -= SPINSZ;
            } else {
                pe = start;
            }
            if (pe <= start) {
                pe = start;
                done++;
            }
            if (pp == pe) {
                break;
            }

            do {
                if (pp - 1 <= pe)
                    break;
                *pp = p2;
                *(pp - 1) = p1;
            } while (pp-- > pe);
            *pp = p2;
        } while (!done);
        FT_DDR_SHOW();
    }

    /* From high address to low address, read p2 */
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = end - 1;
        pp = end - 1;
        done = 0;
        do {
            /* Check for underflow */
            if (pe - SPINSZ < pe) {
                pe -= SPINSZ;
            } else {
                pe = start;
            }
            if (pe <= start) {
                pe = start;
                done++;
            }
            if (pp == pe) {
                break;
            }

            do {
                if (*pp != p2) {
                    FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, p2, *pp);
                    fail = 1;
                    return FT_DDR_ERR;
                }
            } while (pp-- > pe);
             
        } while (!done);
        FT_DDR_SHOW();
    }

    return FT_DDR_SUCCESS;
}

#if 0
void dma_mov_addr(int chan_id, int from_addr, int to_addr, int size)
{
    dma_tx_data_init();
    get_dma_chan_cb(chan_id);
    g_dma_tx_data[chan_id][0].src_addr = from_addr;
    g_dma_tx_data[chan_id][0].dest_addr = to_addr;
    g_dma_tx_data[chan_id][0].size = size;
    dma_move(chan_id, 1);
}
#endif

static void block_move_use_long_type(ulong *s, ulong *d, int blk_size)
{
    uint32 k;

    for (k = 0; k < blk_size / sizeof(ulong); k++) {
        *(d++) = *(s++);
    }
}

/**
 * swap_blocks
 * @iter: Number of repetitions
 * @swap_times: indicates the number of block exchange repetitions
 * @blk_size: indicates the size of each memory block
 *
 * The memory area to be tested is divided into n+1 blocks, with the n+1 block as a temporary swap block
 * From block 1 to block n, exchange the data of two adjacent blocks
 * From block n to block 1, exchange the data of two adjacent blocks
 *
 * No return value
 */ 
static int swap_blocks(char *desc, int iter, int swap_times, ulong blk_size)
{
    int     i, j, done;
    uint32  *pe;
    uint32  *start, *end;
    uint32  *ptemp;
    uint32  *pprev;
    uint32  *pnext;
    ulong   n;
    int     t;
    int     seed1, seed2;
    int     times;
    uint32   num;
    uint32  *pend;
    ulong   test_size;
    
    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }

    iter = 1;
    for (times = 0; times < iter; times++) {
        start = v->map[0].start;
        end = v->map[0].end;
        pe = start;
        pp = start;
        done = 0;
        seed1 = 521288629;
        seed2 = 362436069;

        rand_seed(seed1, seed2);
        
        /* From low address to high address, write rand() */
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                *pp = ft_rand();
            }
        } while (!done);
        FT_DDR_SHOW();

        start = v->map[0].start;
        end = v->map[0].end;
        pe = start;
        pp = start;
        done = 0;

        for (t = 0; t < swap_times; t++) {
            test_size = (ulong)end - (ulong)start;
            n = test_size / blk_size;
            FAC_LOG_DBG(GRTD_LOG_DEBUG,"start = %p, end = %p, test_size:0x%lx n = %lu\r\n", start, end, test_size, n);
            
            /* The last full block serves as the swap block */
            while (1) {
                ptemp = (uint32 *)((ulong)start + (n + 1) * blk_size);
                pend = (uint32 *)((ulong)ptemp + blk_size);
                if (pend >= end) {
                    n--;
                } else {
                    break;
                }
            }
            FT_DDR_SHOW();
            FAC_LOG_DBG(GRTD_LOG_DEBUG,"n = %lu\r\n", n);

            FAC_LOG_DBG(GRTD_LOG_DEBUG,"Test form 1 to %lu.\r\n", n);
            /* From the first block to the n block, the data of the two adjacent blocks is exchanged */
            for (i = 0; (ulong)i < n; i++) {
                pprev = (uint32 *) ((ulong) start + i * blk_size);
                for (j = i + 1; (ulong)j <= n; j++) {
                    /* All data of the current block i is written to the swap block */
                    pprev = (uint32 *) ((ulong) start + i * blk_size);
                    ptemp = (uint32 *) ((ulong) start + (n + 1) * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pprev:%p ptemp:%p.\r\n", pprev, ptemp);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)pprev, (ulong *)ptemp, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong)pprev, (ulong)ptemp, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    //return;
#endif  

                    /* All the data of the current i+1 block is written to the current i block */
                    pprev = (uint32 *)((ulong) start + i * blk_size);
                    pnext = (uint32 *)((ulong) start + j * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pprev:%p pnext:%p.\r\n", pprev, pnext);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)pnext, (ulong *)pprev, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong) pnext, (ulong) pprev, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }

#endif
                    /* All the data of the current switch block is written to the current i+1 block */
                    pnext = (uint32 *) ((ulong) start + j * blk_size);
                    ptemp = (uint32 *) ((ulong) start + (n + 1) * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pnext:%p ptemp:%p.\r\n", pnext, ptemp);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)ptemp, (ulong *)pnext, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong) ptemp, (ulong) pnext, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }

#endif
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"test time %d-%d\r\n", i, j);
                }
                FT_DDR_SHOW();
            }

            FAC_LOG_DBG(GRTD_LOG_DEBUG,"Test form %lu to 1.\r\n", n);
            ptemp = (uint32 *) ((ulong) start + (n + 1) * blk_size);
            /* From block n to block 1, the data of the two adjacent blocks is exchanged */
            for (i = n; i > 0; i--) {
                pprev = (uint32 *) ((ulong) start + i * blk_size);
                for (j = i - 1; j >= 0; j--) {
                    /* All data of the current block i is written to the swap block */
                    pprev = (uint32 *) ((ulong) start + i * blk_size);
                    ptemp = (uint32 *) ((ulong) start + (n + 1) * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pprev:%p ptemp:%p.\r\n", pprev, ptemp);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)pprev, (ulong *)ptemp, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong) pprev, (ulong) ptemp, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }

#endif
                    /* All the data of the current block I-1 is written to the current block i */
                    pnext = (uint32 *) ((ulong) start + j * blk_size);
                    pprev = (uint32 *) ((ulong) start + i * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pnext:%p pprev:%p.\r\n", pnext, pprev);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)pnext, (ulong *)pprev, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong) pnext, (ulong) pprev, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }

#endif
                    /* All the data of the current switch block is written to the current block i-1 */
                    pnext = (uint32 *) ((ulong) start + j * blk_size);
                    ptemp = (uint32 *) ((ulong) start + (n + 1) * blk_size);
                    //FAC_LOG_DBG(GRTD_LOG_DEBUG,"pnext:%p ptemp:%p.\r\n", pnext, ptemp);
#ifndef USE_DMA
                    block_move_use_long_type((ulong *)ptemp, (ulong *)pnext, blk_size);
#else
                    ret = dma_mov_addr(0, (ulong) ptemp, (ulong) pnext, blk_size);
                    if (ret) {
                        fail = 1;
                        return FT_DDR_ERR;
                    }

#endif
                   //FAC_LOG_DBG(GRTD_LOG_DEBUG,"test time %d-%d\r\n", i, j);
                }
                FT_DDR_SHOW();
            }

            /* Low address to high address, read rand() */
            pe = start;
            pp = start;
            done = 0;
            end = (uint32 *) ((ulong)pend - blk_size);
            rand_seed(seed1, seed2);
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }
                for (; pp < pe; pp++) {
                    num = ft_rand();
                    if (*pp != num) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, num, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, num, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                }
            } while (!done);
            FT_DDR_SHOW();
        }
    }

    return FT_DDR_SUCCESS;
}

/**
 * ft_ddr_test_init
 *
 * Initializes the area of memory to be tested, defaulting to all ranges allowed by the factory test
 *
 * No return valu
 */ 
void ft_ddr_test_init(void *start, size_t size)
{
    int i;

    v->rdtsc = 1;
    v->msegs = 1;  /* The number of memory areas currently set for testing is 1 */
    fail = 0;
    segs = 1;

    FAC_LOG_DBG(GRTD_LOG_ERR, "star:%p size:0x%lx(%lu MB).\n", start, size, size / 1024 / 1024);
    for (i = 0; i < v->msegs; i++) {
        v->pmap[i].start = (ulong)start;
        v->pmap[i].end = ((ulong)start + size);
        v->map[i].start = (uint32 *)start;
        v->map[i].end = (uint32 *)((ulong)start + size);
    }
}

/**
 * ddr_half_mov_inv_random_pat
 *
 * Use "half-move inversion" algorithm to write memory address from low address to high address;
 * First write p1 data format to the test memory area, and verify that the written data is correct;
 * Write ~p1 data format to the test memory area, and verify that the written data is correct;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_half_mov_inv_random_pat(char *desc)
{
    int     i, j, done, seed1, seed2;
    uint32  *pe;
    uint32  *start, *end;
    uint32   num;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_half_mov_inv_random_pat test.\n");
    if (fail) {
        sprintf(desc, "No test.");
        return FT_DDR_ERR;
    }
    /* Initialize memory with initial sequence of random numbers.  */
    seed1 = 521288629 + v->pass;
    seed2 = 362436069 - v->pass;
    rand_seed(seed1, seed2);
    
    for (j = 0; j < segs; j++) {
        start = v->map[j].start;
        end = v->map[j].end;
        pe = start;
        pp = start;
        done = 0;
        do {
            /* Check for overflow */
            if (pe + SPINSZ > pe) {
                pe += SPINSZ;
            } else {
                pe = end;
            }
            if (pe >= end) {
                pe = end;
                done++;
            }
            if (pp == pe) {
                break;
            }
            for (; pp < pe; pp++) {
                *pp = ft_rand();
            }
        } while (!done);
        printf(".");
    }

    /* Do moving inversions test. Check for initial pattern and then
     * write the complement for each memory location. Test from bottom
     * up and then from the top down.  
     */
    for (i = 0; i < 2; i++) {
        rand_seed(seed1, seed2);
        for (j = 0; j < segs; j++) {
            start = v->map[j].start;
            end = v->map[j].end;
            pe = start;
            pp = start;
            done = 0;
            do {
                /* Check for overflow */
                if (pe + SPINSZ > pe) {
                    pe += SPINSZ;
                } else {
                    pe = end;
                }
                if (pe >= end) {
                    pe = end;
                    done++;
                }
                if (pp == pe) {
                    break;
                }

                for (; pp < pe; pp++) {
                    num = ft_rand();
                    if (i) {
                        num = ~num;
                    }
                    if (*pp != num) {
                        FAC_LOG_DBG(GRTD_LOG_ERR, "[%p] should be 0x%x,but is 0x%x.\n", pp, num, *pp);
                        sprintf(desc, "[%p] should be 0x%x,but is 0x%x.\n", pp, num, *pp);
                        fail = 1;
                        return FT_DDR_ERR;
                    }
                    *pp = ~num;
                }
            } while (!done);
        }
    }

    return FT_DDR_SUCCESS;
}

/**
 * ddr_mov_inv_random_pat
 *
 * Use the "move inversion" algorithm to write memory addresses from low to high and from high to address;
 * First write p1 data format to the test memory area, and verify that the written data is correct;
 * Write ~p1 data format to the test memory area, and verify that the written data is correct;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mov_inv_random_pat(char *desc)
{
    /* Random Data */
    p1 = ft_rand();
    p2 = ~p1;
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mov_inv_random_pat test.\n");
    return moving_inversions(desc, 2, p1, p2);
}

/**
 * ddr_mov_inv_0_1
 * @show: indicates the information output control parameter
 *
 * Use the "move reversal" algorithm
 * Data format :p1 = 0; p2 = ~0; Or p1 = ~0; p2 = 0;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mov_inv_0_1(char *desc)
{
    static unsigned int test_times = 0;
    
    test_times++;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mov_inv_0_1 test.\n");
    /* Moving inversions, all ones and zeros */
    if (test_times % 2 == 1) {
        p1 = 0;
        p2 = ~p1;
    } else {
        /* Switch patterns */
        p2 = p1;
        p1 = ~p2;
    }
    
    return moving_inversions(desc, 3, p1, p2);
}

/**
 * ddr_mov_inv_8_pat
 * @show: indicates the information output control parameter
 *
 * Use the "move reversal" algorithm
 * Data format:
 * After each call to this function, p0 moves 1 bit to the left
 * p1 = p0; p2 = ~p0; Or p1 = ~p0; p2 = p0;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mov_inv_8_pat(char *desc)
{
    static int times = 0;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mov_inv_8_pat test.\n");
    /* Moving inversions, 8 bit wide walking ones and zeros. */
    if (times == 0) {
        p0 = 0x80;
    } else {
        p0 = p0 >> 1;
    }
    p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);

    if (times % 2 == 0) {
        p2 = ~p1;
    } else {
        /* Switch patterns */
        p2 = p1;
        p1 = ~p2;
    }
    times++;
    
    if (times >= 8) {
        times = 0;
    }
    
    return moving_inversions(desc, 3, p1, p2);
}

/**
 * ddr_mov_inv_32_pat
 * @show: indicates the information output control parameter
 *
 * Use the "Move inversion 32" algorithm
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mov_inv_32_pat(char *desc)
{
    static int times = 0;
    static unsigned int test_times = 0;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mov_inv_32_pat test.\n");
    test_times++;
    if (times == 0) {
        p1 = 1;
    } else {
        p1 = p1 << 1;
        times ++;
        if (times >= 32)
            times = 0;
    }

    if (moving_inversions32(desc, 2, p1, 1, 0x80000000, 0, times) || 
        moving_inversions32(desc, 2, ~p1, 0xfffffffe, 0x7fffffff, 1, times)) {
        return FT_DDR_ERR;
    }

    return FT_DDR_SUCCESS;
}

/**
 * ddr_mod20_random_pat
 * @show: indicates the information output control parameter
 *
 * Adopts the "module 20" algorithm
 * Data format:
 * p1 = ft_rand(); p2 = ~ft_rand(); Or p1 = ~ft_rand(); p2 = ft_rand();
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mod20_random_pat(char *desc)
{
    static int mod20_rand_times = 0;
    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mod20_random_pat test.\n");
    /* Modulo 20 check, Random pattern */
    if (mod20_rand_times % 40 == 0) {
        p1 = ft_rand();
        mod20_rand_times = 0;
    } else {
        mod20_rand_times++;
    }

    if (mod20_rand_times % 2 == 0) {
        p2 = ~p1;
    } else {
        /* Switch patterns */
        p2 = p1;
        p1 = ~p2;
    }
        
    return modtst(desc, mod20_rand_times, 3, p1, p2);
}

/**
 * ddr_mod20_0_1_pat
 * @show: indicates the information output control parameter
 *
 * Adopts the "module 20" algorithm
 * Data format :p1 = 0; p2 = ~0; Or p1 = ~0; p2 = 0;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mod20_0_1_pat(char *desc)
{
    static int ddr_mod20_0_1_times = 0;
    int     times;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mod20_0_1_pat test.\n");
    /* Modulo 20 check, all ones and zeros */
    p1 = 0;
    times = ddr_mod20_0_1_times % MOD_SZ;
    ddr_mod20_0_1_times++;
    
    if (times % 2 == 0) {
        p2 = ~p1;
        /* modtst(times, 3, p1, p2); */
    } else {
        /* Switch patterns */
        p2 = p1;
        p1 = ~p2;
        /* modtst(times, 3, p1, p2); */
    }
    
    return modtst(desc, times, 3, p1, p2);
}

/**
 * ddr_mod20_8_pat
 * @show: indicates the information output control parameter
 *
 * Adopts the "module 20" algorithm
 * Data format:
 * After each call to this function, p0 moves 1 bit to the left
 * p1 = p0; p2 = ~p0; Or p1 = ~p0; p2 = p0;
 *
 * Returns 0 on success; Return 1 on failure
 */ 
static int ddr_mod20_8_pat(char *desc)
{
    static int pass = 0;
    static int ddr_mod20_8_pat_times = 0;
    int     times;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_mod20_8_pat test.\n");
    /* Modulo 20 check, 8 bit pattern */
    pass = ddr_mod20_8_pat_times / MOD_SZ;
    pass = pass % 8;
    if (pass == 0) {
        p0 = 0x80;
    } else {
        p0 = p0 >> 1;
    }

    p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);

    times = ddr_mod20_8_pat_times % MOD_SZ;
    if (times % 2 == 0) {
        p2 = ~p1;
    } else {
        /* Switch patterns */
        p2 = p1;
        p1 = ~p2;
    }
    ddr_mod20_8_pat_times ++;

    return modtst(desc, times, 2, p1, p2);
}

static int ddr_addr_walking(char *desc)
{ 
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_addr_walking test.\n");
    return addrress_walking(desc);
}

static int ddr_march_c(char *desc)
{
    static unsigned int test_times = 0;
    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_march_c test.\n");
    test_times++;
    if (test_times == 1) {
        p1 = 0;
    } else {
        p1 = ft_rand();
    }
    p2 = ~p1;

    return march_c(desc, 1, p1, p2);
}

static int ddr_march_g(char *desc)
{
    static unsigned int test_times = 0;
    
    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_march_g test.\n");
    test_times++;
    if (test_times == 1) {
        p1 = 0;
    } else {
        p1 = ft_rand();
    }
    p2 = ~p1;

    return march_g(desc, 1, p1, p2);
}

static int ddr_galloping(char *desc)
{
    static unsigned int test_times=0;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_galloping test.\n");
    test_times ++;
    if (test_times == 1) {
        p1 = 0;
    } else {
        p1 = ft_rand();
    }
    p2 = ~p1;
    
    return galloping(desc, p1, p2);
}

static int ddr_column_operate(char *desc)
{
    int i;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_column_operate test.\n");
    for (i = 0; i < 1023; i++) {
        p1 = ~i;
        /* column_operate(1, p1, i); */
        if (column_operate(desc, 1, p1, i)) {
            return FT_DDR_ERR;
        }
        if (i % 100 == 0) {
            FT_DDR_SHOW();
        }
    }

    return FT_DDR_SUCCESS;
}

static int ddr_swap_blocks(char *desc)
{
    static unsigned int test_times = 0;
    ulong blk_size;
    int ret;
    int swap_times = 1;
    struct vars v_tmp;

    FAC_LOG_DBG(GRTD_LOG_DEBUG, "ddr_swap_blocks test.\n");
    test_times++;
    blk_size = (ulong)blk_size_array[1]; /* 24MB */

    memcpy(&v_tmp, v, sizeof(struct vars));
    if (((ulong)v->map[0].end - (ulong)v->map[0].start) > MEMORY_SIZE_1G) {
        FAC_LOG_DBG(GRTD_LOG_DEBUG, "set memory size to 0x%lx.\n", MEMORY_SIZE_1G);
        v->map[0].end = (unsigned int *)((ulong)v->map[0].start + MEMORY_SIZE_1G);
    }
    ret = swap_blocks(desc, test_times, swap_times, blk_size);
    strncpy((char *)(v), (char *)(&v_tmp), sizeof(struct vars));

    return ret;
}
