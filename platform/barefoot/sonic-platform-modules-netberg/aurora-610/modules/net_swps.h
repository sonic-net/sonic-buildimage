#ifndef NET_SWPS_H
#define NET_SWPS_H

#include "transceiver.h"
#include "io_expander.h"
#include "net_mux.h"

/* Module settings */
#define SWP_CLS_NAME          "swps"
#define SWP_DEV_PORT          "port"
#define SWP_DEV_MODCTL        "module"
#define SWP_RESET_PWD         "netberg"
#define SWP_POLLING_PERIOD    (300)  /* msec */
#define SWP_POLLING_ENABLE    (1)
#define SWP_AUTOCONFIG_ENABLE (1)

/* Module information */
#define SWP_AUTHOR            "Netberg <support@netbergtw.com>"
#define SWP_DESC              "Netberg port and transceiver driver"
#define SWP_VERSION           "4.3.4"
#define SWP_LICENSE           "GPL"

/* Module status define */
#define SWP_STATE_NORMAL      (0)
#define SWP_STATE_I2C_DIE     (-91)


#define PLATFORM_TYPE_AUTO                (100)
#define PLATFORM_TYPE_AURORA_610_GA       (202)
#define PLATFORM_TYPE_AURORA_610          (203)
/* Current running platfrom */
#define PLATFORM_SETTINGS                 PLATFORM_TYPE_AURORA_610

/* Define platform flag and kernel version */
#if (PLATFORM_SETTINGS == PLATFORM_TYPE_AURORA_610_GA)
  #define SWPS_AURORA_610_GA             (1)
  #define SWPS_KERN_VER_AF_3_10     (1)
#elif (PLATFORM_SETTINGS == PLATFORM_TYPE_AURORA_610)
  #define SWPS_AURORA_610     (1)
  #define SWPS_KERN_VER_AF_3_10     (1)
#endif

struct net_platform_s {
    int  id;
    char name[64];
};

struct net_ioexp_layout_s {
    int ioexp_id;
    int ioexp_type;
    struct ioexp_addr_s addr[4];
};

struct net_port_layout_s {
    int port_id;
    int chan_id;
    int ioexp_id;
    int ioexp_offset;
    int transvr_type;
    int chipset_type;
    int lane_id[8];
};


/* ==========================================
 *   Netberg Platform Settings
 * ==========================================
 */
struct net_platform_s platform_map[] = {
    {PLATFORM_TYPE_AUTO,                 "Auto-Detect"         },
    {PLATFORM_TYPE_AURORA_610_GA,        "Aurora_610_GA"       },
    {PLATFORM_TYPE_AURORA_610,           "Aurora_610"          },
};


/* ==========================================
 *   Aurora 610 GA Layout configuration
 * ==========================================
 */
#ifdef SWPS_AURORA_610_GA
unsigned aurora_610_ga_gpio_rest_mux = MUX_RST_GPIO_69_PCA9548;

struct net_ioexp_layout_s aurora_610_ga_ioexp_layout[] = {
    /* IOEXP_ID / IOEXP_TYPE / { Chan_ID, Chip_addr, Read_offset, Write_offset, config_offset, data_default, conf_default } */
    {0,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {2, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {2, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {2, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {1,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {3, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {3, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {3, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {2,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {4, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {4, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {4, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {3,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {5, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {5, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {5, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {4,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {6, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {6, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {6, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {5,  IOEXP_TYPE_AURORA_610_GA_NABC,  { {7, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                      {7, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                      {7, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {6,  IOEXP_TYPE_AURORA_610_GA_7ABC,  { {8, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xc0, 0xff}, {0xc0, 0xc0}, },    /* addr[0] = I/O Expander 7 A */
                                      {8, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0xc0, 0xff}, },    /* addr[1] = I/O Expander 7 B */
                                      {8, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xc0, 0xff}, {0xff, 0xff}, }, }, /* addr[2] = I/O Expander 7 C */
    },
};



struct net_port_layout_s aurora_610_ga_port_layout[] = {
    /* Port_ID / Chan_ID / IOEXP_ID / IOEXP_VIRT_OFFSET / TRANSCEIVER_TYPE / BFT_CHIP_TYPE / LANE_ID */
    { 0,  10,  0,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  1} },
    { 1,  11,  0,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  2} },
    { 2,  12,  0,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  3} },
    { 3,  13,  0,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  4} },
    { 4,  14,  0,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  5} },
    { 5,  15,  0,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  6} },
    { 6,  16,  0,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  7} },
    { 7,  17,  0,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  8} },
    { 8,  18,  1,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {  9} },
    { 9,  19,  1,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 10} },
    {10,  20,  1,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 11} },
    {11,  21,  1,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 12} },
    {12,  22,  1,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 21} },
    {13,  23,  1,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 22} },
    {14,  24,  1,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 23} },
    {15,  25,  1,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 24} },
    {16,  26,  2,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 33} },
    {17,  27,  2,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 34} },
    {18,  28,  2,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 35} },
    {19,  29,  2,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 36} },
    {20,  30,  2,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 37} },
    {21,  31,  2,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 38} },
    {22,  32,  2,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 39} },
    {23,  33,  2,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 40} },
    {24,  34,  3,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 41} },
    {25,  35,  3,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 42} },
    {26,  36,  3,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 43} },
    {27,  37,  3,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 44} },
    {28,  38,  3,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 49} },
    {29,  39,  3,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 50} },
    {30,  40,  3,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 51} },
    {31,  41,  3,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 52} },
    {32,  42,  4,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 53} },
    {33,  43,  4,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 54} },
    {34,  44,  4,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 55} },
    {35,  45,  4,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 56} },
    {36,  46,  4,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 65} },
    {37,  47,  4,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 66} },
    {38,  48,  4,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 67} },
    {39,  49,  4,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 68} },
    {40,  50,  5,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 69} },
    {41,  51,  5,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 70} },
    {42,  52,  5,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 71} },
    {43,  53,  5,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 72} },
    {44,  54,  5,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 81} },
    {45,  55,  5,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 82} },
    {46,  56,  5,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 83} },
    {47,  57,  5,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, { 84} },
    {48,  58,  6,  0, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, { 97, 98, 99,100} },
    {49,  59,  6,  1, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, { 85, 86, 87, 88} },
    {50,  60,  6,  2, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {101,102,103,104} },
    {51,  61,  6,  3, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {105,106,107,108} },
    {52,  62,  6,  4, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {109,110,111,112} },
    {53,  63,  6,  5, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {109,110,111,112} },
};
#endif


/* ==========================================
 *   Aurora 610 Layout configuration
 * ==========================================
 */
#ifdef SWPS_AURORA_610
unsigned aurora_610_gpio_rest_mux = MUX_RST_GPIO_69_PCA9548;

struct net_ioexp_layout_s aurora_610_ioexp_layout[] = {
    /* IOEXP_ID / IOEXP_TYPE / { Chan_ID, Chip_addr, Read_offset, Write_offset, config_offset, data_default, conf_default } */
    {0,  IOEXP_TYPE_AURORA_610_NABC,      { {2, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[0] = I/O Expander N A */
                                            {2, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[1] = I/O Expander N B */
                                            {2, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {1,  IOEXP_TYPE_AURORA_610_1ABC,      { {3, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[0] = I/O Expander N A */
                                            {3, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[1] = I/O Expander N B */
                                            {3, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {2,  IOEXP_TYPE_AURORA_610_NABC,      { {4, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[0] = I/O Expander N A */
                                            {4, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[1] = I/O Expander N B */
                                            {4, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {3,  IOEXP_TYPE_AURORA_610_3ABC,      { {5, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xf3, 0xf3}, {0xf3, 0xf3}, },    /* addr[0] = I/O Expander N A */
                                            {5, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[1] = I/O Expander N B */
                                            {5, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {4,  IOEXP_TYPE_AURORA_610_NABC,      { {6, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[0] = I/O Expander N A */
                                            {6, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[1] = I/O Expander N B */
                                            {6, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {5,  IOEXP_TYPE_AURORA_610_NABC,      { {7, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[0] = I/O Expander N A */
                                            {7, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xdd, 0xdd}, {0xdd, 0xdd}, },    /* addr[1] = I/O Expander N B */
                                            {7, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xff, 0xff}, {0x00, 0x00}, }, }, /* addr[2] = I/O Expander N C */
    },
    {6,  IOEXP_TYPE_AURORA_610_7ABC,      { {8, 0x20, {0, 1}, {2, 3}, {6, 7}, {0xd6, 0xda}, {0x18, 0xe3}, },    /* addr[0] = I/O Expander 7 A */
                                            {8, 0x21, {0, 1}, {2, 3}, {6, 7}, {0xd6, 0xda}, {0x18, 0xe3}, },    /* addr[1] = I/O Expander 7 B */
                                            {8, 0x22, {0, 1}, {2, 3}, {6, 7}, {0xd6, 0xff}, {0x18, 0xff}, }, }, /* addr[2] = I/O Expander 7 C */
    },
};



struct net_port_layout_s aurora_610_port_layout[] = {
    /* Port_ID / Chan_ID / IOEXP_ID / IOEXP_VIRT_OFFSET / TRANSCEIVER_TYPE / BFT_CHIP_TYPE / LANE_ID */
    { 0,  10,  0,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 1,  11,  0,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 2,  12,  0,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 3,  13,  0,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 4,  14,  0,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 5,  15,  0,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 6,  16,  0,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 7,  17,  0,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 8,  18,  1,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    { 9,  19,  1,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {10,  20,  1,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {11,  21,  1,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {12,  22,  1,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {13,  23,  1,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {14,  24,  1,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {15,  25,  1,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {16,  26,  2,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {17,  27,  2,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {18,  28,  2,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {19,  29,  2,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {20,  30,  2,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {21,  31,  2,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {22,  32,  2,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {23,  33,  2,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {24,  34,  3,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {25,  35,  3,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {26,  36,  3,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {27,  37,  3,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {28,  38,  3,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {29,  39,  3,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {30,  40,  3,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {31,  41,  3,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {32,  42,  4,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {33,  43,  4,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {34,  44,  4,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {35,  45,  4,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {36,  46,  4,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {37,  47,  4,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {38,  48,  4,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {39,  49,  4,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {40,  50,  5,  0, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {41,  51,  5,  1, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {42,  52,  5,  2, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {43,  53,  5,  3, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {44,  54,  5,  4, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {45,  55,  5,  5, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {46,  56,  5,  6, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {47,  57,  5,  7, TRANSVR_TYPE_SFP,     CHIP_TYPE_LAVENDER, {-99} },
    {48,  59,  6,  1, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {49,  58,  6,  0, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {50,  61,  6,  3, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {51,  60,  6,  2, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {52,  63,  6,  5, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {53,  62,  6,  4, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {54,  65,  6,  7, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
    {55,  64,  6,  6, TRANSVR_TYPE_QSFP_28, CHIP_TYPE_LAVENDER, {-99,-99,-99,-99} },
};
#endif

#endif /* NET_SWPS_H */









