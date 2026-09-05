#ifndef INV_SWPS_H
#define INV_SWPS_H

#include "transceiver.h"
#include "io_expander.h"

/* Module settings */
#define SWP_CLS_NAME          "swps"
#define SWP_DEV_PORT          "port"
#define SWP_AUTOCONFIG_ENABLE (1)

/* Module information */
#define SWP_AUTHOR            "Neil <liao.neil@inventec.com>"
#define SWP_DESC              "Inventec port and transceiver driver"
#define SWP_VERSION           "4.2.3"
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
#define PLATFORM_TYPE_CYPRESS_GA2   (152) /* Down -> Up */
#define PLATFORM_TYPE_CYPRESS_BAI   (153) /* Down -> Up */

/* Current running platfrom */
#define PLATFORM_SETTINGS           PLATFORM_TYPE_CYPRESS_GA2

/* Define platform flag and kernel version */
#if (PLATFORM_SETTINGS == PLATFORM_TYPE_CYPRESS_GA2)
  #define SWPS_KERN_VER_BF_3_8   (1)
#elif (PLATFORM_SETTINGS == PLATFORM_TYPE_CYPRESS_BAI)
  #define SWPS_KERN_VER_AF_3_10  (1)
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
struct inv_platform_s platform_map = {PLATFORM_TYPE_CYPRESS_GA2,  "D7054Q28B"  };

/* ==========================================
 *   Cypress Layout configuration (Inventec version [Down->Up])
 * ==========================================
 */
struct inv_ioexp_layout_s cypress_ga2_ioexp_layout[] = {
    /* IOEXP_ID / IOEXP_TYPE / { Chan_ID, Chip_addr, Read_offset, Write_offset, config_offset, data_default, conf_default } */
    {0,  IOEXP_TYPE_CYPRESS_NABC,  { {2, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {2, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {2, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {1,  IOEXP_TYPE_CYPRESS_NABC,  { {3, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {3, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {3, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {2,  IOEXP_TYPE_CYPRESS_NABC,  { {4, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {4, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {4, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {3,  IOEXP_TYPE_CYPRESS_NABC,  { {5, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {5, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {5, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {4,  IOEXP_TYPE_CYPRESS_NABC,  { {6, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {6, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {6, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {5,  IOEXP_TYPE_CYPRESS_NABC,  { {7, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[0] = I/O Expander N A */
                                     {7, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xf0}, {0xff, 0xf0}, },    /* addr[1] = I/O Expander N B */
                                     {7, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {6,  IOEXP_TYPE_CYPRESS_7ABC,  { {8, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0xc0, 0xc0}, },    /* addr[0] = I/O Expander 7 A */
                                     {8, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xc0, 0xc0}, {0xff, 0xc0}, },    /* addr[1] = I/O Expander 7 B */
                                     {8, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 7 C */
    },
};

struct inv_port_layout_s cypress_ga2_port_layout[] = {
    /* Port_ID / Chan_ID / IOEXP_ID / IOEXP_VIRT_OFFSET / TRANSCEIVER_TYPE / BCM_CHIP_TYPE / LANE_ID */
    { 0,  11,  0,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  2} },
    { 1,  10,  0,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  1} },
    { 2,  13,  0,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  4} },
    { 3,  12,  0,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  3} },
    { 4,  15,  0,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  6} },
    { 5,  14,  0,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  5} },
    { 6,  17,  0,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  8} },
    { 7,  16,  0,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  7} },
    { 8,  19,  1,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 10} },
    { 9,  18,  1,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, {  9} },
    {10,  21,  1,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 12} },
    {11,  20,  1,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 11} },
    {12,  23,  1,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 22} },
    {13,  22,  1,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 21} },
    {14,  25,  1,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 24} },
    {15,  24,  1,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 23} },
    {16,  27,  2,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 34} },
    {17,  26,  2,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 33} },
    {18,  29,  2,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 36} },
    {19,  28,  2,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 35} },
    {20,  31,  2,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 38} },
    {21,  30,  2,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 37} },
    {22,  33,  2,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 40} },
    {23,  32,  2,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 39} },
    {24,  35,  3,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 42} },
    {25,  34,  3,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 41} },
    {26,  37,  3,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 44} },
    {27,  36,  3,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 43} },
    {28,  39,  3,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 50} },
    {29,  38,  3,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 49} },
    {30,  41,  3,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 52} },
    {31,  40,  3,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 51} },
    {32,  43,  4,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 54} },
    {33,  42,  4,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 53} },
    {34,  45,  4,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 56} },
    {35,  44,  4,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 55} },
    {36,  47,  4,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 66} },
    {37,  46,  4,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 65} },
    {38,  49,  4,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 68} },
    {39,  48,  4,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 67} },
    {40,  51,  5,  1, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 70} },
    {41,  50,  5,  0, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 69} },
    {42,  53,  5,  3, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 72} },
    {43,  52,  5,  2, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 71} },
    {44,  55,  5,  5, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 82} },
    {45,  54,  5,  4, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 81} },
    {46,  57,  5,  7, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 84} },
    {47,  56,  5,  6, TRANSVR_TYPE_SFP,     BCM_CHIP_TYPE_TOMAHAWK, { 83} },
    {48,  59,  6,  1, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 85, 86, 87, 88} },
    {49,  58,  6,  0, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, { 97, 98, 99,100} },
    {50,  61,  6,  3, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {105,106,107,108} },
    {51,  60,  6,  2, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {101,102,103,104} },
    {52,  63,  6,  5, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {117,118,119,120} },
    {53,  62,  6,  4, TRANSVR_TYPE_QSFP_28, BCM_CHIP_TYPE_TOMAHAWK, {109,110,111,112} },
};


#endif /* SFP_DRIVER_H */






