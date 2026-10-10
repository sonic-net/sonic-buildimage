#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>
#include <wb_bsp_kernel_debug.h>

#define FPGA_INTERNAL_PCA9548        (1)
#define FPGA_EXTERNAL_PCA9548        (2)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static fpga_pca954x_device_t fpga_pca954x_device_data0 = {
    .i2c_bus = 401,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 451,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 451,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 51,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 451,
    .i2c_addr = 0x75,
    .pca9548_base_nr = 59,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {
    .i2c_bus = 402,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 452,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data4 = {
    .i2c_bus = 452,
    .i2c_addr = 0x76,
    .pca9548_base_nr = 67,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {
    .i2c_bus = 403,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 453,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data6 = {
    .i2c_bus = 453,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 75,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {
    .i2c_bus = 404,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 454,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data8 = {
    .i2c_bus = 454,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 83,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {
    .i2c_bus = 405,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 455,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {
    .i2c_bus = 455,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 91,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {
    .i2c_bus = 455,
    .i2c_addr = 0x72,
    .pca9548_base_nr = 99,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {
    .i2c_bus = 406,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 456,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {
    .i2c_bus = 456,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 107,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {
    .i2c_bus = 407,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 457,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {
    .i2c_bus = 457,
    .i2c_addr = 0x72,
    .pca9548_base_nr = 115,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {
    .i2c_bus = 408,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 458,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {
    .i2c_bus = 458,
    .i2c_addr = 0x73,
    .pca9548_base_nr = 123,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {
    .i2c_bus = 409,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 459,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data19 = {
    .i2c_bus = 459,
    .i2c_addr = 0x74,
    .pca9548_base_nr = 131,
    .fpga_9548_flag = FPGA_EXTERNAL_PCA9548,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data20 = {
    .i2c_bus = 410,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 139,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data21 = {
    .i2c_bus = 411,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 233,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data22 = {
    .i2c_bus = 412,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 237,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data23 = {
    .i2c_bus = 413,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 241,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data24 = {
    .i2c_bus = 414,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 245,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data25 = {
    .i2c_bus = 415,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 249,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data26 = {
    .i2c_bus = 416,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 253,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data27 = {
    .i2c_bus = 417,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 257,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data28 = {
    .i2c_bus = 418,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 261,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data29 = {
    .i2c_bus = 419,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 265,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data30 = {
    .i2c_bus = 420,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 269,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data31 = {
    .i2c_bus = 421,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 273,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data32 = {
    .i2c_bus = 422,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 277,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data33 = {
    .i2c_bus = 423,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 281,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data34 = {
    .i2c_bus = 424,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 285,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data35 = {
    .i2c_bus = 425,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 289,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data36 = {
    .i2c_bus = 426,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 293,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data37 = {
    .i2c_bus = 427,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 147,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data38 = {
    .i2c_bus = 428,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 201,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data39 = {
    .i2c_bus = 429,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 205,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data40 = {
    .i2c_bus = 430,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 209,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data41 = {
    .i2c_bus = 431,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 213,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data42 = {
    .i2c_bus = 432,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 217,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data43 = {
    .i2c_bus = 433,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 221,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data44 = {
    .i2c_bus = 434,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 225,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data45 = {
    .i2c_bus = 435,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 229,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data46 = {
    .i2c_bus = 436,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 155,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data47 = {
    .i2c_bus = 437,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 297,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data48 = {
    .i2c_bus = 438,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 301,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data49 = {
    .i2c_bus = 439,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 305,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data50 = {
    .i2c_bus = 440,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 309,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data51 = {
    .i2c_bus = 441,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 313,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data52 = {
    .i2c_bus = 442,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 317,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data53 = {
    .i2c_bus = 443,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 321,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data54 = {
    .i2c_bus = 444,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 325,
    .fpga_9548_flag = FPGA_INTERNAL_PCA9548,
    .fpga_9548_reset_flag = 0,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data0,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data1,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data2,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data3,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data4,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data5,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data6,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data7,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data8,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data9,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data10,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data11,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data12,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data13,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data14,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data15,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data16,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data17,},
    { .type = "wb_fpga_pca9541", .platform_data = &fpga_pca954x_device_data18,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data19,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data20,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data21,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data22,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data23,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data24,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data25,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data26,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data27,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data28,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data29,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data30,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data31,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data32,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data33,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data34,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data35,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data36,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data37,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data38,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data39,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data40,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data41,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data42,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data43,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data44,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data45,},
    { .type = "wb_fpga_pca9548", .platform_data = &fpga_pca954x_device_data46,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data47,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data48,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data49,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data50,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data51,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data52,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data53,},
    { .type = "wb_fpga_pca9544", .platform_data = &fpga_pca954x_device_data54,},
};

static int __init wb_fpga_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_pca954x_device_info); i++) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        fpga_pca954x_device_info[i].addr = fpga_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(fpga_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", fpga_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &fpga_pca954x_device_info[i]);
        if (IS_ERR(client)) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register fpga pca954x device %d at bus %d!\n",
                fpga_pca954x_device_data->i2c_addr, fpga_pca954x_device_data->i2c_bus);
        } else {
            fpga_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_fpga_pca954x_device_exit(void)
{
    int i;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_pca954x_device_info) - 1; i >= 0; i--) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        if (fpga_pca954x_device_data->client) {
            i2c_unregister_device(fpga_pca954x_device_data->client);
            fpga_pca954x_device_data->client = NULL;
        }
    }
}

module_init(wb_fpga_pca954x_device_init);
module_exit(wb_fpga_pca954x_device_exit);
MODULE_DESCRIPTION("FPGA PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
