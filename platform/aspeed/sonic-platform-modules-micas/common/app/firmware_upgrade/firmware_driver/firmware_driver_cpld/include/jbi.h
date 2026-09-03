#ifndef __JBI_H__
#define __JBI_H__

#include <linux/types.h>

/* JTAG operation interface*/
extern int jbi_jtag_io_(int tms, int tdi, int read_tdo);
/* delay function */
extern void jbi_jtag_udelay(unsigned long us);
/* Debug switch */
extern int jbi_debug(int level);
/* JBI upgrade function */
extern int jbi_main(unsigned char *addr, unsigned long size, int argc, char * const argv[]);

#endif /* __JBI_JTAG_H__ */
