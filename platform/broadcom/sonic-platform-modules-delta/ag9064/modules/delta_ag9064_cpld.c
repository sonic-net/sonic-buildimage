#include "delta_ag9064_common.h"

#define QSFP_PRESENCE_1    0x03
#define QSFP_PRESENCE_2    0x03
#define QSFP_PRESENCE_3    0x24
#define QSFP_PRESENCE_4    0x24
#define QSFP_PRESENCE_5    0x04
#define QSFP_PRESENCE_6    0x04
#define QSFP_PRESENCE_7    0x25
#define QSFP_PRESENCE_8    0x25

#define QSFP_LP_MODE_1     0x0c
#define QSFP_LP_MODE_2     0x0c
#define QSFP_LP_MODE_3     0x2a
#define QSFP_LP_MODE_4     0x2a
#define QSFP_LP_MODE_5     0x0d
#define QSFP_LP_MODE_6     0x0d
#define QSFP_LP_MODE_7     0x2b
#define QSFP_LP_MODE_8     0x2b

#define QSFP_RESET_1       0x06
#define QSFP_RESET_2       0x06
#define QSFP_RESET_3       0x26
#define QSFP_RESET_4       0x26
#define QSFP_RESET_5       0x07
#define QSFP_RESET_6       0x07
#define QSFP_RESET_7       0x27
#define QSFP_RESET_8       0x27

#define QSFP_RESPONSE_1    0x09
#define QSFP_RESPONSE_2    0x09
#define QSFP_RESPONSE_3    0x28
#define QSFP_RESPONSE_4    0x28
#define QSFP_RESPONSE_5    0x0a
#define QSFP_RESPONSE_6    0x0a
#define QSFP_RESPONSE_7    0x29
#define QSFP_RESPONSE_8    0x29

#define QSFP_INTERRUPT_1   0x0f
#define QSFP_INTERRUPT_2   0x0f
#define QSFP_INTERRUPT_3   0x2c
#define QSFP_INTERRUPT_4   0x2c 
#define QSFP_INTERRUPT_5   0x10
#define QSFP_INTERRUPT_6   0x10
#define QSFP_INTERRUPT_7   0x2d
#define QSFP_INTERRUPT_8   0x2d

unsigned char cpupld_reg_addr;
/*----------------    CPLD  - start   ------------- */
/*    CPLD  -- device   */
static struct platform_device ag9064_cpld = {
    .name               = "delta-ag9064-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9064_cpld_platform_data,
                .release         = device_release
    },
};

static ssize_t get_present(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u64 data = 0;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_GETDATA;

    dni_klock();

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_PRESENCE_1;
    cmd_data[3] = 1;
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data = (u64)(ret & 0xff);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_PRESENCE_2;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 8;

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_PRESENCE_3;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 16;

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_PRESENCE_4;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 24;

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_PRESENCE_5;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 32;

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_PRESENCE_6;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 40;

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_PRESENCE_7;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 48;

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_PRESENCE_8;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 56;

    dni_kunlock();
    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u64 data = 0;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_GETDATA;

    dni_klock();

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_LP_MODE_1;
    cmd_data[3] = 1;
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data = (u64)(ret & 0xff);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_LP_MODE_2;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 8;

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_LP_MODE_3;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 16;

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_LP_MODE_4;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 24;

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_LP_MODE_5;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 32;

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_LP_MODE_6;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 40;

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_LP_MODE_7;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 48;

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_LP_MODE_8;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 56;

    dni_kunlock();
 
    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t get_reset(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u64 data = 0;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_GETDATA;

    dni_klock();

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESET_1;
    cmd_data[3] = 1;
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data = (u64)(ret & 0xff);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESET_2;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 8;

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESET_3;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 16;

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESET_4;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 24;

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESET_5;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 32;

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESET_6;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 40;

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESET_7;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 48;

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESET_8;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 56;

    dni_kunlock();

    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t get_response(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u64 data = 0;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_GETDATA;

    dni_klock();

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESPONSE_1;
    cmd_data[3] = 1;
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data = (u64)(ret & 0xff);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESPONSE_2;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 8;

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESPONSE_3;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 16;

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESPONSE_4;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 24;

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESPONSE_5;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 32;

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESPONSE_6;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 40;

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESPONSE_7;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 48;

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESPONSE_8;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 56;

    dni_kunlock();

    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t get_interrupt(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u64 data = 0;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_GETDATA;

    dni_klock();

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_1;
    cmd_data[3] = 1;
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data = (u64)(ret & 0xff);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_2;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 8;

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_3;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 16;

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_4;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 24;

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_5;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 32;

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_6;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 40;

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_7;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 48;

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_INTERRUPT_8;
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    data |= (u64)(ret & 0xff) << 56;

    dni_kunlock();

    return sprintf(buf, "0x%016llx\n", data);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long long set_data;
    int err;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_SETDATA;

    dni_klock();

    err = kstrtoull(buf, 16, &set_data);
    if (err){
        dni_kunlock();
        return err;
    } 

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_LP_MODE_1;
    cmd_data[3] = (set_data & 0xff);
    cmd_data_len = sizeof(cmd_data);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_LP_MODE_2;
    cmd_data[3] = ((set_data >> 8 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_LP_MODE_3;
    cmd_data[3] = ((set_data >> 16 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_LP_MODE_4;
    cmd_data[3] = ((set_data >> 24 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_LP_MODE_5;
    cmd_data[3] = ((set_data >> 32 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_LP_MODE_6;
    cmd_data[3] = ((set_data >> 40 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_LP_MODE_7;
    cmd_data[3] = ((set_data >> 48 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_LP_MODE_8;
    cmd_data[3] = ((set_data >> 56 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    dni_kunlock();
   
    return count;
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long long set_data;
    int err;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_SETDATA;

    dni_klock();

    err = kstrtoull(buf, 16, &set_data);
    if (err){
        dni_kunlock();
        return err;
    }

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESET_1;
    cmd_data[3] = (set_data & 0xff);
    cmd_data_len = sizeof(cmd_data);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESET_2;
    cmd_data[3] = ((set_data >> 8 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESET_3;
    cmd_data[3] = ((set_data >> 16 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESET_4;
    cmd_data[3] = ((set_data >> 24 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESET_5;
    cmd_data[3] = ((set_data >> 32 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESET_6;
    cmd_data[3] = ((set_data >> 40 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESET_7;
    cmd_data[3] = ((set_data >> 48 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESET_8;
    cmd_data[3] = ((set_data >> 56 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    dni_kunlock();

    return count;
}

static ssize_t set_response(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long long set_data;
    int err;
    uint8_t cmd_data[4]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_SETDATA;

    dni_klock();

    err = kstrtoull(buf, 16, &set_data);
    if (err){
        dni_kunlock();
        return err;
    }

    /*QSFP1~8*/
    cmd_data[0] = BMC_BUS_5;
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESPONSE_1;
    cmd_data[3] = (set_data & 0xff);
    cmd_data_len = sizeof(cmd_data);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP9~16*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESPONSE_2;
    cmd_data[3] = ((set_data >> 8 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP17~24*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESPONSE_3;
    cmd_data[3] = ((set_data >> 16 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP25~32*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESPONSE_4;
    cmd_data[3] = ((set_data >> 24 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP33~40*/
    cmd_data[1] = SWPLD1_ADDR;
    cmd_data[2] = QSFP_RESPONSE_5;
    cmd_data[3] = ((set_data >> 32 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP41~48*/
    cmd_data[1] = SWPLD2_ADDR;
    cmd_data[2] = QSFP_RESPONSE_6;
    cmd_data[3] = ((set_data >> 40 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP49~56*/
    cmd_data[1] = SWPLD4_ADDR;
    cmd_data[2] = QSFP_RESPONSE_7;
    cmd_data[3] = ((set_data >> 48 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    /*QSFP57~64*/
    cmd_data[1] = SWPLD3_ADDR;
    cmd_data[2] = QSFP_RESPONSE_8;
    cmd_data[3] = ((set_data >> 56 ) & 0xff);
    dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);

    dni_kunlock();  
 
    return count;
}

static ssize_t psu_scan(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    int ret;
    uint8_t cmd_data[2]={0};
    uint8_t set_cmd;
    int cmd_data_len;
    set_cmd = CMD_DEVICE_SCAN;

    dni_klock();
    cmd_data[0] = BMC_BUS_1;
    switch (attr->index)
    {
        case PSU1_SCAN:
            cmd_data[1] = PSU1_EEPROM_ADDR;
            break;
        case PSU2_SCAN:
            cmd_data[1] = PSU2_EEPROM_ADDR;
            break;
    }
    cmd_data_len = sizeof(cmd_data);
    ret = dni_bmc_cmd(set_cmd, cmd_data, cmd_data_len);
    dni_kunlock();
    return sprintf(buf, "0x%x\n", ret);
}

static ssize_t get_cpld_reg(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    int mask;
    int value;
    char note[200];
    unsigned char reg;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct cpld_platform_data *pdata = dev->platform_data;

    dni_klock();
    
    switch (attr->index) {
        case CPLD_REG_ADDR:
            dni_kunlock();
            return sprintf(buf, "0x%02x\n", cpupld_reg_addr);
        case CPLD_REG_VALUE:
            ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, cpupld_reg_addr);
            dni_kunlock();
            return sprintf(buf, "0x%02x\n", ret);
        case CPLD_VER ... OP_MODULE_INT:
            reg   = attribute_data[attr->index].reg;
            mask  = attribute_data[attr->index].mask;
            value = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            sprintf(note, "\n%s\n",attribute_data[attr->index].note);
            value = (value & mask);
            break;
        default:
            dni_kunlock();
            return sprintf(buf, "%d not found", attr->index);
    } 
    
    switch (mask) {
        case 0xFF:
            dni_kunlock();
            return sprintf(buf, "0x%02x%s", value, note);
        case 0x0F:
            dni_kunlock();
            return sprintf(buf, "0x%01x%s", value, note);
        case 0xF0:
            value = value >> 4;
            dni_kunlock();
            return sprintf(buf, "0x%01x%s", value, note);
        default :
            value = value >> dni_log2(mask);
            dni_kunlock();
            return sprintf(buf, "%d%s", value, note);
    }
    dni_kunlock();
    return sprintf(buf, "%d not found", attr->index);
}

static ssize_t set_cpld_reg(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    int err;
    int value;
    int set_data;
    unsigned long set_data_ul;
    unsigned char reg;
    unsigned char mask;  
    unsigned char mask_out;      
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct cpld_platform_data *pdata = dev->platform_data;

    dni_klock();

    err = kstrtoul(buf, 0, &set_data_ul);
    if (err){
        dni_kunlock();
        return err;
    }
    
    set_data = (int)set_data_ul;
    if (set_data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        dni_kunlock();
        return count;
    }

    switch (attr->index) {
        case CPLD_REG_ADDR:
            cpupld_reg_addr = set_data;
            dni_kunlock();
            return count;
        case CPLD_REG_VALUE:
            i2c_smbus_write_byte_data(pdata[system_cpld].client, cpupld_reg_addr, set_data);
            dni_kunlock();
            return count;
        case CPLD_VER ... OP_MODULE_INT:
            reg   = attribute_data[attr->index].reg;
            mask  = attribute_data[attr->index].mask;
            value = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            mask_out = value & ~(mask);
            break;
        default:
            dni_kunlock();
            return sprintf(buf, "%d not found", attr->index); 
    }

    switch (mask) {
        case 0xFF:
            set_data = mask_out | (set_data & mask);
            break;
        case 0x0F:
            set_data = mask_out | (set_data & mask);
            break;
        case 0xF0:
            set_data = set_data << 4;
            set_data = mask_out | (set_data & mask);
            break; 
        default :
            set_data = mask_out | (set_data << dni_log2(mask) );        
    }   

    i2c_smbus_write_byte_data(pdata[system_cpld].client, reg, set_data);
    dni_kunlock();
    return count;
}

static DEVICE_ATTR(qsfp_present,   S_IRUGO,           get_present,   NULL);
static DEVICE_ATTR(qsfp_lpmode,    S_IWUSR | S_IRUGO, get_lpmode,    set_lpmode);
static DEVICE_ATTR(qsfp_reset,     S_IWUSR | S_IRUGO, get_reset,     set_reset);
static DEVICE_ATTR(qsfp_modsel,    S_IWUSR | S_IRUGO, get_response,  set_response);
static DEVICE_ATTR(qsfp_interrupt, S_IRUGO,           get_interrupt, NULL);

static SENSOR_DEVICE_ATTR(cpld_reg_addr,     S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, CPLD_REG_ADDR);
static SENSOR_DEVICE_ATTR(cpld_reg_value,    S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, CPLD_REG_VALUE);
static SENSOR_DEVICE_ATTR(cpld_ver,          S_IRUGO,           get_cpld_reg, NULL,         CPLD_VER);
static SENSOR_DEVICE_ATTR(cpu_board_ver,     S_IRUGO,           get_cpld_reg, NULL,         CPU_BOARD_VER);
static SENSOR_DEVICE_ATTR(cpu_id,            S_IRUGO,           get_cpld_reg, NULL,         CPU_ID);
static SENSOR_DEVICE_ATTR(mb_id,             S_IRUGO,           get_cpld_reg, NULL,         MB_ID);
static SENSOR_DEVICE_ATTR(mb_ver,            S_IRUGO,           get_cpld_reg, NULL,         MB_VER);
static SENSOR_DEVICE_ATTR(cpu0_pwr_ok,       S_IRUGO,           get_cpld_reg, NULL,         CPU0_PWR_OK);
static SENSOR_DEVICE_ATTR(pwr_rail_over_temp,    S_IRUGO,       get_cpld_reg, NULL,         PWR_RAIL_OVER_TEMP);
static SENSOR_DEVICE_ATTR(cpu_disomic_over_temp, S_IRUGO,       get_cpld_reg, NULL,         CPU_DISOMIC_OVER_TEMP);
static SENSOR_DEVICE_ATTR(ddr_over_temp,     S_IRUGO,           get_cpld_reg, NULL,         DDR_OVER_TEMP);
static SENSOR_DEVICE_ATTR(cpld_pwr_on_rst,   S_IRUGO,           get_cpld_reg, NULL,         CPLD_PWR_ON_RST);
static SENSOR_DEVICE_ATTR(cpld_hard_rst,     S_IRUGO,           get_cpld_reg, NULL,         CPLD_HARD_RST);
static SENSOR_DEVICE_ATTR(cpld_rst,          S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, CPLD_RST);
static SENSOR_DEVICE_ATTR(mb_rst,            S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, MB_RST);
static SENSOR_DEVICE_ATTR(psu_fan_int,       S_IRUGO,           get_cpld_reg, NULL,         PSU_FAN_INT);
static SENSOR_DEVICE_ATTR(op_module_int,     S_IRUGO,           get_cpld_reg, NULL,         OP_MODULE_INT);
static SENSOR_DEVICE_ATTR(psu1_scan,         S_IRUGO,           psu_scan,     NULL,         PSU1_SCAN);
static SENSOR_DEVICE_ATTR(psu2_scan,         S_IRUGO,           psu_scan,     NULL,         PSU2_SCAN);

static struct attribute *ag9064_cpld_attrs[] = {
    &dev_attr_qsfp_present.attr,
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    &dev_attr_qsfp_modsel.attr,
    &dev_attr_qsfp_interrupt.attr,
    &sensor_dev_attr_cpld_reg_value.dev_attr.attr,
    &sensor_dev_attr_cpld_reg_addr.dev_attr.attr,    
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_cpu_board_ver.dev_attr.attr,
    &sensor_dev_attr_cpu_id.dev_attr.attr,
    &sensor_dev_attr_mb_id.dev_attr.attr,
    &sensor_dev_attr_mb_ver.dev_attr.attr,
    &sensor_dev_attr_cpu0_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_pwr_rail_over_temp.dev_attr.attr,
    &sensor_dev_attr_cpu_disomic_over_temp.dev_attr.attr,
    &sensor_dev_attr_ddr_over_temp.dev_attr.attr,
    &sensor_dev_attr_cpld_pwr_on_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_hard_rst.dev_attr.attr,
    &sensor_dev_attr_cpld_rst.dev_attr.attr,
    &sensor_dev_attr_mb_rst.dev_attr.attr,
    &sensor_dev_attr_psu_fan_int.dev_attr.attr,
    &sensor_dev_attr_op_module_int.dev_attr.attr,
    &sensor_dev_attr_psu1_scan.dev_attr.attr,
    &sensor_dev_attr_psu2_scan.dev_attr.attr,
    NULL,
};

static struct attribute_group ag9064_cpld_attr_grp = {
    .attrs = ag9064_cpld_attrs,
};

static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS7);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n",BUS7);
        return -ENODEV;
    }

    pdata[system_cpld].client = i2c_new_dummy(parent, pdata[system_cpld].reg_addr);
    if (!pdata[system_cpld].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[system_cpld].reg_addr);
        goto error;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &ag9064_cpld_attr_grp);
   if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
   }

    return 0;

error:
    i2c_unregister_device(pdata[system_cpld].client);
    i2c_put_adapter(parent);

    return -ENODEV;
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &ag9064_cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[system_cpld].client) {
            if (!parent) {
                parent = (pdata[system_cpld].client)->adapter;
            }
        i2c_unregister_device(pdata[system_cpld].client);
        }
    }
    i2c_put_adapter(parent);

    return 0;
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9064-cpld",
    },
};

/*----------------    CPLD  - end   ------------- */

/*----------------   module initialization     ------------- */
static int __init delta_ag9064_cpupld_init(void)
{
    int ret;
    printk(KERN_WARNING "ag9064_platform_cpupld module initialization\n");

    // set the CPUPLD prob and remove
    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpupld driver\n");
        goto error_cpupld_driver;
    }

    // register the CPUPLD
    ret = platform_device_register(&ag9064_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpupld device\n");
        goto error_ag9064_cpupld;
    }
    return 0;

error_ag9064_cpupld:
    platform_driver_unregister(&cpld_driver);
error_cpupld_driver:
    return ret;
}

static void __exit delta_ag9064_cpupld_exit(void)
{
    platform_device_unregister(&ag9064_cpld);
    platform_driver_unregister(&cpld_driver);  
}
module_init(delta_ag9064_cpupld_init);
module_exit(delta_ag9064_cpupld_exit);

MODULE_DESCRIPTION("DNI ag9064 CPLD Platform Support");
MODULE_AUTHOR("Stanley Chi <stanley.chi@deltaww.com>");
MODULE_LICENSE("GPL"); 
