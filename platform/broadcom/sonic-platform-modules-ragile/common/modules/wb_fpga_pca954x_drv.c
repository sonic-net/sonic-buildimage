#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "fpga_i2c.h"

extern int i2c_device_func_write(const char *path, uint32_t pos, uint8_t *val, size_t size);
extern int pcie_device_func_write(const char *path, uint32_t offset, uint8_t *buf, size_t count);

#define PCA954X_MAX_NCHANS           (8)
#define FPGA_INTERNAL_PCA9548        (1)
#define FPGA_EXTERNAL_PCA9548        (2)
#define FPGA_I2C_EXT_9548_EXITS      (0x01 << 0)
#define FPGA_I2C_9548_NO_RESET       (0x01 << 1)

#define SYMBOL_I2C_DEV_MODE          (1)
#define FILE_MODE                    (2)
#define SYMBOL_PCIE_DEV_MODE         (3)
#define SYMBOL_IO_DEV_MODE           (4)

int g_fpga_pca954x_debug = 0;
int g_fpga_pca954x_error = 0;

module_param(g_fpga_pca954x_debug, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_pca954x_error, int, S_IRUGO | S_IWUSR);

#define FPGA_PCA954X_VERBOSE(fmt, args...) do {                                        \
    if (g_fpga_pca954x_debug) { \
        printk(KERN_INFO "[FPGA_PCA954X][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_PCA954X_ERROR(fmt, args...) do {                                        \
    if (g_fpga_pca954x_error) { \
        printk(KERN_ERR "[FPGA_PCA954X][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

enum pca_type {
    pca_9540,
    pca_9541,
    pca_9542,
    pca_9543,
    pca_9544,
    pca_9545,
    pca_9546,
    pca_9547,
    pca_9548,
};

struct pca954x {
    enum pca_type type;
    struct i2c_adapter *virt_adaps[PCA954X_MAX_NCHANS];
    u8 last_chan;                   /* last register value */
    uint32_t fpga_9548_flag;
    uint32_t fpga_9548_reset_flag;
    uint32_t pca9548_base_nr;
    struct i2c_client *client;
};

struct chip_desc {
    u8 nchans;
    u8 enable;    /* used for muxes only */
    enum muxtype {
        pca954x_ismux = 0,
        pca954x_isswi
    } muxtype;
};

/* Provide specs for the PCA954x types we know about */
static const struct chip_desc chips[] = {
    [pca_9540] = {
        .nchans = 2,
        .enable = 0x4,
        .muxtype = pca954x_ismux,
    },
    [pca_9541] = {
        .nchans = 1,
        .muxtype = pca954x_isswi,
    },
    [pca_9543] = {
        .nchans = 2,
        .muxtype = pca954x_isswi,
    },
    [pca_9544] = {
        .nchans = 4,
        .enable = 0x4,
        .muxtype = pca954x_ismux,
    },
    [pca_9545] = {
        .nchans = 4,
        .muxtype = pca954x_isswi,
    },
    [pca_9547] = {
        .nchans = 8,
        .enable = 0x8,
        .muxtype = pca954x_ismux,
    },
    [pca_9548] = {
        .nchans = 8,
        .muxtype = pca954x_isswi,
    },
};

static const struct i2c_device_id fpga_pca954x_id[] = {
    { "wb_fpga_pca9540", pca_9540 },
    { "wb_fpga_pca9541", pca_9541 },
    { "wb_fpga_pca9542", pca_9543 },
    { "wb_fpga_pca9543", pca_9543 },
    { "wb_fpga_pca9544", pca_9544 },
    { "wb_fpga_pca9545", pca_9545 },
    { "wb_fpga_pca9546", pca_9545 },
    { "wb_fpga_pca9547", pca_9547 },
    { "wb_fpga_pca9548", pca_9548 },
    { }
};
MODULE_DEVICE_TABLE(i2c, fpga_pca954x_id);

static int fpga_file_write(const char *path, int pos, unsigned char *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        FPGA_PCA954X_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    tmp_pos = (loff_t)pos;
    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        FPGA_PCA954X_ERROR("kernel_write failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, pos, size, ret);
        goto exit;
    }

    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;

}
static int fpga_device_write(fpga_i2c_dev_t *fpga_i2c, int pos, unsigned char *val, size_t size)
{
    int ret;

    switch (fpga_i2c->i2c_func_mode) {
    case SYMBOL_I2C_DEV_MODE:
        ret = i2c_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case FILE_MODE:
        ret = fpga_file_write(fpga_i2c->dev_name, pos, val, size);
        break;
    case SYMBOL_PCIE_DEV_MODE:
        ret = pcie_device_func_write(fpga_i2c->dev_name, pos, val, size);
        break;
    default:
        FPGA_PCA954X_ERROR("err func mode, write failed.\n");
        return -EINVAL;
    }

    return ret;
}

static int fpga_reg_write(fpga_i2c_dev_t *fpga_i2c, uint32_t addr, uint8_t val)
{
    int ret;

    ret = fpga_device_write(fpga_i2c, addr, &val, sizeof(uint8_t));
    if (ret < 0) {
        FPGA_PCA954X_ERROR("fpga_device_write failed. name:%s, addr:0x%x, value:0x%x.\n",
            fpga_i2c->dev_name, addr, val);
        return ret;
    }

    FPGA_PCA954X_VERBOSE("fpga reg write success, dev name:%s, offset:0x%x, value:0x%x.\n",
        fpga_i2c->dev_name, addr, val);
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,7)
static int pca954x_select_chan(struct i2c_adapter *adap, void *client, u32 chan)
{
    struct pca954x *data = i2c_get_clientdata(client);
    fpga_i2c_dev_t *fpga_i2c;
    fpga_i2c_reg_t *reg;
    int ret;
    u8 regval, i2c_9548_opt;

    while(i2c_parent_is_i2c_adapter(adap)){
        adap = to_i2c_adapter(adap->dev.parent);
    }

    FPGA_PCA954X_VERBOSE("root bus:%d, chan:0x%x, 9548 flag:0x%x, 9548 addr:0x%x.\n",
        adap->nr, chan, data->fpga_9548_flag, client->addr);
    fpga_i2c = i2c_get_adapdata(adap);
    reg = &fpga_i2c->reg;

    regval = 1 << chan;
    if (data->fpga_9548_flag == FPGA_INTERNAL_PCA9548) {
        ret = fpga_reg_write(fpga_i2c, reg->i2c_in_9548_chan, regval);
    } else {
        if (data->fpga_9548_reset_flag == 1) {
            i2c_9548_opt = FPGA_I2C_EXT_9548_EXITS & ~(FPGA_I2C_9548_NO_RESET);
        } else {
            i2c_9548_opt = FPGA_I2C_EXT_9548_EXITS | FPGA_I2C_9548_NO_RESET;
        }
        FPGA_PCA954X_VERBOSE("fpga pca9548 reset flag:0x%x, opt:0x%x.\n",
            data->fpga_9548_reset_flag, i2c_9548_opt);
        ret = fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_exits_flag, i2c_9548_opt);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_addr, client->addr);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_chan, regval);
    }

    return ret;
}

static int pca954x_deselect_mux(struct i2c_adapter *adap, void *client, u32 chan)
{
    struct pca954x *data = i2c_get_clientdata(client);
    fpga_i2c_dev_t *fpga_i2c;
    fpga_i2c_reg_t *reg;
    int ret;

    while(i2c_parent_is_i2c_adapter(adap)){
        adap = to_i2c_adapter(adap->dev.parent);
    }

    fpga_i2c = i2c_get_adapdata(adap);
    reg = &fpga_i2c->reg;
    /* Deselect active channel */
    data->last_chan = 0;
    if (data->fpga_9548_flag == FPGA_INTERNAL_PCA9548) {
        ret = fpga_reg_write(fpga_i2c, reg->i2c_in_9548_chan, 0);
    } else {

        ret = fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_exits_flag, FPGA_I2C_9548_NO_RESET);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_chan, 0);
    }

    return ret;
}
#else
static int pca954x_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca954x *data = i2c_mux_priv(muxc);
    struct i2c_client *client = data->client;
    struct i2c_adapter *adap;
    fpga_i2c_dev_t *fpga_i2c;
    fpga_i2c_reg_t *reg;
    int ret;
    u8 regval, i2c_9548_opt;

    adap = muxc->parent;
    while(i2c_parent_is_i2c_adapter(adap)){
        adap = to_i2c_adapter(adap->dev.parent);
    }

    FPGA_PCA954X_VERBOSE("root bus:%d, chan:0x%x, 9548 flag:0x%x, 9548 addr:0x%x.\n",
        adap->nr, chan, data->fpga_9548_flag, client->addr);
    fpga_i2c = i2c_get_adapdata(adap);
    reg = &fpga_i2c->reg;

    regval = 1 << chan;
    if (data->fpga_9548_flag == FPGA_INTERNAL_PCA9548) {
        ret = fpga_reg_write(fpga_i2c, reg->i2c_in_9548_chan, regval);
    } else {
        if (data->fpga_9548_reset_flag == 1) {
            i2c_9548_opt = FPGA_I2C_EXT_9548_EXITS & ~(FPGA_I2C_9548_NO_RESET);
        } else {
            i2c_9548_opt = FPGA_I2C_EXT_9548_EXITS | FPGA_I2C_9548_NO_RESET;
        }
        FPGA_PCA954X_VERBOSE("fpga pca9548 reset flag:0x%x, opt:0x%x.\n",
            data->fpga_9548_reset_flag, i2c_9548_opt);
        ret = fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_exits_flag, i2c_9548_opt);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_addr, client->addr);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_chan, regval);
    }

    return ret;
}

static int pca954x_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
    struct pca954x *data = i2c_mux_priv(muxc);
    struct i2c_adapter *adap;
    fpga_i2c_dev_t *fpga_i2c;
    fpga_i2c_reg_t *reg;
    int ret;

    adap = muxc->parent;
    while(i2c_parent_is_i2c_adapter(adap)){
        adap = to_i2c_adapter(adap->dev.parent);
    }

    fpga_i2c = i2c_get_adapdata(adap);
    reg = &fpga_i2c->reg;
    ret = 0;
    /* Deselect active channel */
    data->last_chan = 0;

    if (data->fpga_9548_flag == FPGA_INTERNAL_PCA9548) {
        ret = fpga_reg_write(fpga_i2c, reg->i2c_in_9548_chan, 0);
    } else {

        ret = fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_exits_flag, FPGA_I2C_9548_NO_RESET);
        ret += fpga_reg_write(fpga_i2c, reg->i2c_ext_9548_chan, 0);
    }

    return ret;
}
#endif
/*
 * I2C init/probing/exit functions
 */
static int fpga_i2c_pca954x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
    int num, force, class;
    struct pca954x *data;
    int ret = -ENODEV;
    struct device *dev;
    int dynamic_nr = 1;
    fpga_pca954x_device_t *fpga_pca954x_device;

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,6,7)
    struct i2c_mux_core *muxc;
#endif

    if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE)) {
        dev_err(&client->dev, "i2c adapter:%d, unsupport I2C_FUNC_SMBUS_BYTE.\n", adap->nr);
        goto err;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
    data = kzalloc(sizeof(struct pca954x), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "kzalloc failed.\n");
        ret = -ENOMEM;
        goto err;
    }

    i2c_set_clientdata(client, data);
#else
    muxc = i2c_mux_alloc(adap, &client->dev,
                 PCA954X_MAX_NCHANS, sizeof(*data), 0,
                 pca954x_select_chan, pca954x_deselect_mux);
    if (!muxc) {
        dev_err(&client->dev, "i2c_mux_alloc failed.\n");
        return -ENOMEM;
    }
    data = i2c_mux_priv(muxc);
    i2c_set_clientdata(client, muxc);
    data->client = client;
#endif

    dev = &client->dev;
    if (dev == NULL) {
        dev_err(&client->dev, "dev is NULL.\n");
        ret = -ENODEV;
        goto exit_free;
    }

    if (dev->of_node == NULL) {
        if (client->dev.platform_data == NULL) {
            dev_err(&client->dev, "Failed to get 954x platform data config.\n");
            ret = -EINVAL;
            goto exit_free;
        }
        fpga_pca954x_device = client->dev.platform_data;
        data->fpga_9548_flag = fpga_pca954x_device->fpga_9548_flag;
        data->fpga_9548_reset_flag = fpga_pca954x_device->fpga_9548_reset_flag;
        data->pca9548_base_nr = fpga_pca954x_device->pca9548_base_nr;
        if (data->pca9548_base_nr == 0) {

            dynamic_nr = 1;
        } else {
            dynamic_nr = 0;
            FPGA_PCA954X_VERBOSE("pca9548_base_nr:%u.\n", data->pca9548_base_nr);
        }
    } else {
        data->type = id->driver_data;
        /* BUS ID */
        ret = of_property_read_u32(dev->of_node, "fpga_9548_flag", &data->fpga_9548_flag);
        ret += of_property_read_u32(dev->of_node, "fpga_9548_reset_flag", &data->fpga_9548_reset_flag);
        if (ret != 0) {
            dev_err(&client->dev, "Failed to get 954x dts config, ret:%d.\n", ret);
            ret = -EINVAL;
            goto exit_free;
        }
        if (of_property_read_u32(dev->of_node, "pca9548_base_nr", &data->pca9548_base_nr)) {

            dynamic_nr = 1;
            FPGA_PCA954X_VERBOSE("pca9548_base_nr not found, use dynamic adap number");
        } else {
            dynamic_nr = 0;
            FPGA_PCA954X_VERBOSE("pca9548_base_nr:%u.\n", data->pca9548_base_nr);
        }
    }

    if (data->fpga_9548_flag != FPGA_EXTERNAL_PCA9548 && data->fpga_9548_flag != FPGA_INTERNAL_PCA9548) {
        dev_err(&client->dev, "Error: fpga 954x flag config error, value:0x%x.\n", data->fpga_9548_flag);
        ret = -EINVAL;
        goto exit_free;
    }

    data->type = id->driver_data;
    data->last_chan = 0;   /* force the first selection */

    /* Now create an adapter for each channel */
    for (num = 0; num < chips[data->type].nchans; num++) {
        if (dynamic_nr == 1) {
            force = 0;              /* dynamic adap number */
        } else {
            force = data->pca9548_base_nr + num;
        }
        class = 0;              /* no class by default */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
        data->virt_adaps[num] =
            i2c_add_mux_adapter(adap, &client->dev, client,
                force, num, class, pca954x_select_chan, pca954x_deselect_mux);

        if (data->virt_adaps[num] == NULL) {
            ret = -ENODEV;
            dev_err(&client->dev, "Failed to register multiplexed adapter %d as bus %d\n",
                num, force);
            goto virt_reg_failed;
        }
#else
        ret = i2c_mux_add_adapter(muxc, force, num, class);
        if (ret) {
            dev_err(&client->dev, "Failed to register multiplexed adapter %d as bus %d\n",
                num, force);
            goto virt_reg_failed;
        }
#endif
    } /* end for num = 0; num < chips[data->type].nchans... */

    dev_info(&client->dev, "registered %d multiplexed busses for I2C %s %s\n",
        num, chips[data->type].muxtype == pca954x_ismux ? "mux" : "switch", client->name);

    return 0;

virt_reg_failed:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
    for (num--; num >= 0; num--)
        i2c_del_mux_adapter(data->virt_adaps[num]);
exit_free:
    kfree(data);
#else
exit_free:
    i2c_mux_del_adapters(muxc);
#endif
err:
    return ret;
}

static int fpga_i2c_pca954x_remove(struct i2c_client *client)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
    struct pca954x *data = i2c_get_clientdata(client);
    const struct chip_desc *chip = &chips[data->type];
    int i;

    for (i = 0; i < chip->nchans; ++i)
    if (data->virt_adaps[i]) {
        i2c_del_mux_adapter(data->virt_adaps[i]);
        data->virt_adaps[i] = NULL;
    }

    kfree(data);
#else
    struct i2c_mux_core *muxc = i2c_get_clientdata(client);

    i2c_mux_del_adapters(muxc);
#endif

    return 0;
}

static struct i2c_driver fpga_i2c_pca954x_driver = {
    .driver        = {
        .name = "wb_fpga_pca954x",
        .owner = THIS_MODULE,
    },
    .probe = fpga_i2c_pca954x_probe,
    .remove = fpga_i2c_pca954x_remove,
    .id_table = fpga_pca954x_id,
};

static int __init fpga_i2c_pca954x_init(void)
{
    int ret;

    ret = i2c_add_driver(&fpga_i2c_pca954x_driver);
    return ret;
}

static void __exit fpga_i2c_pca954x_exit(void)
{
    i2c_del_driver(&fpga_i2c_pca954x_driver);
}

module_init(fpga_i2c_pca954x_init);
module_exit(fpga_i2c_pca954x_exit);
MODULE_DESCRIPTION("fpga pca954x driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
