/*******************************************************************************

*  Copyright©[2020-2024] [Hangzhou Clounix Technology Limited]. 

*  All rights reserved.



*  This program is free software: you can redistribute it and/or modify

*  it under the terms of the GNU General Public License as published by

*  the Free Software Foundation, either version 3 of the License, or

*  (at your option) any later version.



*  This program is distributed in the hope that it will be useful,

*  but WITHOUT ANY WARRANTY; without even the implied warranty of

*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

*  GNU General Public License for more details.



*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 

*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 

*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 

*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 

*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 

*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 

*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 

*  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 

*  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 

*  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 

*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/device.h>
#include <linux/delay.h>

#define REGS_NUM 435
#define EEPROM_SIZE 253
#define R159_MEMADR_1 0x009f
#define R160_MEMADR_0 0x00a0
#define R161_NVMDAT 0x00a1
#define R162_RAMDAT 0x00a2
#define R164_NVMUNLK 0x00a4
#define R157 0x009d
#define REG_RAGE 0x1b2

#define XIIC_MSB_OFFSET (0)
#define XIIC_REG_OFFSET (0x100+XIIC_MSB_OFFSET)

#define XIIC_CR_REG_OFFSET   (0x00+XIIC_REG_OFFSET) /* Control Register   */
#define XIIC_SR_REG_OFFSET   (0x04+XIIC_REG_OFFSET) /* Status Register    */
#define XIIC_DTR_REG_OFFSET  (0x08+XIIC_REG_OFFSET) /* Data Tx Register   */
#define XIIC_DRR_REG_OFFSET  (0x0C+XIIC_REG_OFFSET) /* Data Rx Register   */
#define XIIC_ADR_REG_OFFSET  (0x10+XIIC_REG_OFFSET) /* Address Register   */
#define XIIC_TFO_REG_OFFSET  (0x14+XIIC_REG_OFFSET) /* Tx FIFO Occupancy  */
#define XIIC_RFO_REG_OFFSET  (0x18+XIIC_REG_OFFSET) /* Rx FIFO Occupancy  */
#define XIIC_TBA_REG_OFFSET  (0x1C+XIIC_REG_OFFSET) /* 10 Bit Address reg */
#define XIIC_RFD_REG_OFFSET  (0x20+XIIC_REG_OFFSET) /* Rx FIFO Depth reg  */
#define XIIC_GPO_REG_OFFSET  (0x24+XIIC_REG_OFFSET) /* Output Register    */

/* Control Register masks */
#define XIIC_CR_ENABLE_DEVICE_MASK        0x01  /* Device enable = 1      */
#define XIIC_CR_TX_FIFO_RESET_MASK        0x02  /* Transmit FIFO reset=1  */
#define XIIC_CR_MSMS_MASK                 0x04  /* Master starts Txing=1  */
#define XIIC_CR_DIR_IS_TX_MASK            0x08  /* Dir of tx. Txing=1     */
#define XIIC_CR_NO_ACK_MASK               0x10  /* Tx Ack. NO ack = 1     */
#define XIIC_CR_REPEATED_START_MASK       0x20  /* Repeated start = 1     */
#define XIIC_CR_GENERAL_CALL_MASK         0x40  /* Gen Call enabled = 1   */

/* Status Register masks */
#define XIIC_SR_GEN_CALL_MASK             0x01  /* 1=a mstr issued a GC   */
#define XIIC_SR_ADDR_AS_SLAVE_MASK        0x02  /* 1=when addr as slave   */
#define XIIC_SR_BUS_BUSY_MASK             0x04  /* 1 = bus is busy        */
#define XIIC_SR_MSTR_RDING_SLAVE_MASK     0x08  /* 1=Dir: mstr <-- slave  */
#define XIIC_SR_TX_FIFO_FULL_MASK         0x10  /* 1 = Tx FIFO full       */
#define XIIC_SR_RX_FIFO_FULL_MASK         0x20  /* 1 = Rx FIFO full       */
#define XIIC_SR_RX_FIFO_EMPTY_MASK        0x40  /* 1 = Rx FIFO empty      */
#define XIIC_SR_TX_FIFO_EMPTY_MASK        0x80  /* 1 = Tx FIFO empty      */

/* Interrupt Status Register masks    Interrupt occurs when...       */
#define XIIC_INTR_ARB_LOST_MASK           0x01  /* 1 = arbitration lost   */
#define XIIC_INTR_TX_ERROR_MASK           0x02  /* 1=Tx error/msg complete */
#define XIIC_INTR_TX_EMPTY_MASK           0x04  /* 1 = Tx FIFO/reg empty  */
#define XIIC_INTR_RX_FULL_MASK            0x08  /* 1=Rx FIFO/reg=OCY level */
#define XIIC_INTR_BNB_MASK                0x10  /* 1 = Bus not busy       */
#define XIIC_INTR_AAS_MASK                0x20  /* 1 = when addr as slave */
#define XIIC_INTR_NAAS_MASK               0x40  /* 1 = not addr as slave  */
#define XIIC_INTR_TX_HALF_MASK            0x80  /* 1 = TX FIFO half empty */

/* The following constants specify the depth of the FIFOs */
#define IIC_RX_FIFO_DEPTH         16    /* Rx fifo capacity               */
#define IIC_TX_FIFO_DEPTH         16    /* Tx fifo capacity               */

#define XIIC_DGIER_OFFSET    (XIIC_MSB_OFFSET+0x1C) /* Device Global Interrupt Enable Register */
#define XIIC_IISR_OFFSET     (XIIC_MSB_OFFSET+0x20) /* Interrupt Status Register */
#define XIIC_IIER_OFFSET     (XIIC_MSB_OFFSET+0x28) /* Interrupt Enable Register */
#define XIIC_RESETR_OFFSET   (XIIC_MSB_OFFSET+0x40) /* Reset Register */

#define XIIC_RESET_MASK             0xAUL

#define XIIC_PM_TIMEOUT     1000    /* ms */
/* timeout waiting for the controller to respond */
#define XIIC_I2C_TIMEOUT    (msecs_to_jiffies(500))
#define XIIC_I2C_INIT_TIMEOUT    (msecs_to_jiffies(50))

#define XIIC_TX_DYN_START_MASK            0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK             0x0200 /* 1 = Set dynamic stop */

#define XIIC_GINTR_ENABLE_MASK      0x80000000UL

#define CLOUNIX_INIT_TIMEOUT (msecs_to_jiffies(100))


struct master_priv_data {
    struct i2c_adapter *adap;
    struct mutex lock;
    void __iomem *mmio;
};

static int tx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_EMPTY_MASK;
}

static int rx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_EMPTY_MASK;
}

static int wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    while (tx_fifo_empty(priv) == 0) {
        if (time_after(jiffies, timeout)) {
            return 0;
        }
    }

    return 1;
}

static int wait_bus_can_rx(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT;

    while (rx_fifo_empty(priv) != 0) {
        if (time_after(jiffies, timeout))
            return 0;
    }

    return 1;
}

static int bus_busy(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK;
}


static int wait_bus_busy_status(struct master_priv_data *priv, unsigned int status)
{
    unsigned long timeout;

    timeout = jiffies + XIIC_I2C_TIMEOUT;
    while (bus_busy(priv) != status) {
        if (time_after(jiffies, timeout))
            return 0;
    }

    return 1;
}


static int i2c_reinit(struct master_priv_data *priv, unsigned long after)
{
    unsigned long timeout;

    writeb(XIIC_RESET_MASK, priv->mmio + XIIC_RESETR_OFFSET);
    timeout = jiffies +  after;
    while(time_after(jiffies, timeout)) {};

    writeb(IIC_RX_FIFO_DEPTH - 1, priv->mmio + XIIC_RFD_REG_OFFSET);

    writeb(XIIC_CR_ENABLE_DEVICE_MASK , priv->mmio + XIIC_CR_REG_OFFSET);
    writeb(XIIC_CR_ENABLE_DEVICE_MASK | XIIC_CR_TX_FIFO_RESET_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
    timeout = jiffies +  after;
    while(time_after(jiffies, timeout)) {};
    writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);

    timeout = jiffies + after;
    while (rx_fifo_empty(priv) == 0) {
        if (time_after(jiffies, timeout)) {
            return -ETIMEDOUT;
        }
    }
    printk(KERN_ALERT "%s[%d] done\n", __func__, __LINE__);
    return 0;
}

static char priv_read_eeprom_byte(struct i2c_client *client, unsigned short reg_addr,char *data)
{
    struct i2c_adapter *adap = client->adapter;
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    int retry = 0;
    unsigned short addr = client->addr;

    if (priv->mmio == NULL) {
        printk("%s[%d] not support for this platform\n", __func__, __LINE__);
        return -1;
    }

    while (1) {
        if (wait_bus_busy_status(priv, 0) != 0)
            break;
        retry++;
        if (retry > 3)
            return -1;
    }

    addr = (addr & 0x7f) << 1;
    writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
    writew((reg_addr >> 8), priv->mmio + XIIC_DTR_REG_OFFSET);
    writew((reg_addr & 0xff), priv->mmio + XIIC_DTR_REG_OFFSET);
    writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
    writew((0x1 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
    writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);

    if (wait_bus_tx_done(priv) == 0) {
        return -1;
    }

    if (wait_bus_can_rx(priv) == 0) {
        return -1;
    }

    data[0] = readb(priv->mmio + XIIC_DRR_REG_OFFSET);
    return 0;
}

static ssize_t nvm_eeprom_get(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_adapter *adap = client->adapter;
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    int i;
    char data[3];
    
    i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);
    /* set eeprom addr */
    for (i = 0; i < EEPROM_SIZE; i++) {
        data[0] = R159_MEMADR_1 >> 8;
        data[1] = R159_MEMADR_1 & 0xff;
        data[2] = 0;
        if (i2c_master_send(client, data, 3) != 3)
            return -ETIMEDOUT;

        data[0] = R160_MEMADR_0 >> 8;
        data[1] = R160_MEMADR_0 & 0xff;
        data[2] = i;
        if (i2c_master_send(client, data, 3) != 3)
            return -ETIMEDOUT;

        if (priv_read_eeprom_byte(client, R161_NVMDAT,buf) != 0)
            return i;
        buf++;
        /* read eeprom output reg */
    }

    return i;
}

static ssize_t nvm_eeprom_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct i2c_adapter *adap = client->adapter;
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    int i;
    char data[3];

    if (count != EEPROM_SIZE) {
        printk(KERN_ALERT "size not match\n");
        return -1;
    }
    i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);

    for (i = 0; i < EEPROM_SIZE; i++) {
        data[0] = R159_MEMADR_1 >> 8;
        data[1] = R159_MEMADR_1 & 0xff;
        data[2] = 0;
        if (i2c_master_send(client, data, 3) != 3)
            return -ETIMEDOUT;

        data[0] = R160_MEMADR_0 >> 8;
        data[1] = R160_MEMADR_0 & 0xff;
        data[2] = i;
        if (i2c_master_send(client, data, 3) != 3)
            return -ETIMEDOUT;

        data[0] = R162_RAMDAT >> 8;
        data[1] = R162_RAMDAT & 0xff;
        data[2] = *buf;
        if (i2c_master_send(client, data, 3) != 3)
            return i;
        buf++;
    }

    data[0] = R164_NVMUNLK >> 8;
    data[1] = R164_NVMUNLK & 0xff;
    data[2] = 0xea;
    if (i2c_master_send(client, data, 3) != 3)
        return 0;

    data[0] = R157 >> 8;
    data[1] = R157 & 0xff;
    data[2] = 0x3;
    if (i2c_master_send(client, data, 3) != 3)
        return 0;

    mdelay(300);
    i = 0;
    while (i<3) {
        if (priv_read_eeprom_byte(client, R157, data) != 0) {
            i = 3;
            goto out;
        }
        if ((data[0] & 0x4) == 0)
            break;
        mdelay(100);
        i++;
    }

out:
    data[0] = R164_NVMUNLK >> 8;
    data[1] = R164_NVMUNLK & 0xff;
    data[2] = 0x0;
    if (i2c_master_send(client, data, 3) != 3)
        return 0;

    return i == 3 ? 0 : EEPROM_SIZE;
}

DEVICE_ATTR(eeprom, 00644, nvm_eeprom_get, nvm_eeprom_set);

static struct attribute *lmk_attrs[] = {
    &dev_attr_eeprom.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = lmk_attrs,
};

static int lmk_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    return sysfs_create_group(&client->dev.kobj, &attr_group);
}

static void lmk_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &attr_group);
    return;
}
static const struct i2c_device_id lmk_id[] = {
    {"LMK05318B", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, lmk_id);

static const struct of_device_id __maybe_unused lmk_of_match[] = {
    {.compatible = "TI, LMK05318B"},
    {}
};
MODULE_DEVICE_TABLE(of, lmk_of_match);

static struct i2c_driver lmk_driver = {
    .driver = {
        .name = "TI LMK",
        .of_match_table = of_match_ptr(lmk_of_match),
    },
    .probe = lmk_probe,
    .remove = lmk_remove,
    .id_table = lmk_id,
};

module_i2c_driver(lmk_driver);

MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("PMBus driver for LMK05318");
MODULE_LICENSE("GPL");
