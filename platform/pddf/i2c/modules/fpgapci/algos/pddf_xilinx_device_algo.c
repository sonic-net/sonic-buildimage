#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>

#include "pddf_i2c_algo.h"

extern void __iomem * fpga_ctl_addr;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);
extern int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int);
static int loglevel = 0x1;
enum log_level
{
    LOG_ERR = 0x01,
    LOG_WARNING = 0x02,
    LOG_INFO = 0x04,
    LOG_DEBUG = 0x08,
};
#define CLX_PRINTK(level,fmt, ...)                        \
    do                                                     \
    {                                                      \
        if (LOG_##level & loglevel)                        \
            printk(KERN_DEBUG fmt, ##__VA_ARGS__); \
    } while (0)

#define DEFAULT_RETRY 3

#define XIIC_MSB_OFFSET (0)
#define XIIC_REG_OFFSET (0x100+XIIC_MSB_OFFSET)
/*
 * Register offsets in bytes from RegisterBase. Three is added to the
 * base offset to access LSB (IBM style) of the word
 */
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

#define XIIC_TX_DYN_START_MASK            0x0100 /* 1 = Set dynamic start */
#define XIIC_TX_DYN_STOP_MASK             0x0200 /* 1 = Set dynamic stop */

#define XIIC_GINTR_ENABLE_MASK      0x80000000UL

#define CLOUNIX_INIT_TIMEOUT (msecs_to_jiffies(100))

struct master_conf {
    int offset;
    char *name;
};


struct master_priv_data {
    struct i2c_adapter *adap;
    struct mutex lock;
    void __iomem *mmio;
};

static struct master_priv_data group_priv[I2C_PCI_MAX_BUS];

static int tx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_FULL_MASK;
}

static int tx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_TX_FIFO_EMPTY_MASK;
}

static int rx_fifo_full(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_FULL_MASK;
}

static int rx_fifo_empty(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_RX_FIFO_EMPTY_MASK;
}

static int tx_fifo_space(struct master_priv_data *priv)
{
    return IIC_TX_FIFO_DEPTH - readb(priv->mmio + XIIC_TFO_REG_OFFSET) - 1;
}

static int bus_busy(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_SR_REG_OFFSET) & XIIC_SR_BUS_BUSY_MASK;
}

static void dump_reg(struct master_priv_data *priv)
{
    int i;

    CLX_PRINTK(INFO, "%x\r\n", readl(priv->mmio + 0x1c));
    CLX_PRINTK(INFO, "%x\r\n", readl(priv->mmio + 0x20));
    CLX_PRINTK(INFO, "%x\r\n", readl(priv->mmio + 0x28));
    
    i = XIIC_CR_REG_OFFSET;
    while (i<=XIIC_RFD_REG_OFFSET) {
        CLX_PRINTK(INFO, "off %x: %x\r\n", i, readl(priv->mmio + i));
        i+=4;
    }
}

static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned long after)
{
    unsigned long timeout;
    
    dump_reg(priv);
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
            CLX_PRINTK(ERR, "%s reinit timeout\r\n", priv->adap->name);
            return -ETIMEDOUT;
        }
    }
    
    dump_reg(priv);
    return 0;
}

static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static int wait_bus_busy_status(struct master_priv_data *priv, unsigned int status)
{
    unsigned long timeout;
    
    timeout = jiffies + XIIC_I2C_TIMEOUT; 
    while (bus_busy(priv) != status) {
        if (time_after(jiffies, timeout)) {
            CLX_PRINTK(DEBUG, "bus status err %x\r\n", status);
            return 0;
        }
    }

    return 1;
}

static int wait_bus_tx_done(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (tx_fifo_empty(priv) == 0) {
        if (time_after(jiffies, timeout)) {
            CLX_PRINTK(DEBUG, "tx fifo not empty\r\n");
            return 0;
        }
    }

    return 1;
}

static int wait_bus_can_tx(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (tx_fifo_full(priv) != 0) {
        if (time_after(jiffies, timeout)) {
            CLX_PRINTK(DEBUG,  "tx fifo full\r\n");
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

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned char addr;
    int i, j;
    unsigned long timeout;

    p = &msgs[0];
    if((0x50 <= p->addr) && (p->addr <= 0x57) && ((p->flags & I2C_M_RD)==0))
    {
        usleep_range(6000, 10000);
    }

    mutex_lock(&priv->lock);
    writeb(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
    if (wait_bus_busy_status(priv, 0) == 0)
       goto out;  
    if (wait_bus_tx_done(priv) == 0)
        goto out;

    for (i=0; i<num; i++) {
        p = &msgs[i];
        if (p->flags & I2C_M_TEN)
            goto out;
        addr = i2c_8bit_addr_from_msg(p);
        
        writew(addr | XIIC_TX_DYN_START_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
        if (p->flags & I2C_M_RD) {
            writew((p->len | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
        }
        j = 0;
        if (p->flags & I2C_M_RD) {
            if (wait_bus_tx_done(priv) == 0)
                goto out;
            timeout = jiffies + XIIC_I2C_TIMEOUT;
            while (j < p->len) {
                if (rx_fifo_empty(priv) != 0) {
                    if (time_after(jiffies, timeout)) {
                        dump_reg(priv);
                        CLX_PRINTK(DEBUG,"rx timeout\r\n");
                        goto out;
                    }
                    continue;
                }
                timeout = jiffies + XIIC_I2C_TIMEOUT;
                p->buf[j] = readb(priv->mmio + XIIC_DRR_REG_OFFSET);
                j++;
            }
        } else {
            timeout = jiffies + XIIC_I2C_TIMEOUT; 
            while (j < p->len) {
                if (tx_fifo_full(priv) != 0) {
                    if (time_after(jiffies, timeout)) {
                        CLX_PRINTK(DEBUG,"tx fifo full\r\n");
                        goto out;
                    }

                    continue;
                }
               
                timeout = jiffies + XIIC_I2C_TIMEOUT; 
                if ((j == p->len - 1)&&(i == num-1)) {
                    writew(p->buf[j] | XIIC_TX_DYN_STOP_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
                } else {
                    writew(p->buf[j], priv->mmio + XIIC_DTR_REG_OFFSET);
                }

                j++;
            }
        
        }
    }
    if (wait_bus_busy_status(priv, 0) == 0)
        goto out;

    writeb((XIIC_CR_TX_FIFO_RESET_MASK | XIIC_CR_ENABLE_DEVICE_MASK), priv->mmio + XIIC_CR_REG_OFFSET);
    writeb(0, priv->mmio + XIIC_CR_REG_OFFSET);
    mutex_unlock(&priv->lock);

    return num;

out:
    fpga_i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);
    mutex_unlock(&priv->lock);
    
    return -ETIMEDOUT;
}

static int repeated_start_status(struct master_priv_data *priv)
{
    return readb(priv->mmio + XIIC_CR_REG_OFFSET) & XIIC_CR_REPEATED_START_MASK;
}

int wait_repeated_start_done(struct master_priv_data *priv)
{
    unsigned long timeout = jiffies + XIIC_I2C_TIMEOUT; 

    while (repeated_start_status(priv) != 0) {
        if (time_after(jiffies, timeout))
            return 0;
    }
    
    return 1;
}

#define DO_RX_B(priv, data) \
    if (wait_bus_can_rx(priv) == 0) { \
        dump_reg(priv); \
        goto out; \
    } \
    data = readb(priv->mmio + XIIC_DRR_REG_OFFSET)

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, 
                       char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    unsigned char tmp;
    unsigned char i;
    
    if (wait_bus_busy_status(priv, 0) == 0)
        return -EBUSY;

    mutex_lock(&priv->lock); 
    switch (size) {
        case I2C_SMBUS_BYTE:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | read_write), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((0x1 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);

                if (wait_bus_tx_done(priv) == 0)
                    goto out;
                
                DO_RX_B(priv, data->byte);
            } else {
                writew((command | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_BYTE_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew((command), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((0x1 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                
                if (wait_bus_tx_done(priv) == 0)
                    goto out;
                
                DO_RX_B(priv, data->byte);
            } else {
                writew((data->byte | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew((command), priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((0x2 | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                if (wait_bus_tx_done(priv) == 0)
                    goto out;

                DO_RX_B(priv, data->word);
                
                DO_RX_B(priv, tmp);
                data->word += tmp*256;
            } else {
                writew((data->word & 0xff), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew((((data->word & 0xff00) >> 8) | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            }
            break;

        case I2C_SMBUS_BLOCK_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew(command, priv->mmio + XIIC_DTR_REG_OFFSET);
            
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(((I2C_SMBUS_BLOCK_MAX+1) | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            
                if (wait_bus_tx_done(priv) == 0)
                    goto out;

                DO_RX_B(priv, data->block[0]);
                if (data->block[0] > I2C_SMBUS_BLOCK_MAX)
                    goto out;

                for (i=1; i<=data->block[0]; i++) {
                    DO_RX_B(priv, data->block[i]);
                }
                
                if (data->block[0] < I2C_SMBUS_BLOCK_MAX) {
                    for (i=i; i<=I2C_SMBUS_BLOCK_MAX; i++) {
                        DO_RX_B(priv, tmp);
                    }
                }
            } else {
                writew(data->block[0], priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                for (i=1; i<=data->block[0]; i++) {
                    if (wait_bus_can_tx(priv) == 0)
                        goto out;
                   
                    if (i == data->block[0])
                        writew(data->block[i] | XIIC_TX_DYN_STOP_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
                    else
                        writew(data->block[i], priv->mmio + XIIC_DTR_REG_OFFSET);
                }
            }
            break;
        case I2C_SMBUS_I2C_BLOCK_DATA:
            addr = (addr & 0x7f) << 1;
            writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_WRITE), priv->mmio + XIIC_DTR_REG_OFFSET);
            writew(command, priv->mmio + XIIC_DTR_REG_OFFSET);
            if (read_write == I2C_SMBUS_READ) {
                writew((addr | XIIC_TX_DYN_START_MASK | I2C_SMBUS_READ), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(((data->block[0]) | XIIC_TX_DYN_STOP_MASK), priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
            
                if (wait_bus_tx_done(priv) == 0)
                    goto out;

                for (i=1; i<= data->block[0]; i++) {
                    DO_RX_B(priv, data->block[i]);
                }
            } else {
                writew(data->block[0], priv->mmio + XIIC_DTR_REG_OFFSET);
                writew(XIIC_CR_ENABLE_DEVICE_MASK, priv->mmio + XIIC_CR_REG_OFFSET);
                for (i=1; i<=data->block[0]; i++) {
                    if (wait_bus_can_tx(priv) == 0)
                        goto out;
                   
                    if (i == data->block[0])
                        writew(data->block[i] | XIIC_TX_DYN_STOP_MASK, priv->mmio + XIIC_DTR_REG_OFFSET);
                    else
                        writew(data->block[i], priv->mmio + XIIC_DTR_REG_OFFSET);
                }
            }
            break;
        default:
            break;
    }

    if (wait_bus_busy_status(priv, 0) == 0)
        goto out; 

    writeb((XIIC_CR_TX_FIFO_RESET_MASK | XIIC_CR_ENABLE_DEVICE_MASK), priv->mmio + XIIC_CR_REG_OFFSET);
    writew(0, priv->mmio + XIIC_CR_REG_OFFSET);
    mutex_unlock(&priv->lock);
    return 0;

out:
    fpga_i2c_reinit(priv, CLOUNIX_INIT_TIMEOUT);
    mutex_unlock(&priv->lock);
    return -ETIMEDOUT;
}

static struct i2c_algorithm clounix_i2c_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer,
    .master_xfer = clounix_i2c_xfer,
    .functionality = clounix_i2c_func,
};


static int adap_data_init(struct i2c_adapter *adap, int i2c_ch_index)
{
    struct fpgapci_devdata *pci_privdata = 0;
    int err = 0;
    
    pci_privdata = (struct fpgapci_devdata*) dev_get_drvdata(adap->dev.parent);
    if (pci_privdata == 0) {
        printk("[%s]: ERROR pci_privdata is 0\n", __FUNCTION__);
        return -1;
    }

    CLX_PRINTK(INFO, "[%s] index: [%d] fpga_data__base_addr:0x%0x8lx"
        " fpgapci_bar_len:0x%08lx fpga_i2c_ch_base_addr:0x%08lx ch_size=0x%x supported_i2c_ch=%d",
             __FUNCTION__, i2c_ch_index, pci_privdata->fpga_data_base_addr,
            pci_privdata->bar_length, pci_privdata->fpga_i2c_ch_base_addr,
            pci_privdata->fpga_i2c_ch_size, pci_privdata->max_fpga_i2c_ch);

    if (i2c_ch_index >= pci_privdata->max_fpga_i2c_ch || pci_privdata->max_fpga_i2c_ch > I2C_PCI_MAX_BUS) {
        printk("[%s]: ERROR i2c_ch_index=%d max_ch_index=%d out of range: %d\n",
             __FUNCTION__, i2c_ch_index, pci_privdata->max_fpga_i2c_ch, I2C_PCI_MAX_BUS);
        return -1;
    }
    memset(&group_priv[i2c_ch_index], 0, sizeof(group_priv[0]));
    group_priv[i2c_ch_index].adap = adap;
    adap->owner = THIS_MODULE;
    adap->algo = &clounix_i2c_algo;
    adap->retries = DEFAULT_RETRY;    
    group_priv[i2c_ch_index].mmio =  pci_privdata->fpga_i2c_ch_base_addr +
                          i2c_ch_index* pci_privdata->fpga_i2c_ch_size;
    mutex_init(&group_priv[i2c_ch_index].lock);
    i2c_set_adapdata(adap, &group_priv[i2c_ch_index]);
    err = fpga_i2c_reinit(&group_priv[i2c_ch_index], XIIC_I2C_TIMEOUT);
    if (err != 0)
    {
        return err;
    }     
    return 0;
}
static int pddf_i2c_pci_add_numbered_bus_default (struct i2c_adapter *adap, int i2c_ch_index)
{
    int ret = 0;

    adap_data_init(adap, i2c_ch_index);
    adap->algo = &clounix_i2c_algo;

    ret = i2c_add_numbered_adapter(adap);
    return ret;
}
/*
 * FPGAPCI APIs
 */
int board_i2c_fpgapci_read(uint32_t offset)
{
	int data = 0;

    if(NULL != fpga_ctl_addr){
	    data=ioread32(fpga_ctl_addr+offset);
    }
	return data;
}


int board_i2c_fpgapci_write(uint32_t offset, uint32_t value)
{
    if(NULL != fpga_ctl_addr){
	    iowrite32(value, fpga_ctl_addr+offset);
    }    
	return (0);
}
static int __init pddf_xilinx_device_7021_algo_init(void)
{
    CLX_PRINTK(INFO,"[%s]\n", __FUNCTION__);
    pddf_i2c_pci_add_numbered_bus = pddf_i2c_pci_add_numbered_bus_default;
    ptr_fpgapci_read = board_i2c_fpgapci_read;
    ptr_fpgapci_write = board_i2c_fpgapci_write;
    return 0;
}

static void __exit pddf_xilinx_device_7021_algo_exit(void)
{
    CLX_PRINTK(INFO, "[%s]\n", __FUNCTION__);

    pddf_i2c_pci_add_numbered_bus = NULL;
    ptr_fpgapci_read = NULL;
    ptr_fpgapci_write = NULL;
    return;
}


module_init (pddf_xilinx_device_7021_algo_init);
module_exit (pddf_xilinx_device_7021_algo_exit);

module_param(loglevel, uint, S_IRUGO|S_IWUSR);

MODULE_DESCRIPTION("Xilinx Corporation Device 7021 FPGAPCIe I2C-Bus algorithm");
MODULE_LICENSE("GPL");
