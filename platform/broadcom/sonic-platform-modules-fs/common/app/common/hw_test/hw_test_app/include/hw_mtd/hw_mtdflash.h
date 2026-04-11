#ifndef _HW_NANDFLASH_H
#define _HW_NANDFLASH_H

struct mtd_dev_priv {
    unsigned int start_addr;
    unsigned int end_addr;
    unsigned char data1;
    unsigned char data2;
    int times;
    unsigned int check_test_errors;
    unsigned char *buffer;
};

#define  MAX_BUFFER_SIZE                128
#define  NAND_TEST_PARTITION            "factory_test1"
#define  NOR_TEST_PARTITION             "factory_test0"

extern int nand_wr_main(int argc, char **argv);
extern int nand_rd_main(int argc, char **argv);
extern int nand_chk_main(int argc, char **argv);
extern int nor_wr_main(int argc, char **argv);
extern int nor_rd_main(int argc, char **argv);
extern int nor_chk_main(int argc, char **argv);

#endif /* _HW_NANDFLASH_H */
