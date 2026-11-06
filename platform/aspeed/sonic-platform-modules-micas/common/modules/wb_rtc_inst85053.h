#ifndef __WB_RTC_INST85053_H__
#define __WB_RTC_INST85053_H__

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/errno.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

/* second regiseter (00h) */
#define INST85053A_REG_SEC              (0x00)
/* second alarm register (01h) */
#define INST85053A_REG_SEC_ALARM        (0x01)
/* minutes register (02h) */
#define INST85053A_REG_MIN              (0x02)
/* minutes alarm regiseter (03h) */
#define INST85053A_REG_MIN_ALARM        (0x03)
#define INST85053A_REG_HOUR             (0x04)
#define INST85053A_REG_HOUR_ALARM       (0x05)
#define INST85053A_REG_WEEKDAY          (0x06)
#define INST85053A_REG_DAY              (0x07)
#define INST85053A_REG_MONTH            (0x08)
#define INST85053A_REG_YEAR             (0x09)
/* control register (0Ah) */
#define INST85053A_REG_CTRL1            (0x0A)
#define CTRL1_STOP                      BIT(7)
#define CTRL1_12_24                     BIT(5)
#define CTRL1_DSM                       BIT(4)
#define CTRL1_AIE                       BIT(3)
#define CTRL1_OFIE                      BIT(2)
#define CTRL1_CIE                       BIT(1)
#define CTRL1_TWO                       BIT(0)
/* status regiseter (0Bh) */
#define INST85053A_REG_STATUS           (0x0B)
#define STATUS_AF                       BIT(7)
#define STATUS_OF                       BIT(6)
#define STATUS_RTCF                     BIT(5)
#define STATUS_CIF                      BIT(4)
/* status regiseter (0Bh) */
#define INST85053A_REG_VERSION          (0x0F)
#define INST85053A_REG_VENDOR           (0x10)
#define INST85053A_REG_MODEL            (0x11)
#define INST85053A_REG_OFFSET           (0x12)

#define INST85053A_REG_OSC              (0x13)
#define OSC_OFFM                        (0x06)
#define INST85053A_REG_ACSCNF           (0x14)

#define INST85053A_FAST_MODE            (0x01)
#define INST85053A_NORMAL_MODE          (0x00)

#define INST85053A_MODE_STEP0           (2170)
#define INST85053A_MODE_STEP1           (2934)

#define INST85053A_TIME_REG_NUM         (10)

#define INST85053A_SRAM_BASE            (0x00)
#define INST85053A_SRAM_SIZE            (0x80)
#define INST85053A_ENABLE               (1)
#define INST85053A_DISABLE              (0)

#define INST85053A_SRAM_ADDR            (0x57)

#define INST85053A_VERSION_LEN          (0x0F)

/*  controller (0Bh) */
#define CTRL2_AF                        BIT(5)

/* ----------------- Device data structure ----------------- */
struct inst85053a {
    unsigned char sram_cache[INST85053A_SRAM_SIZE];
    struct i2c_client *client;
    struct rtc_device *rtc;
    struct mutex sram_lock;
    struct regmap *regmap;
    unsigned char irq_enable;
    int irq;
};

struct inst85053a_config {
    struct regmap_config regmap;
    unsigned char alarm_support;
};

struct inst85053a_sram_config {
    struct regmap_config regmap;
};

#endif /* __WB_RTC_INST85053_H__ */
