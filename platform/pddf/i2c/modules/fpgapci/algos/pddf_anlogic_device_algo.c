#include <linux/types.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/msi.h>
#include <linux/bits.h>
#include <linux/i2c.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/i2c-smbus.h>

#include "pddf_i2c_algo.h"

extern void __iomem * fpga_ctl_addr;
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);
extern int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int);


#define DEFAULT_RETRY 3

#define FPGA_I2C_MGR_CFG (0x00)
#define FPGA_I2C_MGR_CTRL (0x04)
#define FPGA_I2C_MGR_STAT (0x08)
#define FPGA_I2C_MGR_TX (0x10)
#define FPGA_I2C_MGR_RX (0x40)
#define FPGA_I2C_MGR_MUX (0x70)

#define I2C_CFG_RST (GENMASK(31, 31))
#define I2C_CFG_ENABLE (GENMASK(30, 30))
#define I2C_CFG_ABORT (GENMASK(29, 29))
#define I2C_CFG_STAT_CLR (GENMASK(28, 28))
#define I2C_CFG_START (GENMASK(24, 24))
#define I2C_CFG_BSP (GENMASK(15, 14))
#define I2C_CFG_ACK_POL (GENMASK(13, 13))
#define I2C_CFG_CLK_DIV (GENMASK(10, 0))

#define I2C_STAT_BUS_BUSY (GENMASK(31, 31))
#define I2C_STAT_BUS_ABORT (GENMASK(30, 30))
#define I2C_STAT_BUS_ERR (GENMASK(23, 16))

#define I2C_RX_BYTE(byte) ((byte) << 0)
#define I2C_TX_BYTE(byte) ((byte) << 8)
#define I2C_SLAVE_ADDR(addr) ((addr) << 17)

#define CLOUNIX_I2C_TIMEOUT (msecs_to_jiffies(100))

#define MAX_DATA_LEN (32)

/* for fpga msi irq */
#define mgr_irq_ctl (0x700)
#define mgr_irq_ctl_4 (0x710)
#define mgr_irq_stat_4 (0x720)

#define SMBUS_ALERT_ENABLE_MASK  (0x1)
#define PLL_ALERT_MASK (0x1)
#define TEMP_ALERT_MASK (0x2)
#define PMBUS_ALERT_MASK (0x4)

#define MSI_SMBUS_ALERT_IRQ (0x0)
#define SMBUS_ALERT_ARA (0x0c)

static struct mutex mux_lock;

struct master_priv_data {
    struct i2c_adapter *adap;
    int mux;
    void __iomem *mmio;
};
static struct master_priv_data group_priv[I2C_PCI_MAX_BUS];
static void force_delay(unsigned int us)
{
    unsigned long time_out;

    time_out = jiffies + usecs_to_jiffies(us);
    while (time_after(jiffies, time_out) == 0) {};

    return;
}

static int fpga_i2c_reinit(struct master_priv_data *priv, unsigned int after_us)
{

    writel(I2C_CFG_RST | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

    force_delay(after_us);

    writel(I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

    return 0;
}

static int wait_busy_ide(struct master_priv_data *priv)
{
    unsigned long time_out;

    time_out = jiffies + (msecs_to_jiffies(1)/10);
    while (time_after(jiffies, time_out) == 0) {};

    time_out = jiffies + CLOUNIX_I2C_TIMEOUT;
    while ((readl(priv->mmio + FPGA_I2C_MGR_STAT) & I2C_STAT_BUS_BUSY) != 0) {
        if (time_after(jiffies, time_out) == 1) {
            fpga_i2c_reinit(priv, 15);
            return -1;
        }
    }

    return 0;
}

static int clounix_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    struct i2c_msg *p;
    unsigned int ctrl_data;
    int i, j, k;
    unsigned int rd_data;
    unsigned int tmp_buf[8 + 1] = {0};

    for (k=0; k<num; k++) {
        p = &msgs[k];
        if (p->flags & I2C_M_TEN)
            goto out;
        if (p->len > (I2C_SMBUS_BLOCK_MAX + 2))
            goto out;

        if (wait_busy_ide(priv) != 0)
            return -EBUSY;

        writel(priv->mux, priv->mmio + FPGA_I2C_MGR_MUX);

        ctrl_data = I2C_SLAVE_ADDR(p->addr);
        if (p->flags & I2C_M_RD) {
            ctrl_data = ctrl_data | I2C_RX_BYTE(p->len);
            writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
            writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);

            if (wait_busy_ide(priv) != 0)
                goto out;

            for (i=0; i<p->len; ) {
                rd_data = readl(priv->mmio + FPGA_I2C_MGR_RX);
                for (j=0; j<sizeof(int); j++) {
                    if (p->len > (i+j)) {
                        p->buf[p->len-(i+j) - 1] = (rd_data >> (j*8)) & 0xff;
                    } else {
                        break;
                    }
                }
                i += j;
            }
        } else {
            ctrl_data = ctrl_data | I2C_TX_BYTE(p->len);
            writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);

            i = 0;
            while (i<p->len) {
                tmp_buf[i/sizeof(int)] |= p->buf[i] << (24-8*(i%sizeof(int)));
                i++;
            }

            for (i=i/4; i>=0; i--) {
                writel(tmp_buf[i], priv->mmio + FPGA_I2C_MGR_TX);
            }

            writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
        }
    }

    return num;

out:
    return -EIO;
}

static int clounix_i2c_smbus_xfer(struct i2c_adapter *adap, unsigned short addr, unsigned short flags, char read_write, unsigned char command, int size, union i2c_smbus_data *data)
{
    struct master_priv_data *priv = i2c_get_adapdata(adap);
    unsigned int ctrl_data;
    int i, j;
    unsigned int rd_data;
    unsigned int wt_data;
    unsigned char tmp_buf[I2C_SMBUS_BLOCK_MAX + 1] = {0};

    if (wait_busy_ide(priv) != 0)
        return -EBUSY;

    writel(priv->mux, priv->mmio + FPGA_I2C_MGR_MUX);
    switch(size) {
        case I2C_SMBUS_BYTE:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_RX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;

                data->byte = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xff;
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_BYTE_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;
                
                data->byte = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xff;
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(2);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel((command << 24) | (data->byte << 16), priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_WORD_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(2);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(command << 24, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);
                if (wait_busy_ide(priv) != 0)
                    goto err_out;
                
                data->word = readl(priv->mmio + FPGA_I2C_MGR_RX) & 0xffff;
                data->word = ntohs(data->word);
            } else {
                ctrl_data = ctrl_data | I2C_TX_BYTE(3);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                wt_data = command << 24;
                wt_data |= (data->word & 0xff) << 16;
                wt_data |= ((data->word >> 8 ) & 0xff) << 8;
                writel(wt_data, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
        case I2C_SMBUS_BLOCK_DATA:
            ctrl_data = I2C_SLAVE_ADDR(addr);
            if (read_write == I2C_SMBUS_READ) {
                ctrl_data = ctrl_data | I2C_TX_BYTE(1) | I2C_RX_BYTE(I2C_SMBUS_BLOCK_MAX+1);
                writel(ctrl_data, priv->mmio + FPGA_I2C_MGR_CTRL);
                writel(command << 24, priv->mmio + FPGA_I2C_MGR_TX);
                writel(I2C_CFG_START | I2C_CFG_ENABLE | I2C_CFG_ACK_POL, priv->mmio + FPGA_I2C_MGR_CFG);

                if (wait_busy_ide(priv) != 0)
                    return -EIO;
              
                for (i=0; i<I2C_SMBUS_BLOCK_MAX+1; ) {
                    rd_data = readl(priv->mmio + FPGA_I2C_MGR_RX);
                    for (j=0; j<sizeof(int); j++) {
                        if (I2C_SMBUS_BLOCK_MAX >= (i+j)) {
                            tmp_buf[I2C_SMBUS_BLOCK_MAX - (i+j)] = (rd_data >> (j*8)) & 0xff;
                        }
                    }
                    i += j;
                }

                for (i=0; i<=I2C_SMBUS_BLOCK_MAX; i++) {
                    if (tmp_buf[i] != 0 && tmp_buf[i] <= I2C_SMBUS_BLOCK_MAX) {
                        memcpy(data->block, &tmp_buf[i], tmp_buf[i]+1);
                        return 0;
                    }
                }
                goto err_out;
            } else {
                i = 0;
                while (i <= data->block[0]) {
                    tmp_buf[i/sizeof(int)] |= data->block[i] << (24-8*(i%sizeof(int)));
                    i++;
                }

                for (i=i/4; i>=0; i--) {
                    writel(tmp_buf[i], priv->mmio + FPGA_I2C_MGR_TX);
                }
                writel(I2C_CFG_START | I2C_CFG_ENABLE, priv->mmio + FPGA_I2C_MGR_CFG);
            }
            break;
    }

    return 0;

err_out:
    return -EIO;
}

static u32 clounix_i2c_func(struct i2c_adapter *a)
{
    return ((I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL) & (~I2C_FUNC_SMBUS_QUICK)) | I2C_FUNC_SMBUS_BLOCK_DATA;
}

static struct i2c_algorithm clounix_i2c_algo = {
    .smbus_xfer = clounix_i2c_smbus_xfer,
    .master_xfer = clounix_i2c_xfer,
    .functionality = clounix_i2c_func,
};


void lock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    mutex_lock(&mux_lock);
}

int trylock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    return -EPERM;
}

void unlock_fpga_i2c_bus(struct i2c_adapter *adap, unsigned int flags)
{
    mutex_unlock(&mux_lock);
}

static struct i2c_lock_operations lock_ops = {
    .lock_bus = lock_fpga_i2c_bus,
    .trylock_bus = NULL,
    .unlock_bus = unlock_fpga_i2c_bus,
};

int adap_data_init(struct i2c_adapter *adap, int i2c_ch_index)
{
    struct pci_dev *pdev = pci_get_device(0x16c3, 0xabcd, NULL);
    struct fpgapci_devdata *pci_privdata = NULL;
    int err;

    if (pdev == NULL) {
        return -ENXIO;
    }
    pci_privdata = (struct fpgapci_devdata*) dev_get_drvdata(adap->dev.parent);
    if (pci_privdata == 0) {
        printk("[%s]: ERROR pci_privdata is 0\n", __FUNCTION__);
        return -1;
    }
#ifdef DEBUG
    pddf_dbg(FPGA, KERN_INFO "[%s] index: [%d] fpga_data__base_addr:0x%0x8lx"
        " fpgapci_bar_len:0x%08lx fpga_i2c_ch_base_addr:0x%08lx ch_size=0x%x supported_i2c_ch=%d",
             __FUNCTION__, i2c_ch_index, pci_privdata->fpga_data_base_addr,
            pci_privdata->bar_length, pci_privdata->fpga_i2c_ch_base_addr,
            pci_privdata->fpga_i2c_ch_size, pci_privdata->max_fpga_i2c_ch);
#endif
    if (i2c_ch_index >= pci_privdata->max_fpga_i2c_ch || pci_privdata->max_fpga_i2c_ch > I2C_PCI_MAX_BUS) {
        printk("[%s]: ERROR i2c_ch_index=%d max_ch_index=%d out of range: %d\n",
             __FUNCTION__, i2c_ch_index, pci_privdata->max_fpga_i2c_ch, I2C_PCI_MAX_BUS);
        return -1;
    }
    mutex_init(&mux_lock);

    memset(&group_priv[i2c_ch_index], 0, sizeof(group_priv[0]));
    group_priv[i2c_ch_index].adap = adap;
    adap->owner = THIS_MODULE;
    adap->lock_ops = &lock_ops;
    adap->algo = &clounix_i2c_algo;
    adap->retries = DEFAULT_RETRY;
    adap->dev.parent = &pdev->dev;
    adap->dev.of_node = pdev->dev.of_node;
    adap->timeout = CLOUNIX_I2C_TIMEOUT;
    adap->retries = 2;
    group_priv[i2c_ch_index].mmio = pci_privdata->fpga_i2c_ch_base_addr +
                          i2c_ch_index* pci_privdata->fpga_i2c_ch_size;;
    group_priv[i2c_ch_index].mux = i2c_ch_index;

   
    i2c_set_adapdata(adap, &group_priv[i2c_ch_index]);

    err =  fpga_i2c_reinit(&group_priv[i2c_ch_index], 15);
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
	int data;
	data=ioread32(fpga_ctl_addr+offset);
	return data;
}


int board_i2c_fpgapci_write(uint32_t offset, uint32_t value)
{
	iowrite32(value, fpga_ctl_addr+offset);
	return (0);
}
static int __init pddf_anlogic_device_algo_init(void)
{  

    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);
 
    pddf_i2c_pci_add_numbered_bus = pddf_i2c_pci_add_numbered_bus_default;
    ptr_fpgapci_read = board_i2c_fpgapci_read;
    ptr_fpgapci_write = board_i2c_fpgapci_write;
    return 0;
}

static void __exit pddf_anlogic_device_algo_exit(void)
{
    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);
    pddf_i2c_pci_add_numbered_bus = NULL;
    ptr_fpgapci_read = NULL;
    ptr_fpgapci_write = NULL;
    return;
}


module_init (pddf_anlogic_device_algo_init);
module_exit (pddf_anlogic_device_algo_exit);
MODULE_DESCRIPTION("Anlogic Corporation Device 0xABCD  FPGAPCIe I2C-Bus algorithm");
MODULE_LICENSE("GPL");