#ifndef __CPLD_I2C_DEBUG_H__
#define __CPLD_I2C_DEBUG_H__

#define DEBUG_FILE                     "/.cpld_i2c_debug_flag"

typedef enum {
    CPLD_I2C_DBG_ERR,
    CPLD_I2C_DBG_WARN,
    CPLD_I2C_DBG_VBOSE,
    CPLD_I2C_DBG_ISPVME,
    CPLD_I2C_DBG_KERNEL,
    CPLD_I2C_DBG_CNT,
} DFD_DEBUG_TYPE_E;

extern int g_cpld_i2c_debug;

#define CPLD_I2C_DEBUG_CHECK(type)      (g_cpld_i2c_debug & (1U << (type)))

#define CPLD_I2C_ERROR(fmt, args...) do {                     \
    if (CPLD_I2C_DEBUG_CHECK(CPLD_I2C_DBG_ERR)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "FIRMWARE", "err", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define CPLD_I2C_WARN(fmt, args...) do {                     \
    if (CPLD_I2C_DEBUG_CHECK(CPLD_I2C_DBG_WARN)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "FIRMWARE", "warn", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define CPLD_I2C_VERBOS(fmt, args...) do {                     \
    if (CPLD_I2C_DEBUG_CHECK(CPLD_I2C_DBG_VBOSE)) {               \
        printf("[%s-%s]:<File:%s, Func:%s, Line:%d>\n" fmt, "FIRMWARE", "vbose", \
            __FILE__, __FUNCTION__, __LINE__, ##args);  \
    }                                                   \
} while (0)

#define CPLD_I2C_ISPVME(fmt, args...) do {                     \
    if (CPLD_I2C_DEBUG_CHECK(CPLD_I2C_DBG_ISPVME)) {               \
        printf("" fmt, ##args);  \
    }                                                   \
} while (0)

void cpld_i2c_debug_init(void);

#endif /* End of __CPLD_I2C_DEBUG_H__ */
