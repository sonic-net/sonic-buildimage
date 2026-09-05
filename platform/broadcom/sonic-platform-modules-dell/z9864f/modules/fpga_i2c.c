#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/i2c.h>
#include "fpga.h"
#include "fpga_attr.h"
#include "fpga_gpio.h"
#include "fpga_i2c.h"

int num_i2c_adapter;

fpga_i2c_s fpga_i2c_info[] = {
    {"FPGA SMBUS - PLD"         , 0, DELTA_I2C_BASE(0), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - MON"         , 1, DELTA_I2C_BASE(1), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_0"      , 2, DELTA_I2C_BASE(2), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_1"      , 3, DELTA_I2C_BASE(3), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_2"      , 4, DELTA_I2C_BASE(4), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_3"      , 5, DELTA_I2C_BASE(5), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_4"      , 6, DELTA_I2C_BASE(6), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_5"      , 7, DELTA_I2C_BASE(7), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_6"      , 8, DELTA_I2C_BASE(8), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - PORT_7"      , 9, DELTA_I2C_BASE(9), FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - 1588 DPLL"   , 10, DELTA_DPLL_I2C_BASE, FPGA_I2C_MUX_DIS, 0x00, 0},
    {"FPGA SMBUS - 1588 FPGA"   , 11, DELTA_FPGA_I2C_BASE, FPGA_I2C_MUX_DIS, 0x00, 0},
};

static int delta_wait_i2c_complete(struct i2c_bus_dev *i2c);
static void delta_fpga_i2c_data_reg_set(struct i2c_bus_dev *i2c, int addr, int data);
static void delta_fpga_i2c_addr_reg_set(struct i2c_bus_dev *i2c, int data);
static void delta_fpga_i2c_ctrl_set(struct i2c_bus_dev *i2c, int data);
static int delta_fpga_i2c_ctrl_get(struct i2c_bus_dev *i2c);

static int dni_fpga_i2c_read(struct i2c_bus_dev *i2c, int addr, int raddr, int rsize, uint8_t *readout, int dsize);
static int dni_fpga_i2c_write(struct i2c_bus_dev *i2c, int addr, int raddr, int rsize, uint8_t *data, int dsize);
static int io_read(struct i2c_bus_dev *i2c, int offset);
static void io_write(struct i2c_bus_dev *i2c, int offset, int data);

struct mutex fpga_i2c_lock;

static s32 dni_fpga_i2c_access(struct i2c_adapter *adap, u16 addr,
                               unsigned short flags, char read_write, u8 command, int size,
                               union i2c_smbus_data *data)
{
    struct i2c_bus_dev *i2c = adap->algo_data;
    int len;
    uint8_t i2c_data;
    int rv = 0;

    mutex_lock(&fpga_i2c_lock);
    //	printk(KERN_INFO "size=%d, read_write=%d addr=0x%x command=0x%x fpga_bus=%d \n",size,read_write,addr,command,i2c->busnum);
    switch (size)
    {

    case I2C_SMBUS_QUICK:
        //		printk(KERN_INFO "I2C_SMBUS_QUICK");
        rv = dni_fpga_i2c_write(i2c, addr, command, 0, &i2c_data, 0);
        goto done;
        break;
    case I2C_SMBUS_BYTE:
        //		printk(KERN_INFO "I2C_SMBUS_BYTE");
        if (read_write == I2C_SMBUS_WRITE)
        {
            rv = dni_fpga_i2c_write(i2c, addr, command, 1, &i2c_data, 0);
            goto done;
        }
        else
        {
            rv = dni_fpga_i2c_read(i2c, addr, command, 1, &data->byte, 1);
            goto done;
        }

        break;
    case I2C_SMBUS_BYTE_DATA:
        if (read_write == I2C_SMBUS_WRITE)
        {
            rv = dni_fpga_i2c_write(i2c, addr, command, 1, &data->byte, 1);
            goto done;
        }
        else
        {
            rv = dni_fpga_i2c_read(i2c, addr, command, 1, &data->byte, 1);
            goto done;
        }
        break;
    case I2C_SMBUS_WORD_DATA:
        if (read_write == I2C_SMBUS_WRITE)
        {
            /* TODO: not verify */
            rv = dni_fpga_i2c_write(i2c, addr, command, 1, (uint8_t *)&data->word, 2);
            //			printk(KERN_INFO "I2C_FUNC_SMBUS_WORD_DATA write");
            goto done;
        }
        else
        {
            rv = dni_fpga_i2c_read(i2c, addr, command, 1, (uint8_t *)&data->word, 2);
            goto done;
        }
        break;

    case I2C_SMBUS_BLOCK_DATA:
        if (read_write == I2C_SMBUS_WRITE)
        {

            len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
            rv = dni_fpga_i2c_write(i2c, addr, command, 1, &data->block[0], len + 1);
            goto done;
        }
        else
        {
            //			dni_fpga_i2c_read(i2c, addr, command, 1, &data->byte, 1);
            //			len = min_t(u8, data->byte, I2C_SMBUS_BLOCK_MAX);
            len = I2C_SMBUS_BLOCK_MAX;
            rv = dni_fpga_i2c_read(i2c, addr, command, 1, &data->block[0], len + 1);
            goto done;
        }
        break;
    case I2C_SMBUS_I2C_BLOCK_DATA:
        if (read_write == I2C_SMBUS_WRITE)
        {
            len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
            rv = dni_fpga_i2c_write(i2c, addr, command, 1, &data->block[1], len);
            goto done;
        }
        else
        {
            len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
            rv = dni_fpga_i2c_read(i2c, addr, command, 1, &data->block[1], len);
            goto done;
        }
        break;
    case I2C_SMBUS_PROC_CALL:

        break;
    case I2C_SMBUS_BLOCK_PROC_CALL:
        break;
    }

    mutex_unlock(&fpga_i2c_lock);
    return -1;
done:
    mutex_unlock(&fpga_i2c_lock);
    return rv;

}

static int delta_wait_i2c_complete(struct i2c_bus_dev *i2c)
{
    unsigned long timeout = 0;
    uint64_t status;

    while ((status = delta_fpga_i2c_ctrl_get(i2c)) & I2C_TRANS_ENABLE)
    {
        if (timeout > DELTA_I2C_WAIT_BUS_TIMEOUT)
        {
            printk(KERN_INFO "i2c wait for complete timeout: time=%lu us status=0x%llx", timeout, status);
            return -ETIMEDOUT;
        }
        udelay(100);
        timeout += 100;
    }

    return 0;
}

static void delta_fpga_i2c_data_reg_set(struct i2c_bus_dev *i2c, int addr, int data)
{

    uint64_t hi_cmd, lo_cmd, i2c_cmd;
    int offset;
    hi_cmd = 0;
    lo_cmd = data;
    i2c_cmd = (hi_cmd << 32) | lo_cmd;
    offset = DELTA_I2C_DATA(i2c->offset) + addr;
    io_write(i2c, offset, i2c_cmd);
}

static void delta_fpga_i2c_addr_reg_set(struct i2c_bus_dev *i2c, int data)
{
    uint64_t hi_cmd, lo_cmd, i2c_cmd;
    int offset;
    hi_cmd = 0;
    lo_cmd = data;
    i2c_cmd = (hi_cmd << 32) | lo_cmd;
    offset = DELTA_I2C_ADDR(i2c->offset);
    io_write(i2c, offset, i2c_cmd);
}
#ifdef FPGA_PCA9548
static void delta_fpga_i2c_conf_reg_set(struct i2c_bus_dev *i2c, int data)
{
    uint64_t hi_cmd, lo_cmd, i2c_cmd;
    int offset;
    if (ch == 2)
    {
        hi_cmd = 0;
        lo_cmd = (data << 25) | 0x5A; //100Khz
        i2c_cmd = (hi_cmd << 32) | lo_cmd;
        offset = DELTA_I2C_CONF(i2c->offset);
        io_write(i2c, offset, i2c_cmd);
    }
}
#endif
static void delta_fpga_i2c_ctrl_set(struct i2c_bus_dev *i2c, int data)
{
    uint64_t hi_cmd, lo_cmd, i2c_cmd;
    int offset;
    hi_cmd = 0;
    lo_cmd = data;
    i2c_cmd = (hi_cmd << 32) | lo_cmd;
    offset = DELTA_I2C_CTRL(i2c->offset);
    io_write(i2c, offset, i2c_cmd);
}

static int delta_fpga_i2c_ctrl_get(struct i2c_bus_dev *i2c)
{
    int offset;
    offset = DELTA_I2C_CTRL(i2c->offset);
    return io_read(i2c, offset);
}


static int dni_fpga_i2c_write(struct i2c_bus_dev *i2c, int addr, int raddr, int rsize, uint8_t *data, int dsize)
{
    int status;
    uint32_t ctrl_data, addr_data, rw_data;

    int rv = -1;
    int offset, i;
    int four_byte, one_byte;

    four_byte = dsize / 4;
    one_byte = dsize % 4;

    if (i2c->mux_en == FPGA_I2C_MUX_EN)
    {
        if (addr < 0x50 || addr > 0x58)
            goto fail;
    }

    for (i = 0; i < four_byte; i++)
    {
        offset = i * 4;
        rw_data = (data[i * 4 + 3] << 24) | (data[i * 4 + 2] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 0];
        delta_fpga_i2c_data_reg_set(i2c, offset, rw_data);
    }
    //	printk(KERN_INFO "four_byte : rw_data = 0x%lx",rw_data);
    rw_data = 0;
    for (i = 0; i < one_byte; i++)
    {
        rw_data |= (data[four_byte * 4 + i] << (i * 8));
    }
    offset = four_byte * 4;
    delta_fpga_i2c_data_reg_set(i2c, offset, rw_data);

    //	printk(KERN_INFO "one_byte : rw_data = 0x%lx",rw_data);

    /* Set address register */
    if (rsize == 0)
        addr_data = 0;
    else if (rsize == 1)
    {
        addr = addr + (raddr / 0x100);
        addr_data = (raddr & 0xff);
    }
    else if (rsize == 2)
        addr_data = (raddr & 0xffff);
    delta_fpga_i2c_addr_reg_set(i2c, addr_data);
#ifdef FPGA_PCA9548
    delta_fpga_i2c_conf_reg_set(i2c, ch, 0x70);
#endif
    ctrl_data = 0;
    /* Set ctrl reg */
    ctrl_data |= ((addr & 0x7f) << DELTA_FPGA_I2C_SLAVE_OFFSET);
    ctrl_data |= ((rsize & 0x3) << DELTA_FPGA_I2C_REG_LEN_OFFSET);
    ctrl_data |= ((dsize & 0x1ff) << DELTA_FPGA_I2C_DATA_LEN_OFFSET);
    ctrl_data |= 1 << DELTA_FPGA_I2C_RW_OFFSET;
    ctrl_data |= 1 << DELTA_FPGA_I2C_START_OFFSET;
#ifdef FPGA_PCA9548
    if (i2c->mux_en == FPGA_I2C_MUX_EN)
    {
        ctrl_data |= ((i2c->mux_ch & 0x7) << DELTA_FPGA_I2C_CH_SEL_OFFSET);
        ctrl_data |= ((1 & 0x1) << DELTA_FPGA_I2C_CH_EN_OFFSET);
    }
#endif
    /* check rsize */
    if (rsize > 2)
    {
        return -1;
    }

    delta_fpga_i2c_ctrl_set(i2c, ctrl_data);
    /* wait for i2c transaction completion */
    if (delta_wait_i2c_complete(i2c))
    {
        printk(KERN_INFO "i2c transaction completion timeout");
        rv = -EBUSY;
        return rv;
    }
    /* check status */
    //DNI_LOG_INFO("check i2c transaction status ...");
    status = io_read(i2c, DELTA_I2C_CTRL(i2c->offset));
    //	udelay(500);
    if ((status & I2C_TRANS_FAIL))
    {
        //		printk(KERN_INFO "I2C read failed: status=0x%lx", status);
        rv = -EIO; /* i2c transfer error */
        goto fail;
    }
    return 0;
fail:

    return rv;
}

static int dni_fpga_i2c_read(struct i2c_bus_dev *i2c, int addr, int raddr, int rsize, uint8_t *readout, int dsize)
{
    int status;
    uint32_t ctrl_data, addr_data, rw_data;
    int offset;
    int rv = -1;
    int i;

    if (i2c->mux_en == FPGA_I2C_MUX_EN)
    {
        if (addr < 0x50 || addr > 0x58)
            goto fail;
    }

    /* Set address register */
    if (rsize == 0)
        addr_data = 0;
    else if (rsize == 1)
    {
        addr = addr + (raddr / 0x100);
        addr_data = (raddr & 0xff);
    }
    else if (rsize == 2)
        addr_data = (raddr & 0xffff);
    delta_fpga_i2c_addr_reg_set(i2c, addr_data);
#ifdef FPGA_PCA9548
    delta_fpga_i2c_conf_reg_set(i2c, ch, 0x70);
#endif
    ctrl_data = 0;
    /* Set ctrl reg */
    ctrl_data |= ((addr & 0x7f) << DELTA_FPGA_I2C_SLAVE_OFFSET);
    ctrl_data |= ((rsize & 0x3) << DELTA_FPGA_I2C_REG_LEN_OFFSET);
    ctrl_data |= ((dsize & 0x1ff) << DELTA_FPGA_I2C_DATA_LEN_OFFSET);
    ctrl_data |= 0 << DELTA_FPGA_I2C_RW_OFFSET;
    ctrl_data |= 1 << DELTA_FPGA_I2C_START_OFFSET;
#ifdef FPGA_PCA9548
    if (i2c->mux_en == FPGA_I2C_MUX_EN)
    {
        ctrl_data |= ((i2c->mux_ch & 0x7) << DELTA_FPGA_I2C_CH_SEL_OFFSET);
        ctrl_data |= ((1 & 0x1) << DELTA_FPGA_I2C_CH_EN_OFFSET);
    }
#endif
    /* check rsize */
    if (rsize > 2)
    {
        goto fail;
    }

    delta_fpga_i2c_ctrl_set(i2c, ctrl_data);
    /* wait for i2c transaction completion */
    if (delta_wait_i2c_complete(i2c))
    {
        printk(KERN_ERR "i2c transaction completion timeout");
        rv = -EBUSY;
        goto fail;
    }
    /* check status */
    //DNI_LOG_INFO("check i2c transaction status ...\n");
    udelay(100);
    status = io_read(i2c, DELTA_I2C_CTRL(i2c->offset));
    if ((status & I2C_TRANS_FAIL))
    {
        //printk(KERN_ERR "I2C read failed:addr_data=0x%lx,  ctrl_data=0x%lx, status=0x%lx ",addr_data, ctrl_data ,status);
        rv = -EIO; /* i2c transfer error */
        goto fail;
    }

    for (i = 1; i <= dsize; i++)
    {
        if ((i - 1) % 4 == 0)
        {
            offset = DELTA_I2C_DATA(i2c->offset) + (i - 1);
            rw_data = io_read(i2c, offset);
        }
        readout[i - 1] = (uint8_t)(rw_data >> (((i - 1) % 4) * 8));
    }
    return 0;
fail:
    return -1;
}

static int io_read(struct i2c_bus_dev *i2c, int offset)
{
    return ioread32(i2c->bar + offset);
}
static void io_write(struct i2c_bus_dev *i2c, int offset, int data)
{
    iowrite32(data, i2c->bar + offset);
}

static u32 dni_fpga_i2c_func(struct i2c_adapter *adapter)
{
    return I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE |
           I2C_FUNC_SMBUS_BYTE_DATA |
           I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA |
           I2C_FUNC_SMBUS_PROC_CALL | I2C_FUNC_SMBUS_BLOCK_PROC_CALL |
           I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_PEC;
}

static const struct i2c_algorithm smbus_algorithm = {
    .smbus_xfer = dni_fpga_i2c_access,
    .functionality = dni_fpga_i2c_func,
};

int i2c_adapter_init(struct pci_dev *dev, struct fpga_dev *fpga)
{
    int pci_base, pci_size;
    int i, j, error, bus = 0;
    int num_i2c_master;

    num_i2c_master = sizeof(fpga_i2c_info) / sizeof(fpga_i2c_s);
    for (i = 0; i < num_i2c_master; i++)
    {
        num_i2c_adapter++;
        if (fpga_i2c_info[i].mux_en == FPGA_I2C_MUX_EN)
            num_i2c_adapter += fpga_i2c_info[i].num_ch;
    }
    pci_base = pci_resource_start(dev, 0);
    pci_size = pci_resource_len(dev, 0);

    fpga->i2c = kzalloc(sizeof(struct i2c_bus_dev) * num_i2c_adapter, GFP_KERNEL);

    if (!fpga)
        return -ENOMEM;

    pci_set_drvdata(dev, fpga);
    printk(KERN_INFO "fpga = %p, pci_size = 0x%x \n", fpga, pci_size);
    mutex_init(&fpga_i2c_lock);
    /* Create PCIE device */
    for (i = 0; i < num_i2c_master; i++)
    {
        fpga->dev = dev;
        fpga->pci_base = pci_base;
        fpga->pci_size = pci_size;

        (fpga->i2c + bus)->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));

        printk(KERN_INFO "BAR0 Register[%p] = 0x%x\n",
               (fpga->i2c + bus)->bar, ioread32((fpga->i2c + bus)->bar));
        /* Create I2C adapter */
        printk(KERN_INFO "dev-%d, pci_base = 0x%x, dev_offset = 0x%x", i, fpga->pci_base, fpga_i2c_info[i].offset);
        (fpga->i2c + bus)->adapter.owner = THIS_MODULE;
        snprintf((fpga->i2c + bus)->adapter.name, sizeof((fpga->i2c + bus)->adapter.name),
                fpga_i2c_info[i].name, i);
        (fpga->i2c + bus)->adapter.class = I2C_CLASS_HWMON;
        (fpga->i2c + bus)->adapter.algo = &smbus_algorithm;
        (fpga->i2c + bus)->adapter.algo_data = fpga->i2c + bus;
        /* set up the sysfs linkage to our parent device */
        (fpga->i2c + bus)->adapter.dev.parent = &dev->dev;
        /* set i2c adapter number */
        (fpga->i2c+bus)->adapter.nr = i+600;
        /* set up the busnum */
        (fpga->i2c + bus)->busnum = i;
        (fpga->i2c + bus)->offset = fpga_i2c_info[i].offset;
        /* set up i2c mux */
        (fpga->i2c + bus)->mux_ch = 0;
        (fpga->i2c + bus)->mux_en = FPGA_I2C_MUX_DIS;

        error = i2c_add_numbered_adapter(&(fpga->i2c+bus)->adapter);
        //error = i2c_add_adapter(&(fpga->i2c + bus)->adapter);
        if (error)
            goto out_release_region;

        bus++;
        if (fpga_i2c_info[i].mux_en == FPGA_I2C_MUX_EN)
        {
            for (j = 0; j < fpga_i2c_info[i].num_ch; j++)
            {
                (fpga->i2c + bus)->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));

                printk(KERN_INFO "BAR0 Register[%p] = 0x%x\n",
                       (fpga->i2c + bus)->bar, ioread32((fpga->i2c + bus)->bar));

                /* Create I2C adapter */
                printk(KERN_INFO "dev-%d, pci_base = 0x%x, dev_offset = 0x%x", i, fpga->pci_base, fpga_i2c_info[i].offset);
                (fpga->i2c + bus)->adapter.owner = THIS_MODULE;
                snprintf((fpga->i2c + bus)->adapter.name, sizeof((fpga->i2c + bus)->adapter.name),
                        fpga_i2c_info[i].name, i, j);
                (fpga->i2c + bus)->adapter.class = I2C_CLASS_HWMON;
                (fpga->i2c + bus)->adapter.algo = &smbus_algorithm;
                (fpga->i2c + bus)->adapter.algo_data = fpga->i2c + bus;
                /* set up the sysfs linkage to our parent device */
                (fpga->i2c + bus)->adapter.dev.parent = &dev->dev;
                /* set i2c adapter number */
                //		(fpga->i2c+bus)->adapter.nr = i+10;
                /* set up the busnum */
                (fpga->i2c + bus)->busnum = i;
                /* set up i2c mux */
                (fpga->i2c + bus)->mux_ch = j;
                (fpga->i2c + bus)->mux_en = FPGA_I2C_MUX_EN;
                error = i2c_add_adapter(&(fpga->i2c + bus)->adapter);
                if (error)
                    goto out_release_region;
                bus++;
            }
        }
    }
    return 0;
out_release_region:
    return error;
}
