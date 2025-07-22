#ifndef _HW_MISC_H
#define _HW_MISC_H

typedef unsigned long long  u64;
#define MAX_DATA_SIZE      16

extern int reload_main(int argc, char **argv);
extern int sys_info_main(int argc, char **argv);
extern int mem_dump_main(int argc, char **argv);
extern int cache_flush_main(int argc, char **argv);
extern int reg_wr64_main(int argc, char **argv);
extern int reg_rd64_main(int argc, char **argv);
extern int reg_wr32_main(int argc, char **argv);
extern int reg_rd32_main(int argc, char **argv);
extern int reg_wr16_main(int argc, char **argv);
extern int reg_rd16_main(int argc, char **argv);
extern int reg_wr8_main(int argc, char **argv);
extern int reg_rd8_main(int argc, char **argv);
extern int phydev_list_main(int argc, char **argv);
extern int phydev_rd_main(int argc, char **argv);
extern int phydev_wr_main(int argc, char **argv);
extern int mdiodev_list_main(int argc, char **argv);
extern int mdiodev_rd_main(int argc, char **argv);
extern int mdiodev_wr_main(int argc, char **argv);

#endif /* _HW_MISC_H */
