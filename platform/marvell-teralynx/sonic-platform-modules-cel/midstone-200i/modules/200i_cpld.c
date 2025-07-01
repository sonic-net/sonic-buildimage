/*
 * ms200i_cpld.c - driver for MidStone's CPLD
 *
 * Copyright (C) 2017 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>

#define DRIVER_NAME "ms200i_cpld"

#define RESET0102   0xA248
#define RESET0310   0xA2DC
#define RESET1118   0xA2DD
#define RESET1926   0xA2DE
#define RESET2733   0xA2DF
#define RESET3441   0xA31C
#define RESET4249   0xA31D
#define RESET5057   0xA31E
#define RESET5864   0xA31F

#define LPMOD0102   0xA249
#define LPMOD0310   0xA2E0
#define LPMOD1118   0xA2E1
#define LPMOD1926   0xA2E2
#define LPMOD2733   0xA2E3
#define LPMOD3441   0xA320
#define LPMOD4249   0xA321
#define LPMOD5057   0xA322
#define LPMOD5864   0xA323

#define ABS0102   0xA24A
#define ABS0310   0xA2E4
#define ABS1118   0xA2E5
#define ABS1926   0xA2E6
#define ABS2733   0xA2E7
#define ABS3441   0xA324
#define ABS4249   0xA325
#define ABS5057   0xA326
#define ABS5864   0xA327

#define ABS6566   0xA244

#define INT0102   0xA24B
#define INT0310   0xA2E8
#define INT1118   0xA2E9
#define INT1926   0xA2EA
#define INT2733   0xA2EB
#define INT3441   0xA328
#define INT4249   0xA329
#define INT5057   0xA32A
#define INT5864   0xA32B

#define LENGTH_PORT_CPLD                66

#define CPLD2_EX_CP_I2CFDR0_I2C         0xA230
#define CPLD2_EX_CP_I2CCR0_I2C          0xA231
#define CPLD2_EX_CP_I2CSR0_I2C          0xA232
#define CPLD2_EX_CP_I2CDR0_I2C          0xA233
#define CPLD2_EX_CP_I2CID0_I2C          0xA234

#define CPLD3_EX_CP_I2CFDR0_I2C         0xA2D0
#define CPLD3_EX_CP_I2CCR0_I2C          0xA2D1
#define CPLD3_EX_CP_I2CSR0_I2C          0xA2D2
#define CPLD3_EX_CP_I2CDR0_I2C          0xA2D3
#define CPLD3_EX_CP_I2CID0_I2C          0xA2D4

#define CPLD4_EX_CP_I2CFDR0_I2C         0xA310
#define CPLD4_EX_CP_I2CCR0_I2C          0xA311
#define CPLD4_EX_CP_I2CSR0_I2C          0xA312
#define CPLD4_EX_CP_I2CDR0_I2C          0xA313
#define CPLD4_EX_CP_I2CID0_I2C          0xA314

enum {
    I2C_SR_BIT_RXAK = 0,
    I2C_SR_BIT_MIF,
    I2C_SR_BIT_SRW,
    I2C_SR_BIT_BCSTM,
    I2C_SR_BIT_MAL,
    I2C_SR_BIT_MBB,
    I2C_SR_BIT_MAAS,
    I2C_SR_BIT_MCF
};

enum {
    I2C_CR_BIT_BCST = 0,
    I2C_CR_BIT_RSTA = 2,
    I2C_CR_BIT_TXAK,
    I2C_CR_BIT_MTX,
    I2C_CR_BIT_MSTA,
    I2C_CR_BIT_MIEN,
    I2C_CR_BIT_MEN,
};

#ifdef DEBUG_KERN
#define info(fmt,args...)  printk(KERN_INFO "line %3d : "fmt,__LINE__,##args)
#define check(REG)         printk(KERN_INFO "line %3d : %-8s = %2.2X",__LINE__,#REG,inb(REG));
#else
#define info(fmt,args...)
#define check(REG)
#endif

#define GET_REG_BIT(REG,BIT)   ((inb(REG) >> BIT) & 0x01)
#define SET_REG_BIT_H(REG,BIT) outb(inb(REG) |  (0x01 << BIT),REG)
#define SET_REG_BIT_L(REG,BIT) outb(inb(REG) & ~(0x01 << BIT),REG)

struct ms200i_i2c_data {
        int portid;
        unsigned REG_FDR0;
        unsigned REG_CR0;
        unsigned REG_SR0;
        unsigned REG_DR0;
        unsigned REG_ID0;
};

struct ms200i_cpld_data {
        struct i2c_adapter *i2c_adapter[LENGTH_PORT_CPLD];
        struct mutex       cpld_lock;
        unsigned char sfpp_lpmode[2];
        unsigned char sfpp_reset[2];
};

struct ms200i_cpld_data *cpld_data;

int strtobp(const char* str,unsigned char *bytes){
    unsigned length = strlen(str);
    if(length > 20){
        return 0;
    }
    int i,b=0;
    memset(bytes,0,10);
    for(i=0;i<length;i++){
        unsigned char byte;
        char c = str[length-1-i];
        if(c >= '0' && c <= '9'){
            byte = c - '0';
        }else if(c >= 'a' && c <= 'f'){
            byte = c - 'a' + 0x0a;
        }else if(c >= 'A' && c <= 'F'){
            byte = c - 'A' + 0x0a;
        }else if(c == 'x' || c == 'X'){
            break;
        }else{
            continue;
        }
        if(b%2==0)
            bytes[b/2] = byte & 0x0F;
        else
            bytes[b/2] += (byte << 4) & 0xF0;
        b++;
    }
    return (i/2) + (i%2);
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
        unsigned long reset = 0;

        mutex_lock(&cpld_data->cpld_lock);

        reset =
            ((unsigned long)(inb(RESET5864) & 0x7F)   << 57 )|
            ((unsigned long) inb(RESET5057)           << 49 )|
            ((unsigned long) inb(RESET4249)           << 41 )|
            ((unsigned long) inb(RESET3441)           << 33 )|
            ((unsigned long)(inb(RESET2733) & 0x7F)   << 26 )|
            ((unsigned long) inb(RESET1926)           << 18 )|
            ((unsigned long) inb(RESET1118)           << 10 )|
            ((unsigned long) inb(RESET0310)           << 2  )|
            ((unsigned long)(inb(RESET0102) & 0x03)         );

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%x%16.16lx\n",
                    cpld_data->sfpp_reset[1] << 1 | cpld_data->sfpp_reset[0],
                    reset & 0x3ffffffffffffffff);
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
        unsigned char reset[10];
        int num;

        mutex_lock(&cpld_data->cpld_lock);

        num = strtobp(buf,reset);
        if (num <= 0)
        {
                mutex_unlock(&cpld_data->cpld_lock);
                return 22;
        }

        outb (( reset[0] & 0x03 )          ,RESET0102);
        outb (((reset[0] >>2) & 0x3F) |
             (((reset[1] )    & 0x03) << 6),RESET0310);
        outb (((reset[1] >>2) & 0x3F) |
             (((reset[2] )    & 0x03) << 6),RESET1118);
        outb (((reset[2] >>2) & 0x3F) |
             (((reset[3] )    & 0x03) << 6),RESET1926);
        outb (((reset[3] >>2) & 0x3F) |
             (((reset[4] )    & 0x01) << 6),RESET2733);
        outb (((reset[4] >>1) & 0x7F) |
             (((reset[5] )    & 0x01) << 7),RESET3441);
        outb (((reset[5] >>1) & 0x7F) |
             (((reset[6] )    & 0x01) << 7),RESET4249);
        outb (((reset[6] >>1) & 0x7F) |
             (((reset[7] )    & 0x01) << 7),RESET5057);
        outb (((reset[7] >>1) & 0x7F)      ,RESET5864);

        cpld_data->sfpp_reset[0] = reset[8]      & 0x01;
        cpld_data->sfpp_reset[1] = (reset[8]>>1) & 0x01;

        mutex_unlock(&cpld_data->cpld_lock);

        return count;
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
        unsigned long lpmod = 0;

        mutex_lock(&cpld_data->cpld_lock);

        lpmod =
            ((unsigned long)(inb(LPMOD5864) & 0x7F) << 57 )|
            ((unsigned long) inb(LPMOD5057)         << 49 )|
            ((unsigned long) inb(LPMOD4249)         << 41 )|
            ((unsigned long) inb(LPMOD3441)         << 33 )|
            ((unsigned long)(inb(LPMOD2733) & 0x7F) << 26 )|
            ((unsigned long) inb(LPMOD1926)         << 18 )|
            ((unsigned long) inb(LPMOD1118)         << 10 )|
            ((unsigned long) inb(LPMOD0310)         << 2  )|
            ((unsigned long)(inb(LPMOD0102) & 0x03)       );

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%x%16.16lx\n",
                    cpld_data->sfpp_lpmode[1] << 1 | cpld_data->sfpp_lpmode[0],
                    lpmod & 0xffffffffffffffff);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
        unsigned char lpmod[10];
        int num;

        mutex_lock(&cpld_data->cpld_lock);

        num = strtobp(buf,lpmod);
        if (num <= 0)
        {
                mutex_unlock(&cpld_data->cpld_lock);
                return 22;
        }

        outb (( lpmod[0] & 0x03 )          ,LPMOD0102);
        outb (((lpmod[0] >>2) & 0x3F) |
             (((lpmod[1] )    & 0x03) << 6),LPMOD0310);
        outb (((lpmod[1] >>2) & 0x3F) |
             (((lpmod[2] )    & 0x03) << 6),LPMOD1118);
        outb (((lpmod[2] >>2) & 0x3F) |
             (((lpmod[3] )    & 0x03) << 6),LPMOD1926);
        outb (((lpmod[3] >>2) & 0x3F) |
             (((lpmod[4] )    & 0x01) << 6),LPMOD2733);
        outb (((lpmod[4] >>1) & 0x7F) |
             (((lpmod[5] )    & 0x01) << 7),LPMOD3441);
        outb (((lpmod[5] >>1) & 0x7F) |
             (((lpmod[6] )    & 0x01) << 7),LPMOD4249);
        outb (((lpmod[6] >>1) & 0x7F) |
             (((lpmod[7] )    & 0x01) << 7),LPMOD5057);
        outb (((lpmod[7] >>1) & 0x7F)      ,LPMOD5864);

        cpld_data->sfpp_lpmode[0] = lpmod[8]      & 0x01;
        cpld_data->sfpp_lpmode[1] = (lpmod[8]>>1) & 0x01;

        mutex_unlock(&cpld_data->cpld_lock);

        return count;
}

static ssize_t get_modprs(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
	unsigned long present;
        int present_sfp;

        mutex_lock(&cpld_data->cpld_lock);
        present =
            ((unsigned long)(inb(ABS5864) & 0x7F) << 57  )|
            ((unsigned long) inb(ABS5057)         << 49  )|
            ((unsigned long) inb(ABS4249)         << 41  )|
            ((unsigned long) inb(ABS3441)         << 33  )|
            ((unsigned long)(inb(ABS2733) & 0x7F) << 26  )|
            ((unsigned long) inb(ABS1926)         << 18  )|
            ((unsigned long) inb(ABS1118)         << 10  )|
            ((unsigned long) inb(ABS0310)         << 2   )|
            ((unsigned long)(inb(ABS0102) & 0x03)        );

        present_sfp = (inb(ABS6566) & 0x03);
        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%d%16.16lx\n", present_sfp, present & 0xffffffffffffffff);
}

static ssize_t get_modirq(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
        unsigned long irq;

        mutex_lock(&cpld_data->cpld_lock);

        irq =
            ((unsigned long)(inb(INT5864) & 0x7F) << 57  )|
            ((unsigned long) inb(INT5057)         << 49  )|
            ((unsigned long) inb(INT4249)         << 41  )|
            ((unsigned long) inb(INT3441)         << 33  )|
            ((unsigned long)(inb(INT2733) & 0x7F) << 26  )|
            ((unsigned long) inb(INT1926)         << 18  )|
            ((unsigned long) inb(INT1118)         << 10  )|
            ((unsigned long) inb(INT0310)         << 2   )|
            ((unsigned long)(inb(INT0102) & 0x03)        );

        mutex_unlock(&cpld_data->cpld_lock);

        return sprintf(buf,"0x%17.17lx\n", irq  & 0x3ffffffffffffffff);
}

static DEVICE_ATTR(qsfp_reset , S_IRUGO | S_IWUSR, get_reset, set_reset);
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO | S_IWUSR, get_lpmode, set_lpmode);
static DEVICE_ATTR(qsfp_modprs, S_IRUGO, get_modprs, NULL);
static DEVICE_ATTR(qsfp_modirq, S_IRUGO, get_modirq, NULL);

static struct attribute *ms200i_lpc_attrs[] = {
        &dev_attr_qsfp_reset.attr,
        &dev_attr_qsfp_lpmode.attr,
        &dev_attr_qsfp_modprs.attr,
        &dev_attr_qsfp_modirq.attr,
        NULL,
};

static struct attribute_group ms200i_lpc_attr_grp = {
        .attrs = ms200i_lpc_attrs,
};

static struct resource cel_ms200i_lpc_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void cel_ms200i_lpc_dev_release( struct device * dev)
{
        return;
}

static struct platform_device cel_ms200i_lpc_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(cel_ms200i_lpc_resources),
        .resource       = cel_ms200i_lpc_resources,
        .dev = {
                        .release = cel_ms200i_lpc_dev_release,
        }
};


static int i2c_wait_ack(struct i2c_adapter *a,unsigned timeout,int writing){
    int error = 0;
    unsigned tick=0;
    int Status;

    struct ms200i_i2c_data *new_data = i2c_get_adapdata(a);

    check(new_data->REG_SR0);
    check(new_data->REG_CR0);

    while(1){
        Status = inb(new_data->REG_SR0);
        tick++;
        if(tick > timeout){
            info("Status %2.2X",Status);
            info("Error Timeout");
            error = -ETIMEDOUT;
            break;
        }


        if(Status & (1 << I2C_SR_BIT_MIF)){
            break;
        }

        if(writing == 0 && (Status & (1<<I2C_SR_BIT_MCF))){
            break;
        }
    }
    Status = inb(new_data->REG_SR0);
    outb(0, new_data->REG_SR0);

    if(error<0){
        info("Status %2.2X",Status);
        return error;
    }

    if(!(Status & (1 << I2C_SR_BIT_MCF))){
        info("Error Unfinish");
        return -EIO;
    }

    if(Status & (1 <<I2C_SR_BIT_MAL)){
        info("Error MAL");
        return -EAGAIN;
    }

    if(Status & (1 << I2C_SR_BIT_RXAK)){
        info( "SL No Acknowlege");
        if(writing){
            info("Error No Acknowlege");
            outb(new_data->REG_CR0,1<<I2C_CR_BIT_MEN);
            return -ENXIO;
        }
    }else{
        info( "SL Acknowlege");
    }

    return 0;
}

static int ms200i_i2c_access(struct i2c_adapter *a, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data)
{
        int error = 0;

        struct ms200i_i2c_data *new_data;

        mutex_lock(&cpld_data->cpld_lock);

        /* Write the command register */
        new_data = i2c_get_adapdata(a);

        unsigned int  portid = new_data->portid;

#ifdef DEBUG_KERN
        printk(KERN_INFO "portid %2d|@ 0x%2.2X|f 0x%4.4X|(%d)%-5s| (%d)%-10s|CMD %2.2X |DAT %4.4X"
            ,portid,addr,flags,rw,rw == 1 ? "READ ":"WRITE"
            ,size,                  size == 0 ? "QUICK" :
                                    size == 1 ? "BYTE" :
                                    size == 2 ? "BYTE_DATA" :
                                    size == 3 ? "WORD_DATA" :
                                    size == 4 ? "PROC_CALL" :
                                    size == 5 ? "BLOCK_DATA" :  "ERROR"
            ,cmd,data->word);
#endif
        /* Map the size to what the chip understands */
        switch (size) {
            case I2C_SMBUS_QUICK:
            case I2C_SMBUS_BYTE:
            case I2C_SMBUS_BYTE_DATA:
            case I2C_SMBUS_WORD_DATA:
            case I2C_SMBUS_BLOCK_DATA:
                break;
            default:
                printk(KERN_INFO "Unsupported transaction %d\n", size);
                error = -EOPNOTSUPP;
                goto Done;
        }

        unsigned int REG_FDR0;
        unsigned int REG_CR0;
        unsigned int REG_SR0;
        unsigned int REG_DR0;
        unsigned int REG_ID0;

        REG_FDR0  = new_data->REG_FDR0;
        REG_CR0   = new_data->REG_CR0;
        REG_SR0   = new_data->REG_SR0;
        REG_DR0   = new_data->REG_DR0;
        REG_ID0   = new_data->REG_ID0;
        outb(portid,REG_ID0);

        int timeout=0;
        int cnt=0;

        ////[S][ADDR/R]
        //Clear status register
        outb( 0 , REG_SR0);
        outb( 1 << I2C_CR_BIT_MIEN | 1 << I2C_CR_BIT_MTX | 1 << I2C_CR_BIT_MSTA ,REG_CR0);
        SET_REG_BIT_H(REG_CR0,I2C_CR_BIT_MEN);

        if(rw == I2C_SMBUS_READ &&
                (size == I2C_SMBUS_QUICK || size == I2C_SMBUS_BYTE)){
            // sent device address with Read mode
            outb(addr << 1 | 0x01,REG_DR0);
        }else{
            // sent device address with Write mode
            outb(addr << 1 | 0x00,REG_DR0);
        }



        info( "MS Start");

        //// Wait {A}
        error = i2c_wait_ack(a,50000,1);
        if(error<0){
            info( "get error %d",error);
            goto Done;
        }

        //// [CMD]{A}
        if(size == I2C_SMBUS_BYTE_DATA ||
            size == I2C_SMBUS_WORD_DATA ||
            size == I2C_SMBUS_BLOCK_DATA ||
            (size == I2C_SMBUS_BYTE && rw == I2C_SMBUS_WRITE)){

            //sent command code to data register
            outb(cmd,REG_DR0);
            info( "MS Send CMD 0x%2.2X",cmd);

            // Wait {A}
            error = i2c_wait_ack(a,50000,1);
            if(error<0){
                info( "get error %d",error);
                goto Done;
            }
        }

        switch(size){
            case I2C_SMBUS_BYTE_DATA:
                    cnt = 1;  break;
            case I2C_SMBUS_WORD_DATA:
                    cnt = 2;  break;
            case I2C_SMBUS_BLOCK_DATA:
            // in block data mode keep number of byte in block[0]
                    cnt = data->block[0];
                              break;
            default:
                    cnt = 0;  break;
        }

        // [CNT]  used only bloack data write
        if(size == I2C_SMBUS_BLOCK_DATA && rw == I2C_SMBUS_WRITE){

            outb(cnt,REG_DR0);
            info( "MS Send CNT 0x%2.2X",cnt);

            // Wait {A}
            error = i2c_wait_ack(a,50000,1);
            if(error<0){
                info( "get error %d",error);
                goto Done;
            }
        }

        // [DATA]{A}
        if( rw == I2C_SMBUS_WRITE && (
                    size == I2C_SMBUS_BYTE ||
                    size == I2C_SMBUS_BYTE_DATA ||
                    size == I2C_SMBUS_WORD_DATA ||
                    size == I2C_SMBUS_BLOCK_DATA
            )){
            int bid=0;
            info( "MS prepare to sent [%d bytes]",cnt);
            if(size == I2C_SMBUS_BLOCK_DATA ){
                bid=1;      // block[0] is cnt;
                cnt+=1;     // offset from block[0]
            }
            for(;bid<cnt;bid++){

                outb(data->block[bid],REG_DR0);
                info( "   Data > %2.2X",data->block[bid]);
                // Wait {A}
                error = i2c_wait_ack(a,50000,1);
                if(error<0){
                    goto Done;
                }
            }

        }

        //REPEATE START
        if( rw == I2C_SMBUS_READ && (
                size == I2C_SMBUS_BYTE_DATA ||
                size == I2C_SMBUS_WORD_DATA ||
                size == I2C_SMBUS_BLOCK_DATA
            )){
            info( "MS Repeated Start");

            SET_REG_BIT_L(REG_CR0,I2C_CR_BIT_MEN);
            outb(1 << I2C_CR_BIT_MIEN |
                1 << I2C_CR_BIT_MTX |
                1 << I2C_CR_BIT_MSTA |
                1 << I2C_CR_BIT_RSTA ,REG_CR0);
            SET_REG_BIT_H(REG_CR0,I2C_CR_BIT_MEN);

            // sent Address with Read mode
            outb( addr<<1 | 0x1 ,REG_DR0);

            // Wait {A}
            error = i2c_wait_ack(a,50000,1);
            if(error<0){
                goto Done;
            }

        }

        if( rw == I2C_SMBUS_READ && (
                size == I2C_SMBUS_BYTE ||
                size == I2C_SMBUS_BYTE_DATA ||
                size == I2C_SMBUS_WORD_DATA ||
                size == I2C_SMBUS_BLOCK_DATA
            )){

            switch(size){
                case I2C_SMBUS_BYTE:
                case I2C_SMBUS_BYTE_DATA:
                        cnt = 1;  break;
                case I2C_SMBUS_WORD_DATA:
                        cnt = 2;  break;
                case I2C_SMBUS_BLOCK_DATA:
                    //will be changed after recived first data
                        cnt = 3;  break;
                default:
                        cnt = 0;  break;
            }

            int bid = 0;
            info( "MS Receive");

            //set to Receive mode
            outb(1 << I2C_CR_BIT_MEN |
                1 << I2C_CR_BIT_MIEN |
                1 << I2C_CR_BIT_MSTA , REG_CR0);

            for(bid=-1;bid<cnt;bid++){

                // Wait {A}
                error = i2c_wait_ack(a,50000,0);
                if(error<0){
                    goto Done;
                }

                if(bid == cnt-2){
                    info( "SET NAK");
                    SET_REG_BIT_H(REG_CR0,I2C_CR_BIT_TXAK);
                }

                if(bid<0){
                    inb(REG_DR0);
                    info( "READ Dummy Byte" );
                }else{

                    if(bid==cnt-1){
                        info ( "SET STOP in read loop");
                        SET_REG_BIT_L(REG_CR0,I2C_CR_BIT_MSTA);
                    }
                    data->block[bid] = inb(REG_DR0);

                    info( "DATA IN [%d] %2.2X",bid,data->block[bid]);

                    if(size==I2C_SMBUS_BLOCK_DATA && bid == 0){
                        cnt = data->block[0] + 1;
                    }
                }
            }
        }


Stop:
        //[P]
        SET_REG_BIT_L(REG_CR0,I2C_CR_BIT_MSTA);
        info( "MS STOP");

Done:
        outb(1<<I2C_CR_BIT_MEN,REG_CR0);
        check(REG_CR0);
        check(REG_SR0);
#ifdef DEBUG_KERN
        printk(KERN_INFO "END --- Error code  %d",error);
#endif

        mutex_unlock(&cpld_data->cpld_lock);

        return error;
}

static u32 ms200i_i2c_func(struct i2c_adapter *a)
{
        return I2C_FUNC_SMBUS_QUICK |
            I2C_FUNC_SMBUS_BYTE |
            I2C_FUNC_SMBUS_BYTE_DATA |
            I2C_FUNC_SMBUS_WORD_DATA |
            I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm ms200i_i2c_algorithm = {
        .smbus_xfer = ms200i_i2c_access,
        .functionality  = ms200i_i2c_func,
};

static struct i2c_adapter * cel_ms200i_i2c_init(struct platform_device *pdev, int portid)
{
        int error;

        struct i2c_adapter *new_adapter;
        struct ms200i_i2c_data *new_data;

        new_adapter = kzalloc(sizeof(*new_adapter), GFP_KERNEL);
        if (!new_adapter)
            return NULL;

        new_adapter->dev.parent = &pdev->dev;
        new_adapter->owner = THIS_MODULE;
        new_adapter->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
        new_adapter->algo  = &ms200i_i2c_algorithm;

        snprintf(new_adapter->name, sizeof(new_adapter->name),
                        "SMBus ms200i i2c Adapter portid@%04x", portid);

        new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
        if (!new_data)
            return NULL;

        new_data->portid = portid;

        // QSFP 1-2 and SFP 1-2
        if((portid >= 1 && portid <= 2)  || (portid >= 65 && portid <= 66)){
            new_data->REG_FDR0  = CPLD2_EX_CP_I2CFDR0_I2C;
            new_data->REG_CR0   = CPLD2_EX_CP_I2CCR0_I2C;
            new_data->REG_SR0   = CPLD2_EX_CP_I2CSR0_I2C;
            new_data->REG_DR0   = CPLD2_EX_CP_I2CDR0_I2C;
            new_data->REG_ID0   = CPLD2_EX_CP_I2CID0_I2C;

        }else if((portid >= 3 && portid <= 33)){
            new_data->REG_FDR0  = CPLD3_EX_CP_I2CFDR0_I2C;
            new_data->REG_CR0   = CPLD3_EX_CP_I2CCR0_I2C;
            new_data->REG_SR0   = CPLD3_EX_CP_I2CSR0_I2C;
            new_data->REG_DR0   = CPLD3_EX_CP_I2CDR0_I2C;
            new_data->REG_ID0   = CPLD3_EX_CP_I2CID0_I2C;

        }else if((portid >= 34 && portid <= 64)){
            new_data->REG_FDR0  = CPLD4_EX_CP_I2CFDR0_I2C;
            new_data->REG_CR0   = CPLD4_EX_CP_I2CCR0_I2C;
            new_data->REG_SR0   = CPLD4_EX_CP_I2CSR0_I2C;
            new_data->REG_DR0   = CPLD4_EX_CP_I2CDR0_I2C;
            new_data->REG_ID0   = CPLD4_EX_CP_I2CID0_I2C;
        }
        outb(portid,new_data->REG_ID0);
        outb(0x1F,new_data->REG_FDR0); // 0x1F 100kHz


        i2c_set_adapdata(new_adapter,new_data);

        error = i2c_add_adapter(new_adapter);
        if(error)
            return NULL;

        return new_adapter;
};

static int cel_ms200i_lpc_drv_probe(struct platform_device *pdev)
{
        struct resource *res;
        int ret =0;
        int portid_count;

        cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct ms200i_cpld_data),
                        GFP_KERNEL);
        if (!cpld_data)
            return -ENOMEM;

        mutex_init(&cpld_data->cpld_lock);

        res = platform_get_resource(pdev, IORESOURCE_IO, 0);
        if (unlikely(!res)) {
            printk(KERN_ERR " Specified Resource Not Available...\n");
            return -1;
        }

        ret = sysfs_create_group(&pdev->dev.kobj, &ms200i_lpc_attr_grp);
        if (ret) {
            printk(KERN_ERR "Cannot create sysfs\n");
        }

        for(portid_count=1 ; portid_count<=LENGTH_PORT_CPLD ; portid_count++)
            cpld_data->i2c_adapter[portid_count-1] = cel_ms200i_i2c_init(pdev, portid_count);
        return 0;
}

static int cel_ms200i_lpc_drv_remove(struct platform_device *pdev)
{
        int portid_count;
        struct ms200i_i2c_data *new_data;

        sysfs_remove_group(&pdev->dev.kobj, &ms200i_lpc_attr_grp);

        for (portid_count=1 ; portid_count<=LENGTH_PORT_CPLD ; portid_count++)
            i2c_del_adapter(cpld_data->i2c_adapter[portid_count-1]);
        return 0;
}

static struct platform_driver cel_ms200i_lpc_drv = {
        .probe  = cel_ms200i_lpc_drv_probe,
        .remove = __exit_p(cel_ms200i_lpc_drv_remove),
        .driver = {
            .name   = DRIVER_NAME,
        },
};

int cel_ms200i_lpc_init(void)
{
        platform_device_register(&cel_ms200i_lpc_dev);
        platform_driver_register(&cel_ms200i_lpc_drv);

        return 0;
}

void cel_ms200i_lpc_exit(void)
{
        platform_driver_unregister(&cel_ms200i_lpc_drv);
        platform_device_unregister(&cel_ms200i_lpc_dev);
}

module_init(cel_ms200i_lpc_init);
module_exit(cel_ms200i_lpc_exit);

MODULE_AUTHOR("Pariwat Leamsumran  <pleamsum@celestica.com>");
MODULE_DESCRIPTION("Celestica MidStone ms200i LPC Driver");
MODULE_LICENSE("GPL");
