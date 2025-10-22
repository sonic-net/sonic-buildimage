/*
* Copyright (c) 2019  <sonic@h3c.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#define CPLD_OFFSET (0x2000)
#define CPLD_BASE_ADDR (0xdffe0000+CPLD_OFFSET)
/*A2 A1 A0 : 1 1 1*/
#define I2C_9548_DEVADDR 0x77
/*A2 A1 A0 : 0 1 1*/
#define I2C_9545_DEVADDR 0x73
#define NONE_DEVADDR 0xff
#define NONE_CHANNEL 0xff

/* cat /proc/devices 查看空项设备号 */
static int major = 240;
volatile void *g_u64Cpld_addr = 0;
unsigned char g_ucBoardType = 0;
static struct class *class;
struct i2c_adapter *smbus = NULL;
/*
    定义DEBUG_ENABLE即可打开驱动的debug打印
*/
#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
#define cpld_debug(fmt,args...) printk(KERN_INFO"h3c : "fmt, ##args)
#else
#define cpld_debug(fmt,args...)
#endif
typedef struct
{
    u16 size;
    u16 offset;
} CPLDhdr;
typedef struct
{
    CPLDhdr header;
    u8 data[1024]; /*from 0x2000 to 0x23ff*/
} CPLDrw;
enum CMD
{
    CPLD_READ = 0,
    CPLD_WRITE,
    EEPROM_READ = 0x11,
    EEPROM_WRITE,
    I2C_RESET,
    OPTIC_READ,
    OPTIC_WRITE,
    OPTIC_READ_sfp_51,
    OPTIC_WRITE_sfp_51,
    SENSOR_READ,
    SELECT_OPTIC_I2C,
};
enum SENSOR_ID
{
    LM75 = 0,         /*CPU扣板下方，靠近管理板连接器位置*/
    Max6696_0,        /*MAC芯片正面PCB靠近Cage侧*/
    Max6696_1,        /*MAC芯片内部温度*/
    Max6696_2,        /*MAC芯片背面PCB温度*/
};
typedef struct
{
    u8 ucChannelId;
    /*9548：64H只有光模块选通才使用*/
    u8 uc9548DevAddr;
    u8 uc9548ChannelId;

    /*9545：64H只有光模块选通才使用*/
    u8 uc9545DevAddr;
    u8 uc9545ChannelId;
    u8 ucDevAddr;
} I2C_Device;
enum i2c_device_enum
{
    /*光模块开始*/
    enumOpt1,
    enumOpt2,
    enumOpt3,
    enumOpt4,
    enumOpt5,
    enumOpt6,
    enumOpt7,
    enumOpt8,
    enumOpt9,
    enumOpt10,
    enumOpt11,
    enumOpt12,
    enumOpt13,
    enumOpt14,
    enumOpt15,
    enumOpt16,
    enumOpt17,
    enumOpt18,
    enumOpt19,
    enumOpt20,
    enumOpt21,
    enumOpt22,
    enumOpt23,
    enumOpt24,
    enumOpt25,
    enumOpt26,
    enumOpt27,
    enumOpt28,
    enumOpt29,
    enumOpt30,
    enumOpt31,
    enumOpt32,
    enumOpt33,
    enumOpt34,
    enumOpt35,
    enumOpt36,
    enumOpt37,
    enumOpt38,
    enumOpt39,
    enumOpt40,
    enumOpt41,
    enumOpt42,
    enumOpt43,
    enumOpt44,
    enumOpt45,
    enumOpt46,
    enumOpt47,
    enumOpt48,
    enumOpt49,
    enumOpt50,
    enumOpt51,
    enumOpt52,
    enumOpt53,
    enumOpt54,
    enumOpt55,
    enumOpt56,
    enumOpt57,
    enumOpt58,
    enumOpt59,
    enumOpt60,
    enumOpt61,
    enumOpt62,
    enumOpt63,
    enumOpt64,
    /*除光模块外的其他器件起始*/
    enumEEPROM,
    enumLM75,
    enum6696,
};

I2C_Device i2c_device_table[] =
{
    {0x01, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x00,            0xff},/*    1    */
    {0x01, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x01,            0xff},/*    2    */
    {0x01, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x00,            0xff},/*    3    */
    {0x01, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x01,            0xff},/*    4    */
    {0x01, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x00,            0xff},/*    5    */
    {0x01, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x01,            0xff},/*    6    */
    {0x01, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x00,            0xff},/*    7    */
    {0x01, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x01,            0xff},/*    8    */
    {0x01, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x00,            0xff},/*    9    */
    {0x01, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x01,            0xff},/*    10    */
    {0x01, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x00,            0xff},/*    11    */
    {0x01, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x01,            0xff},/*    12    */
    {0x01, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x00,            0xff},/*    13    */
    {0x01, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x01,            0xff},/*    14    */
    {0x01, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x00,            0xff},/*    15    */
    {0x01, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x01,            0xff},/*    16    */
    {0x05, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x00,            0xff},/*    17    */
    {0x05, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x01,            0xff},/*    18    */
    {0x05, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x00,            0xff},/*    19    */
    {0x05, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x01,            0xff},/*    20    */
    {0x05, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x00,            0xff},/*    21    */
    {0x05, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x01,            0xff},/*    22    */
    {0x05, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x00,            0xff},/*    23    */
    {0x05, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x01,            0xff},/*    24    */
    {0x05, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x00,            0xff},/*    25    */
    {0x05, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x01,            0xff},/*    26    */
    {0x05, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x00,            0xff},/*    27    */
    {0x05, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x01,            0xff},/*    28    */
    {0x05, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x00,            0xff},/*    29    */
    {0x05, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x01,            0xff},/*    30    */
    {0x05, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x00,            0xff},/*    31    */
    {0x05, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x01,            0xff},/*    32    */

    {0x01, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x02,            0xff},/*    33    */
    {0x01, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x03,            0xff},/*    34    */
    {0x01, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x02,            0xff},/*    35    */
    {0x01, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x03,            0xff},/*    36    */
    {0x01, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x02,            0xff},/*    37    */
    {0x01, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x03,            0xff},/*    38    */
    {0x01, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x02,            0xff},/*    39    */
    {0x01, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x03,            0xff},/*    40    */
    {0x01, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x02,            0xff},/*    41    */
    {0x01, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x03,            0xff},/*    42    */
    {0x01, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x02,            0xff},/*    43    */
    {0x01, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x03,            0xff},/*    44    */
    {0x01, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x02,            0xff},/*    45    */
    {0x01, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x03,            0xff},/*    46    */
    {0x01, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x02,            0xff},/*    47    */
    {0x01, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x03,            0xff},/*    48    */
    {0x05, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x02,            0xff},/*    49    */
    {0x05, I2C_9548_DEVADDR, 0x00,            I2C_9545_DEVADDR, 0x03,            0xff},/*    50    */
    {0x05, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x02,            0xff},/*    51    */
    {0x05, I2C_9548_DEVADDR, 0x01,            I2C_9545_DEVADDR, 0x03,            0xff},/*    52    */
    {0x05, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x02,            0xff},/*    53    */
    {0x05, I2C_9548_DEVADDR, 0x02,            I2C_9545_DEVADDR, 0x03,            0xff},/*    54    */
    {0x05, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x02,            0xff},/*    55    */
    {0x05, I2C_9548_DEVADDR, 0x03,            I2C_9545_DEVADDR, 0x03,            0xff},/*    56    */
    {0x05, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x02,            0xff},/*    57    */
    {0x05, I2C_9548_DEVADDR, 0x04,            I2C_9545_DEVADDR, 0x03,            0xff},/*    58    */
    {0x05, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x02,            0xff},/*    59    */
    {0x05, I2C_9548_DEVADDR, 0x05,            I2C_9545_DEVADDR, 0x03,            0xff},/*    60    */
    {0x05, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x02,            0xff},/*    61    */
    {0x05, I2C_9548_DEVADDR, 0x06,            I2C_9545_DEVADDR, 0x03,            0xff},/*    62    */
    {0x05, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x02,            0xff},/*    63    */
    {0x05, I2C_9548_DEVADDR, 0x07,            I2C_9545_DEVADDR, 0x03,            0xff},/*    64    */

    {0x00, NONE_DEVADDR, NONE_CHANNEL,    NONE_DEVADDR, NONE_CHANNEL,    0x50},/*enumEEPROM*/
    {0x02, NONE_DEVADDR, NONE_CHANNEL,    I2C_9545_DEVADDR, 0x01,            0x48},/*enumLM75*/
    {0x02, NONE_DEVADDR, NONE_CHANNEL,    I2C_9545_DEVADDR, 0x00,            0x18},/*enum6696*/

};

void dumpdata(u8 *data, u16 len)
{
    int  i = 0;
    for (i = 0; i < len; i++)
    {
        printk("%02x", data[i]);
        if ((i + 1) % 8 == 0)
            printk("\n");
    }
    printk("\n");
    return;
}
/*
    caution :
*/
long cpld_read_byte(u8 *value, volatile void *offset)
{
    *value = readb(offset);
    return 0;
}
/*
    caution :
*/
long cpld_write_byte(u8 value, volatile void *offset)
{
    writeb(value, offset);
    return 0;
}
long cpld_set_bit(u16 cpld_offset, u8 bit, u8 value)
{
    u8 val = 0;
    long ret;
    /*
        args validity check
    */
    if ((0 != value) && (1 != value))
    {
        cpld_debug("sensor6696_init args invalid !\n");
        return -EINVAL;
    }
    ret = cpld_read_byte(&val, (g_u64Cpld_addr + cpld_offset));
    if (0 != ret)
    {
        cpld_debug("sensor6696_init failed !\n");
        return ret;
    }
    if (0 == value)
        val &= (~(1 << bit));
    else
        val |= (1 << bit);
    cpld_debug("cpld_set_bit: set 0x%x to %x!\n", cpld_offset, val);
    ret = cpld_write_byte(val, (g_u64Cpld_addr + cpld_offset));
    if (0 != ret)
    {
        cpld_debug("sensor6696_init failed !\n");
        return ret;
    }
    return 0;
}

long i2c_init(void)
{
    u8 i = 0;
    do
    {
        smbus = i2c_get_adapter(i);
        cpld_debug("i2c %d: %s\n", i, smbus->name);
        if ((NULL == smbus) || (NULL != strstr(smbus->name, "SMBus")))
            break;
        i2c_put_adapter(smbus);
        cpld_debug("i2c put adapter %d\n", i);
        i++;
    } while (1);

    if (NULL == smbus)
    {
        cpld_debug("error : i2c adapter SMBus not found!!\n");
        return -ENODEV;
    }
    else
    {
        cpld_debug("i2c adapter SMBus I801 found success!!\n");
    }
    return 0;
}

long sensor6696_init(void)
{
    /*
        先写1再写0进行复位
    */
    long ret = 0;
    ret = cpld_set_bit(0x215, 4, 1);
    if (0 != ret)
        return ret;
    ret = cpld_set_bit(0x215, 4, 0);
    if (0 != ret)
        return ret;
    /*
        delay一下， 这个125ms是验证之后的经验值
    */
    mdelay(125);

    return 0;
}
void i2c_exit(void)
{
    i2c_put_adapter(smbus);
    return;
}

/*
typedef struct{
    u8 ucChannelId;

    u8 uc9548DevAddr;
    u8 uc9548ChannelId;

    u8 uc9545DevAddr;
    u8 uc9545ChannelId;

    u8 ucDevAddr;
}I2C_Device;
*/
long select_i2c_device_with_device_table(enum i2c_device_enum index)
{
    union i2c_smbus_data data;
    long ret = 0;

    cpld_debug("ucChannelId uc9548DevAddr uc9548ChannelId uc9545DevAddr uc9545ChannelId : 0x%x 0x%x 0x%x 0x%x 0x%x\n",
               i2c_device_table[index].ucChannelId, i2c_device_table[index].uc9548DevAddr, i2c_device_table[index].uc9548ChannelId,
               i2c_device_table[index].uc9545DevAddr, i2c_device_table[index].uc9545ChannelId);

    if (0 != cpld_write_byte(i2c_device_table[index].ucChannelId, (g_u64Cpld_addr + 0x247)))
    {
        cpld_debug("select 9545:%d failed !\n", i2c_device_table[index].ucChannelId);
        return -EINVAL;
    }
    /*所要选通的I2C器件前级有9548*/
    if (NONE_DEVADDR != i2c_device_table[index].uc9548DevAddr)
    {
        //select 9548 记住， 选9548的地方使用的是I2C_SMBUS_BYTE，相比于 I2C_SMBUS_BYTE_DATA，它只发送command不发送data
        ret = i2c_smbus_xfer(smbus, i2c_device_table[index].uc9548DevAddr, 0, I2C_SMBUS_WRITE, (1 << i2c_device_table[index].uc9548ChannelId), I2C_SMBUS_BYTE, &data);
        if (0 != ret)
        {
            cpld_debug("select 9548 failed\n");
            return ret;
        }
    }
    /*所要选通的I2C器件前级有9545*/
    if (NONE_DEVADDR != i2c_device_table[index].uc9545DevAddr)
    {
        //select 9545 记住， 选9545的地方使用的是I2C_SMBUS_BYTE，相比于 I2C_SMBUS_BYTE_DATA，它只发送command不发送data
        ret = i2c_smbus_xfer(smbus, i2c_device_table[index].uc9545DevAddr, 0, I2C_SMBUS_WRITE, (1 << i2c_device_table[index].uc9545ChannelId), I2C_SMBUS_BYTE, &data);
        if (0 != ret)
        {
            cpld_debug("select 9545 failed\n");
            return ret;
        }
    }
    return 0;
}

/*
    firtly select CPU_I2C2 or CPU_I2C4, and then select channel(0~3)
    number: 2 or 4
    channel : 0~3
*/
long select_9545(u8 number, u8 channel)
{
    union i2c_smbus_data data;
    long ret = 0;
    //    cpld_debug("number %d, channel %d\n", number, channel);
    if (((2 != number) && (4 != number)))
    {
        cpld_debug("this piece of 9545 does not exist!\n");
        return -EINVAL;
    }
    if (3 < channel)
    {
        cpld_debug("this channel of 9545 does not exist!\n");
        return -EINVAL;
    }
    ret = cpld_write_byte(number, (g_u64Cpld_addr + 0x247));
    if (ret != 0)
    {
        cpld_debug("select 9545:%d failed !\n", number);
        return ret;
    }

    //select 9545 记住， 选9545的地方使用的是I2C_SMBUS_BYTE，相比于 I2C_SMBUS_BYTE_DATA，它只发送command不发送data
    ret = i2c_smbus_xfer(smbus, 0x73, 0, I2C_SMBUS_WRITE, (1 << channel), I2C_SMBUS_BYTE, &data);
    if (0 != ret)
    {
        cpld_debug("select 9545:%d failed %ld\n", number, ret);
        return ret;
    }
    return 0;
}

long detach_9545(u8 number)
{
    union i2c_smbus_data data;
    long ret = 0;
    if ((2 != number && 4 != number))
    {
        cpld_debug("this piece of 9545 does not exist!\n");
        return -EINVAL;
    }

    ret = cpld_write_byte(number, (g_u64Cpld_addr + 0x247));
    if (0 != ret)
    {
        cpld_debug("select 9545:%d failed !\n", number);
        return ret;
    }

    //select 9545 记住， 选9545的地方使用的是I2C_SMBUS_BYTE，相比于 I2C_SMBUS_BYTE_DATA，它只发送command不发送data
    ret = i2c_smbus_xfer(smbus, 0x73, 0, I2C_SMBUS_WRITE, 0, I2C_SMBUS_BYTE, &data);
    if (0 != ret)
    {
        cpld_debug("detach 9545:%d failed %ld\n", number, ret);
        return ret;
    }
    return 0;
}

long lpc_cpld_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    CPLDrw stTmp;
    u8 copy_size =  1 + (u8)sizeof(CPLDrw);
    long ret = 0;
    switch (cmd)
    {
        /*read  */
        case CPLD_READ:
        {
            volatile void *addr;
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDhdr)))  /*size of CPLDhdr*/
            {
                cpld_debug("0x22 copy from user failed\n");
                return -EFAULT;
            }
            /*read cpld*/
            addr = (volatile void *)((u64)g_u64Cpld_addr + stTmp.header.offset);
            stTmp.data[0] = readb(addr);
            if (copy_to_user((u8 *)arg, (u8 *)&stTmp, copy_size))
                return -EFAULT;
            break;
        }
        /*write */
        case CPLD_WRITE:
        {
            volatile void *addr;
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, 1 + sizeof(CPLDhdr)))
            {
                cpld_debug("0x11 copy from user failed\n");
                return -EFAULT;
            }
            /*write cpld*/
            addr = (volatile void *)((u64)(g_u64Cpld_addr + stTmp.header.offset));
            writeb(stTmp.data[0], addr);
            break;
        }
#define CPU_I2Cx   2
            //#define CHANNEL_ID 1<<2
#define CHANNEL_ID 2
#define DEV_ADDRES 0x50
        case EEPROM_READ:
        {
            u16 addr;
            union i2c_smbus_data data;
            u8 channel = CHANNEL_ID;

            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("OPTIC_WRITE copy from user failed\n");
                return -EFAULT;
            }

            if (0x1c == g_ucBoardType)
            {
                ret = select_i2c_device_with_device_table(enumEEPROM);
                if (0 != ret)
                {
                    cpld_debug("EEPROM_READ 1 failed %ld\n", ret);
                    return ret;
                }
            }
            else
            {
                ret = select_9545(CPU_I2Cx, channel);
                if (0 != ret)
                {
                    cpld_debug("EEPROM_READ 1 failed %ld\n", ret);
                    return ret;
                }
            }
            /*write eeprom offset */
            addr = stTmp.header.offset;
            data.byte = (u8)(addr & 0x0ff);
            ret = i2c_smbus_xfer(smbus, DEV_ADDRES, 0, I2C_SMBUS_WRITE, (u8)((addr >> 8) & 0xff), I2C_SMBUS_BYTE_DATA, &data);
            if (0 != ret)
            {
                cpld_debug("EEPROM_READ 2 failed %ld\n", ret);
                return ret;
            }
            /*read data at the offset on eeprom*/
            ret = i2c_smbus_xfer(smbus, DEV_ADDRES, 0, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
            if (0 != ret)
            {
                cpld_debug("EEPROM_READ 3 failed %ld\n", ret);
                return ret;
            }

            /*read data success, process data*/
            cpld_debug("read eeprom offset %x data : %x\n", addr, data.byte);
            if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                return -EFAULT;
            break;
        }
        case EEPROM_WRITE:
        {
            u16 addr = 0;
            union i2c_smbus_data data;
            u8 real_data_byte = 0;
            u8 channel = CHANNEL_ID;

            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("EEPROM_WRITE copy from user failed\n");
                return -EFAULT;
            }

            if (0x1c == g_ucBoardType)
            {
                ret = select_i2c_device_with_device_table(enumEEPROM);
                if (0 != ret)
                {
                    cpld_debug("EEPROM_WRITE 1 failed %ld\n", ret);
                    return ret;
                }
            }
            else
            {
                ret = select_9545(CPU_I2Cx, channel);
                if (0 != ret)
                {
                    cpld_debug("EEPROM_WRITE 1 failed %ld\n", ret);
                    return ret;
                }
            }

            /*write eeprom */
            real_data_byte = stTmp.data[0];
            addr = stTmp.header.offset; //2byte address
            data.word = ((real_data_byte << 8) | (addr & 0x00ff));
            ret = i2c_smbus_xfer(smbus, DEV_ADDRES, 0, I2C_SMBUS_WRITE, (u8)((addr >> 8) & 0xff), I2C_SMBUS_WORD_DATA, &data);
            if (0 != ret)
            {
                cpld_debug("EEPROM_WRITE 2 failed %ld\n", ret);
                return ret;
            }

            cpld_debug("write eeprom offset=%x, data=%x\n", stTmp.header.offset, stTmp.data[0]);
            break;
        }
        case SELECT_OPTIC_I2C:
        {
            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("OPTIC_WRITE copy from user failed\n");
                return -EFAULT;
            }
            /*data[0] 存放光模块索引传入驱动*/
            if (stTmp.data[0] - 1 >= 0)
            {
                ret = select_i2c_device_with_device_table(stTmp.data[0] - 1);
            }
            else
            {
                cpld_debug("OPTIC_WRITE copy from user failed\n");
                return -EFAULT;
            }

            if (0 != ret)
            {
                cpld_debug("SELECT_OPTIC_I2C 1 failed %ld\n", ret);
                return ret;
            }
            break;
        }
#if 0
        case OPTIC_READ_sfp_51:
        {
            optic_addr = 0x51;
        }
        case OPTIC_READ:
        {
            union i2c_smbus_data data;
            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("OPTIC_READ copy from user failed\n");
                return -EFAULT;
            }

            /*select page*/
            //TODO

            /*read data at the offset on optical module*/
            data.block[0] = (u8)stTmp.header.size;

            ret = i2c_smbus_xfer(smbus, optic_addr, 0, I2C_SMBUS_READ, (u8)stTmp.header.offset, I2C_SMBUS_BYTE_DATA, &data);
            //ret = i2c_smbus_xfer(smbus, optic_addr, 0, I2C_SMBUS_READ, (u8)stTmp.header.offset, I2C_SMBUS_I2C_BLOCK_DATA, &data );
            if (0 != ret)
            {
                cpld_debug("OPTIC READ 2 failed %ld  size=%u,offset=%x\n", ret, stTmp.header.size, stTmp.header.offset);
                return ret;
            }

            /*read data success, process data*/
            cpld_debug("optic read size=%x,offset=%x,value=%x\n", stTmp.header.size, stTmp.header.offset, data.block[0]);
            if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                return -EFAULT;
            break;
        }
        case OPTIC_WRITE_sfp_51:
        {
            optic_addr = 0x51;
        }
        case OPTIC_WRITE:
        {
            u16 addr = 0;
            union i2c_smbus_data data;

            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("OPTIC_WRITE copy from user failed\n");
                return -EFAULT;
            }

            /*select page*/
            //TODO
            /*write data at the offset on optical module*/
            addr = (u8)stTmp.header.offset;
            data.block[0] = (u8)stTmp.header.size;
            cpld_debug("%x\n", stTmp.data[0]);
            /*copy data to data[]*/
            memcpy(&(data.block[1]), stTmp.data, (u8)stTmp.data[0]);
            memcpy(&(data.block[0]), stTmp.data, (u8)stTmp.header.size);
            ret = i2c_smbus_xfer(smbus, optic_addr, 0, I2C_SMBUS_WRITE, (u8)stTmp.header.offset, I2C_SMBUS_BYTE_DATA, &data);
            cpld_debug("optic_addr=%x offset=%x data=%x \n", optic_addr, stTmp.header.offset, stTmp.data[0]);
            //ret = i2c_smbus_xfer(smbus, optic_addr, 0, I2C_SMBUS_WRITE, (u8)stTmp.header.offset, I2C_SMBUS_I2C_BLOCK_DATA, &data );
            if (0 != ret)
            {
                cpld_debug("OPTIC_WRITE 2 failed %ld\n", ret);
                return ret;
            }

            /*write data success, process data*/
            cpld_debug("optic write size=%x,offset=%x,value=%x\n", stTmp.header.size, stTmp.header.offset, data.block[1]);

            break;
        }
#endif
#define _CPU_I2Cx   2
            //#define _CHANNEL_ID 1
#define _DEV_ADDRES 0x48
#define _DEV_ADDRES_1 0x18
        case SENSOR_READ:
        {
            union i2c_smbus_data data;
            u8 channel;
            u8 sensor_internal_reg_addr = 0;//用来选择6696的内部寄存器偏移，作为command传入
            u16 dev_addr = 0;

            /*get data from user space*/
            if (copy_from_user((u8 *)&stTmp, (u8 *)arg, sizeof(CPLDrw)))  /*size of CPLDhdr*/
            {
                cpld_debug("SENSOR_READ copy from user failed\n");
                return -EFAULT;
            }
            /*
                在这里重新复位6696的原因是，发现每次复位之后6696只能好使一段时间， 所以每次读之前复位一下
            */
            ret = sensor6696_init();
            if (0 != ret)
            {
                cpld_debug("SENSOR_READ sensor6696_init failed failed\n");
                return ret;
            }

            switch (stTmp.data[0])
            {
                case LM75:
                {
                    dev_addr = _DEV_ADDRES;
                    channel = 1;
                    sensor_internal_reg_addr = 0x00;

                    if (0x1c == g_ucBoardType)
                    {
                        ret = select_i2c_device_with_device_table(enumLM75);
                        if (0 != ret)
                        {
                            cpld_debug("EEPROM_READ 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    else
                    {
                        ret = select_9545(_CPU_I2Cx, channel);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ LM75 1 failed %ld\n", ret);
                            return ret;
                        }
                    }

                    cpld_debug("SENSOR_READ sensor N.O.%d : dev_addr %x channel %x\n", stTmp.data[0], dev_addr, channel);

                    /*read data at the offset on sensor*/
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_READ, sensor_internal_reg_addr, I2C_SMBUS_BYTE, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ LM75 3 failed %ld\n", ret);
                        return ret;
                    }

                    /*read data success, process data*/
                    cpld_debug("SENSOR_READ LM75 offset %x data : %x\n", sensor_internal_reg_addr, data.byte);
                    if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                        return EFAULT;

                    break;
                }
                case Max6696_0:
                {
                    dev_addr = _DEV_ADDRES_1;
                    channel = 0;
                    sensor_internal_reg_addr = 0x00;
                    if (0x1c == g_ucBoardType)
                    {
                        ret = select_i2c_device_with_device_table(enum6696);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_0 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    else
                    {
                        ret = select_9545(_CPU_I2Cx, channel);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_0 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    cpld_debug("SENSOR_READ sensor N.O.%d : dev_addr %x channel %x sensor_internal_reg_addr %x\n", stTmp.data[0], dev_addr, channel, sensor_internal_reg_addr);

                    /*read data at the offset on sensor*/
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_READ, sensor_internal_reg_addr, I2C_SMBUS_BYTE, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ Max6696_0 3 failed %ld\n", ret);
                        return ret;
                    }

                    /*read data success, process data*/
                    cpld_debug("read sensor Max6696_0 offset %x data : %x\n", sensor_internal_reg_addr, data.byte);
                    if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                        return -EFAULT;

                    break;
                }
                case Max6696_1:
                {
                    dev_addr = _DEV_ADDRES_1;
                    channel = 0;
                    sensor_internal_reg_addr = 0x01;
                    if (0x1c == g_ucBoardType)
                    {
                        ret = select_i2c_device_with_device_table(enum6696);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_1 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    else
                    {
                        ret = select_9545(_CPU_I2Cx, channel);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_1 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    cpld_debug("SENSOR_READ sensor N.O.%d : dev_addr %x channel %x sensor_internal_reg_addr %x\n", stTmp.data[0], dev_addr, channel, sensor_internal_reg_addr);

                    /*read data at the offset on sensor*/
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_READ, sensor_internal_reg_addr, I2C_SMBUS_BYTE, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ Max6696_1 3 failed %ld\n", ret);
                        return ret;
                    }

                    /*read data success, process data*/
                    cpld_debug("read sensor Max6696_1 offset %x data : %x\n", sensor_internal_reg_addr, data.byte);
                    if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                        return -EFAULT;

                    break;
                }
                case Max6696_2:
                {
                    dev_addr = _DEV_ADDRES_1;
                    channel = 0;
                    if (0x1c == g_ucBoardType)
                    {
                        ret = select_i2c_device_with_device_table(enum6696);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_2 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    else
                    {
                        ret  = select_9545(_CPU_I2Cx, channel);
                        if (0 != ret)
                        {
                            cpld_debug("SENSOR_READ Max6696_2 1 failed %ld\n", ret);
                            return ret;
                        }
                    }
                    /*write sensor internal register 0x09 with 0x08 to select channel 2 hotspot*/
                    sensor_internal_reg_addr = 0x09;
                    data.byte = (u8)(0x08);
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_WRITE, sensor_internal_reg_addr, I2C_SMBUS_BYTE_DATA, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ Max6696_2 2 failed %ld\n", ret);
                        return ret;
                    }
                    /*read data at the offset on sensor*/
                    sensor_internal_reg_addr = 0x01;
                    cpld_debug("SENSOR_READ sensor N.O.%d : dev_addr %x channel %x sensor_internal_reg_addr %x\n", stTmp.data[0], dev_addr, channel, sensor_internal_reg_addr);
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_READ, sensor_internal_reg_addr, I2C_SMBUS_BYTE, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ Max6696_2 3 failed %ld\n", ret);
                        return ret;
                    }

                    /*read data success, process data*/
                    cpld_debug("read sensor Max6696_2 offset %x data : %x\n", sensor_internal_reg_addr, data.byte);
                    if (copy_to_user(((CPLDrw *)arg)->data, data.block, stTmp.header.size))
                        return -EFAULT;

                    /*write sensor internal register 0x09 with 0x00 to unselect channel 2 hotspot*/
                    sensor_internal_reg_addr = 0x09;
                    data.byte = (u8)(0x00);
                    ret = i2c_smbus_xfer(smbus, dev_addr, 0, I2C_SMBUS_WRITE, sensor_internal_reg_addr, I2C_SMBUS_BYTE_DATA, &data);
                    if (0 != ret)
                    {
                        cpld_debug("SENSOR_READ Max6696_2 4 failed %ld\n", ret);
                        return ret;
                    }
                    break;
                }
                default:
                {
                    cpld_debug("SENSOR_READ sensor id error\n");
                    return -EINVAL;
                }
            }
            break;
        }
        case I2C_RESET:
        {
            u8 tmp;
            /*9545 复位，解复位*/
            tmp = readb((g_u64Cpld_addr + 0x216));
            writeb(tmp | 0x3, (g_u64Cpld_addr + 0x216)); //bit 0 1 是两个9545的复位寄存器
            break;
        }
        default:
        {
            //TODO
            cpld_debug("invalid command\n");
            return -EINVAL;
        }
    }
    return 0;
}

/*read a single byte */
ssize_t lpc_cpld_read_byte(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    return 0;
}

/*write a single byte */
ssize_t lpc_cpld_write_byte(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    return 0;
}

int lpc_cpld_open(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations cpld_fops =
{
    .owner = THIS_MODULE,
    .write = lpc_cpld_write_byte,
    .read = lpc_cpld_read_byte,
    .open = lpc_cpld_open,
    .unlocked_ioctl = lpc_cpld_ioctl,
};

int lpc_dbg_init(void)
{
    cpld_debug("lpc_dbg_init\n");
    register_chrdev(major, "cpld", &cpld_fops);//（如果major为0，则是让系统自动分配）
    class = class_create(THIS_MODULE, "cpld");
    device_create(class, NULL, MKDEV(major, 0), NULL, "cpld");

    if (0 != i2c_init())
    {
        unregister_chrdev(major, "cpld");
        device_destroy(class, MKDEV(major, 0));
        class_destroy(class);
        cpld_debug("lpc_dbg_init i2c init failed\n");
        return -1;
    }
    g_u64Cpld_addr = ioremap(CPLD_BASE_ADDR, 1024);//CPLD_BASE_ADDR + CPLD_OFFSET = 0xdffe2200
    cpld_debug("virtual cpld address is %p\n", g_u64Cpld_addr);

    if (0 != sensor6696_init())
    {
        unregister_chrdev(major, "cpld");
        device_destroy(class, MKDEV(major, 0));
        class_destroy(class);
        iounmap(g_u64Cpld_addr);
        cpld_debug("lpc_dbg_init sensor init failed\n");
        return -1;
    }

    if (0 != cpld_read_byte(&g_ucBoardType, (g_u64Cpld_addr + 0x202)))
    {
        cpld_debug("get g_ucBoardType failed !\n");
        return -1;
    }
    cpld_debug("g_ucBoardType 0x%x\n", g_ucBoardType);
    return 0;
}

void lpc_dbg_exit(void)
{
    cpld_debug("lpc_dbg_exit\n");
    detach_9545(2);
    detach_9545(4);
    unregister_chrdev(major, "cpld");
    device_destroy(class, MKDEV(major, 0));
    class_destroy(class);
    iounmap(g_u64Cpld_addr);
    i2c_exit();
    return;
}
/*宏定义, 声明一个模块的初始化和清理函数*/
module_init(lpc_dbg_init);
module_exit(lpc_dbg_exit);

MODULE_LICENSE("Dual BSD/GPL");
