#ifndef INV_SWPS_H
#define INV_SWPS_H

#include "transceiver.h"
#include "io_expander.h"

/* Module settings */
#define SWP_CLS_NAME          "swps"
#define SWP_DEV_PORT          "port"
#define SWP_DEV_MODCTL        "module"
#define SWP_RESET_PWD         "inventec"
#define SWP_POLLING_PERIOD    (300)  /* msec */
#define SWP_POLLING_ENABLE    (1)
#define SWP_AUTOCONFIG_ENABLE (1)

/* Module information */
#define SWP_AUTHOR            "Neil <liao.neil@inventec.com>"
#define SWP_DESC              "Inventec port and transceiver driver"
#define SWP_VERSION           "4.2.5"
#define SWP_LICENSE           "GPL"

/* Module status define */
#define SWP_STATE_NORMAL      (0)
#define SWP_STATE_I2C_DIE     (-91)

/* [Note]:
 *  Functions and mechanism for auto-detect platform type is ready,
 *  But HW and BIOS not ready! We need to wait them.
 *  So, please do not use PLATFORM_TYPE_AUTO until they are ready.
 *  (2016.06.13)
 */
#define PLATFORM_TYPE_SEQUOIA_GA    (171)
/* Current running platfrom */
#define PLATFORM_SETTINGS           PLATFORM_TYPE_SEQUOIA_GA

/* Define platform flag and kernel version */
#if (PLATFORM_SETTINGS == PLATFORM_TYPE_SEQUOIA_GA)
  #define SWPS_SEQUOIA           (1)
  #define SWPS_KERN_VER_BF_3_8   (1)
#endif


struct inv_platform_s {
    int  id;
    char name[64];
};

struct inv_ioexp_layout_s {
    int ioexp_id;
    int ioexp_type;
    struct ioexp_addr_s addr[4];
};

struct inv_port_layout_s {
    int port_id;
    int chan_id;
    int ioexp_id;
    int ioexp_offset;
    int transvr_type;
    int chipset_type;
    int lane_id[8];
};


/* ==========================================
 *   Inventec Platform Settings
 * ==========================================
 */
struct inv_platform_s platform_map = {PLATFORM_TYPE_SEQUOIA_GA,   "Sequoia_GA"   };

/* ==========================================
 *   Sequoia Layout configuration
 * ==========================================
 */

struct inv_ioexp_layout_s secquoia_ioexp_layout[] = {
    /* IOEXP_ID / IOEXP_TYPE / { Chan_ID, Chip_addr, Read_offset, Write_offset, config_offset, data_default, conf_default } */
    {0,  IOEXP_TYPE_SEQUOIA_NABC, { {2, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 0 A */
                                    {2, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 0 B */
                                    {2, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 0 C */
    },
    {1,  IOEXP_TYPE_SEQUOIA_NABC, { {3, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 1 A */
                                    {3, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 1 B */
                                    {3, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 1 C */
    },
    {2,  IOEXP_TYPE_SEQUOIA_NABC, { {4, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 2 A */
                                    {4, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 2 B */
                                    {4, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 2 C */
    },
    {3,  IOEXP_TYPE_SEQUOIA_NABC, { {5, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 3 A */
                                    {5, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 3 B */
                                    {5, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 3 C */
    },
    {4,  IOEXP_TYPE_SEQUOIA_NABC, { {6, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 4 A */
                                    {6, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 4 B */
                                    {6, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 4 C */
    },
    {5,  IOEXP_TYPE_SEQUOIA_NABC, { {7, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 5 A */
                                    {7, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 5 B */
                                    {7, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 5 C */
    },
    {6,  IOEXP_TYPE_SEQUOIA_NABC, { {8, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 6 A */
                                    {8, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 6 B */
                                    {8, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 6 C */
    },
    {7,  IOEXP_TYPE_SEQUOIA_NABC, { {9, 0x20, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0x00, 0x00}, },    /* addr[0] = I/O Expander 7 A */
                                    {9, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0x00}, {0x00, 0xff}, },    /* addr[1] = I/O Expander 7 B */
                                    {9, 0x22, {0, 1}, {2, 3}, {6, 7}, {0x00, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 7 C */
    },
};


struct inv_port_layout_s secquoia_port_layout[] = {
    /* Port_ID / Chan_ID / IOEXP_ID / IOEXP_VIRT_OFFSET / TRANSCEIVER_TYPE / BCM_CHIP_TYPE / LANE_ID */
    { 0,  10,  0,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {  9,  10,  11,  12} },
    { 1,  11,  0,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {  1,   2,   3,   4} },
    { 2,  12,  0,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 25,  26,  27,  28} },
    { 3,  13,  0,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 17,  18,  19,  20} },
    { 4,  14,  0,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 41,  42,  43,  44} },
    { 5,  15,  0,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 33,  34,  35,  36} },
    { 6,  16,  0,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 57,  58,  59,  60} },
    { 7,  17,  0,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 49,  50,  51,  52} },
    { 8,  18,  1,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 73,  74,  75,  76} },
    { 9,  19,  1,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 65,  66,  67,  68} },
    {10,  20,  1,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 89,  90,  91,  92} },
    {11,  21,  1,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 81,  82,  83,  84} },
    {12,  22,  1,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {105, 106, 107, 108} },
    {13,  23,  1,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 97,  98,  99, 100} },
    {14,  24,  1,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {121, 122, 123, 124} },
    {15,  25,  1,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {113, 114, 115, 116} },
    {16,  26,  2,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {137, 138, 139, 140} },
    {17,  27,  2,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {129, 130, 131, 132} },
    {18,  28,  2,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {153, 154, 155, 156} },
    {19,  29,  2,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {145, 146, 147, 148} },
    {20,  30,  2,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {169, 170, 171, 172} },
    {21,  31,  2,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {161, 162, 163, 164} },
    {22,  32,  2,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {185, 186, 187, 188} },
    {23,  33,  2,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {177, 178, 179, 180} },
    {24,  34,  3,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {201, 202, 203, 204} },
    {25,  35,  3,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {193, 194, 195, 196} },
    {26,  36,  3,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {217, 218, 219, 220} },
    {27,  37,  3,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {209, 210, 211, 212} },
    {28,  38,  3,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {233, 234, 235, 236} },
    {29,  39,  3,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {225, 226, 227, 228} },
    {30,  40,  3,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {249, 250, 251, 252} },
    {31,  41,  3,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {241, 242, 243, 244} },
    {32,  45,  4,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 13,  14,  15,  16} },
    {33,  44,  4,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {  5,   6,   7,   8} },
    {34,  43,  4,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 29,  30,  31,  32} },
    {35,  42,  4,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 21,  22,  23,  24} },
    {36,  49,  4,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 45,  46,  47,  48} },
    {37,  48,  4,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 37,  38,  39,  40} },
    {38,  47,  4,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 61,  62,  63,  64} },
    {39,  46,  4,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 53,  54,  55,  56} },
    {40,  53,  5,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 77,  78,  79,  80} },
    {41,  52,  5,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 69,  70,  71,  72} },
    {42,  51,  5,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 93,  94,  95,  96} },
    {43,  50,  5,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 85,  86,  87,  88} },
    {44,  57,  5,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {109, 110, 111, 112} },
    {45,  56,  5,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {101, 102, 103, 104} },
    {46,  55,  5,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {125, 126, 127, 128} },
    {47,  54,  5,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {117, 118, 119, 120} },
    {48,  61,  6,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {141, 142, 143, 144} },
    {49,  60,  6,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {133, 134, 135, 136} },
    {50,  59,  6,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {157, 158, 159, 160} },
    {51,  58,  6,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {149, 150, 151, 152} },
    {52,  65,  6,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {173, 174, 175, 176} },
    {53,  64,  6,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {165, 166, 167, 168} },
    {54,  63,  6,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {189, 190, 191, 192} },
    {55,  62,  6,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {181, 182, 183, 184} },
    {56,  69,  7,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {205, 206, 207, 208} },
    {57,  68,  7,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {197, 198, 199, 200} },
    {58,  67,  7,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {221, 222, 223, 224} },
    {59,  66,  7,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {213, 214, 215, 216} },
    {60,  73,  7,  7, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {237, 238, 239, 240} },
    {61,  72,  7,  6, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {229, 230, 231, 232} },
    {62,  71,  7,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {253, 254, 255, 256} },
    {63,  70,  7,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {245, 246, 247, 248} },
};


#endif /* INV_SWPS_H */


