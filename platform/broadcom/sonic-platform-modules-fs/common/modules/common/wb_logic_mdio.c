#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/mdio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>

#include "wb_logic_mdio.h"

#define DRV_NAME "wb-logic-mdio"
#include <wb_bsp_kernel_debug.h>

#define MDIO_DATA    (0x00)
#define MDIO_CTRL    (0x04)

#define MDIO_TIMEOUT        (100)       /* ms */
#define MDIO_WAIT_SCH       (10)       /* us */

/* data reg */
#define MDIO_RDATA(data)        ((data) & 0xffff)

#define MDIO_RNW                BIT(16)    /* 0:write 1:read */
#define MDIO_RD_VALID           BIT(17)    /* 1:valid */
#define MDIO_BUSY               BIT(18)    /* 1:busy */

#define NOT_BUSY                0
#define BUSY                    1

/* ctrl reg */
#define MDIO_DEVAD(devad)       (((devad)) & 0x1f)
#define MDIO_PRTAD(addr)        ((((addr)) & 0x1f) << 8)
#define MDIO_REGAD(reg)         ((((reg)) & 0xffff) << 16)

#define GET_MDIO_DEVAD(regnum)      ((regnum >> 16) & 0x1f)
#define GET_MDIO_REG(regnum)        (regnum  & 0xffff)

#define SYMBOL_I2C_DEV_MODE   (1)
#define FILE_MODE             (2)
#define SYMBOL_PCIE_DEV_MODE  (3)
#define SYMBOL_IO_DEV_MODE    (4)

#define REG_WIDTH_1        (1)
#define REG_WIDTH_2        (2)
#define REG_WIDTH_4        (4)

struct logic_mdio_info {
    const char *dev_name;
    u32 big_endian;
    u32 reg_access_mode;
    u32 reg_width;
    u32 reg_offset;

    int (*setreg)(struct logic_mdio_info *logic_mdio, int reg, u32 value);
    int (*getreg)(struct logic_mdio_info *logic_mdio, int reg, u32 *value);
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
};

/* Use the wb_bsp_kernel_debug header file must define debug variable */
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int wb_logic_mdio_reg_write(struct logic_mdio_info *logic_mdio, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)logic_mdio->write_intf_addr;
    return pfunc(logic_mdio->dev_name, pos, val, size);
}

static int wb_logic_mdio_reg_read(struct logic_mdio_info *logic_mdio, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)logic_mdio->read_intf_addr;
    return pfunc(logic_mdio->dev_name, pos, val, size);
}

static int wb_logic_mdio_setreg_8(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u8 buf_tmp[REG_WIDTH_1];
    int ret;

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, value);

    buf_tmp[0] = (value & 0Xff);
    ret = wb_logic_mdio_reg_write(logic_mdio, reg, buf_tmp, REG_WIDTH_1);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_write failed, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static int wb_logic_mdio_setreg_16(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u8 buf_tmp[REG_WIDTH_2];
    int ret;

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, value);

    buf_tmp[0] = (value & 0Xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    ret = wb_logic_mdio_reg_write(logic_mdio, reg, buf_tmp, REG_WIDTH_2);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_write failed, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static int wb_logic_mdio_setreg_32(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u8 buf_tmp[REG_WIDTH_4];
    int ret;

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, value);

    buf_tmp[0] = (value & 0xff);
    buf_tmp[1] = (value >> 8) & 0xff;
    buf_tmp[2] = (value >> 16) & 0xff;
    buf_tmp[3] = (value >> 24) & 0xff;

    ret = wb_logic_mdio_reg_write(logic_mdio, reg, buf_tmp, REG_WIDTH_4);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_write failed, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static int wb_logic_mdio_setreg_16be(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u8 buf_tmp[REG_WIDTH_2];
    int ret;

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, value);

    buf_tmp[0] = (value >> 8) & 0xff;
    buf_tmp[1] = (value & 0Xff);
    ret = wb_logic_mdio_reg_write(logic_mdio, reg, buf_tmp, REG_WIDTH_2);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_write failed, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static int wb_logic_mdio_setreg_32be(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u8 buf_tmp[REG_WIDTH_4];
    int ret;

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, value);

    buf_tmp[0] = (value >> 24) & 0xff;
    buf_tmp[1] = (value >> 16) & 0xff;
    buf_tmp[2] = (value >> 8) & 0xff;
    buf_tmp[3] = (value & 0xff);
    ret = wb_logic_mdio_reg_write(logic_mdio, reg, buf_tmp, REG_WIDTH_4);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_write failed, ret %d.\n", ret);
        return ret;
    }

    return 0;
}

static inline int wb_logic_mdio_getreg_8(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u8 buf_tmp[REG_WIDTH_1];
    u32 tmp_value;
    int ret;

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = wb_logic_mdio_reg_read(logic_mdio, reg, buf_tmp, REG_WIDTH_1);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_read failed, ret %d.\n", ret);
        return ret;
    }
    tmp_value = buf_tmp[0];

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, tmp_value);

    *value = tmp_value;
    return 0;
}

static inline int wb_logic_mdio_getreg_16(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u8 buf_tmp[REG_WIDTH_2];
    u32 tmp_value;
    int i;
    int ret;

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = wb_logic_mdio_reg_read(logic_mdio, reg, buf_tmp, REG_WIDTH_2);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_read failed, ret %d.\n", ret);
        return ret;
    }

    tmp_value = 0;
    for (i = 0; i < REG_WIDTH_2; i++) {
        tmp_value |= buf_tmp[i] << (8 * i);
    }

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, tmp_value);

    *value = tmp_value;
    return 0;
}

static inline int wb_logic_mdio_getreg_32(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u8 buf_tmp[REG_WIDTH_4];
    u32 tmp_value;
    int i;
    int ret;

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = wb_logic_mdio_reg_read(logic_mdio, reg, buf_tmp, REG_WIDTH_4);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_read failed, ret %d.\n", ret);
        return ret;
    }

    tmp_value = 0;
    for (i = 0; i < REG_WIDTH_4; i++) {
        tmp_value |= buf_tmp[i] << (8 * i);
    }

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, tmp_value);

    *value = tmp_value;
    return 0;
}

static inline int wb_logic_mdio_getreg_16be(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u8 buf_tmp[REG_WIDTH_2];
    u32 tmp_value;
    int i;
    int ret;

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = wb_logic_mdio_reg_read(logic_mdio, reg, buf_tmp, REG_WIDTH_2);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_read failed, ret %d.\n", ret);
        return ret;
    }

    tmp_value = 0;
    for (i = 0; i < REG_WIDTH_2; i++) {
        tmp_value |= buf_tmp[i] << (8 * (REG_WIDTH_2 -i - 1));
    }

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, tmp_value);

    *value = tmp_value;
    return 0;
}

static inline int wb_logic_mdio_getreg_32be(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u8 buf_tmp[REG_WIDTH_4];
    u32 tmp_value;
    int i;
    int ret;

    mem_clear(buf_tmp, sizeof(buf_tmp));
    ret = wb_logic_mdio_reg_read(logic_mdio, reg, buf_tmp, REG_WIDTH_4);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_reg_read failed, ret %d.\n", ret);
        return ret;
    }

    tmp_value = 0;
    for (i = 0; i < REG_WIDTH_4; i++) {
        tmp_value |= buf_tmp[i] << (8 * (REG_WIDTH_4 -i - 1));
    }

    DEBUG_VERBOSE("path:%s, access mode:%d, reg:0x%x, value0x%x.\n",
                       logic_mdio->dev_name, logic_mdio->reg_access_mode, reg, tmp_value);

    *value = tmp_value;
    return 0;
}

static inline int wb_logic_mdio_setreg(struct logic_mdio_info *logic_mdio, int reg, u32 value)
{
    u32 pos;

    pos = logic_mdio->reg_offset + reg;
    return logic_mdio->setreg(logic_mdio, pos, value);
}

static inline int wb_logic_mdio_getreg(struct logic_mdio_info *logic_mdio, int reg, u32 *value)
{
    u32 pos;

    pos = logic_mdio->reg_offset + reg;
    return logic_mdio->getreg(logic_mdio, pos, value);
}

static int wb_logic_mdio_wait_data_nobusy(struct logic_mdio_info *logic_mdio, int *busy)
{
    u32 data;
    unsigned long time_out;
    unsigned int sch_time;
    int ret;

    time_out = jiffies + msecs_to_jiffies(MDIO_TIMEOUT);
    sch_time = MDIO_WAIT_SCH;
    while (1) {
        data = 0;
        ret = wb_logic_mdio_getreg(logic_mdio, MDIO_DATA, &data);
        if (ret < 0) {
            DEBUG_ERROR("wb_logic_mdio_getreg MDIO_DATA failed, ret %d.\n", ret);
            return ret;
        }

        if (!(data & MDIO_BUSY)) {
            DEBUG_VERBOSE("not busy!\n");
            *busy = NOT_BUSY;
            break;
        }

        if (time_after(jiffies, time_out)) {
            *busy = BUSY;
            return -ETIMEDOUT;
        }
        usleep_range(sch_time, sch_time + 1);
    }

    return 0;
}

static int wb_logic_mdio_read(struct mii_bus *bus, int addr, int regnum)
{
    struct logic_mdio_info *logic_mdio = bus->priv;
    int devad, reg;
    u32 ctrl;
    u32 data;
    unsigned long time_out;
    unsigned int sch_time;
    u32 value;
    int ret;
    int busy;

    DEBUG_VERBOSE("addr:0x%x, regnum:0x%x.\n", addr, regnum);

    /* logic-mdio just clause 45 for the moment */
    if (!(regnum & MII_ADDR_C45)) {
        DEBUG_ERROR("only support clause 45!\n");
        return 0xffff;
    }

    /* check data not busy before write MDIO_CTRL */
    ret = wb_logic_mdio_wait_data_nobusy(logic_mdio, &busy);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_wait_data_nobusy check failed, ret %d.\n", ret);
        return ret;
    }
    if (busy != NOT_BUSY) {
        DEBUG_ERROR("logic-mdio busy!\n");
        return -EBUSY;
    }

    devad = GET_MDIO_DEVAD(regnum);
    reg = GET_MDIO_REG(regnum);

    ctrl = MDIO_DEVAD(devad) | MDIO_PRTAD(addr) | MDIO_REGAD(reg);
    ret = wb_logic_mdio_setreg(logic_mdio, MDIO_CTRL, ctrl);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_setreg MDIO_CTRL failed, ret %d.\n", ret);
        return ret;
    }

    /* check data not busy before write MDIO_DATA */
    ret = wb_logic_mdio_wait_data_nobusy(logic_mdio, &busy);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_wait_data_nobusy check failed, ret %d.\n", ret);
        return ret;
    }
    if (busy != NOT_BUSY) {
        DEBUG_ERROR("logic-mdio busy!\n");
        return -ETIMEDOUT;
    }

    /* read */
    data = MDIO_RNW;
    ret = wb_logic_mdio_setreg(logic_mdio, MDIO_DATA, data);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_setreg MDIO_DATA failed, ret %d.\n", ret);
        return ret;
    }
    time_out = jiffies + msecs_to_jiffies(MDIO_TIMEOUT);
    sch_time = MDIO_WAIT_SCH;
    while (1) {
        ret = wb_logic_mdio_getreg(logic_mdio, MDIO_DATA, &data);
        if (ret < 0) {
            DEBUG_ERROR("wb_logic_mdio_getreg MDIO_DATA failed, ret %d.\n", ret);
            return ret;
        }

        if (!(data & MDIO_BUSY)) {
            DEBUG_VERBOSE("not busy!\n");
            if ((data & MDIO_RD_VALID)) {
                DEBUG_VERBOSE("read success!\n");
                break;
            } else {
                DEBUG_ERROR("data_reg(0x%x), read data invalid!\n", data);
                return -EINVAL;
            }
        }

        if (time_after(jiffies, time_out)) {
            return -ETIMEDOUT;
        }
        usleep_range(sch_time, sch_time + 1);
    }

    value = MDIO_RDATA(data);
    DEBUG_VERBOSE("addr:0x%x, devad:0x%x, reg:0x%x, value:0x%x.\n",
                       addr, devad, reg, value);

    return value;
}

static int wb_logic_mdio_write(struct mii_bus *bus, int addr, int regnum, u16 val)
{
    struct logic_mdio_info *logic_mdio = bus->priv;
    int devad, reg;
    u32 ctrl;
    u32 data;
    int ret;
    int busy;

    DEBUG_VERBOSE("addr:0x%x, regnum:0x%x.\n", addr, regnum);

    /* logic-mdio just clause 45 for the moment */
    if (!(regnum & MII_ADDR_C45)) {
        DEBUG_ERROR("only support clause 45!\n");
        return 0xffff;
    }

    /* check data not busy before write MDIO_CTRL */
    ret = wb_logic_mdio_wait_data_nobusy(logic_mdio, &busy);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_wait_data_nobusy check failed, ret %d.\n", ret);
        return ret;
    }
    if (busy != NOT_BUSY) {
        DEBUG_ERROR("logic-mdio busy!\n");
        return -EBUSY;
    }

    devad = GET_MDIO_DEVAD(regnum);
    reg = GET_MDIO_REG(regnum);

    ctrl = MDIO_DEVAD(devad) | MDIO_PRTAD(addr) | MDIO_REGAD(reg);
    ret = wb_logic_mdio_setreg(logic_mdio, MDIO_CTRL, ctrl);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_setreg MDIO_CTRL failed, ret %d.\n", ret);
        return ret;
    }

    /* check data not busy before write MDIO_DATA */
    ret = wb_logic_mdio_wait_data_nobusy(logic_mdio, &busy);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_wait_data_nobusy check failed, ret %d.\n", ret);
        return ret;
    }
    if (busy != NOT_BUSY) {
        DEBUG_ERROR("logic-mdio busy!\n");
        return -ETIMEDOUT;
    }

    /* write */
    data = val;
    ret = wb_logic_mdio_setreg(logic_mdio, MDIO_DATA, data);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_setreg MDIO_DATA failed, ret %d.\n", ret);
        return ret;
    }

    /* check data not busy after write MDIO_DATA */
    ret = wb_logic_mdio_wait_data_nobusy(logic_mdio, &busy);
    if (ret < 0) {
        DEBUG_ERROR("wb_logic_mdio_wait_data_nobusy check failed, ret %d.\n", ret);
        return ret;
    }
    if (busy != NOT_BUSY) {
        DEBUG_ERROR("logic-mdio busy!\n");
        return -ETIMEDOUT;
    }

    return 0;
}

static int logic_mdio_config_init(struct platform_device *pdev, struct logic_mdio_info *logic_mdio)
{
    int ret;
    logic_mdio_device_t *logic_mdio_device;

    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "dev_name", &logic_mdio->dev_name);
        ret += of_property_read_u32(pdev->dev.of_node, "reg_access_mode", &logic_mdio->reg_access_mode);
        ret += of_property_read_u32(pdev->dev.of_node, "reg_width", &logic_mdio->reg_width);
        ret += of_property_read_u32(pdev->dev.of_node, "reg_offset", &logic_mdio->reg_offset);
        if (ret != 0) {
            dev_err(&pdev->dev, "dts config error.ret:%d.\r\n", ret);
            return -ENXIO;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        logic_mdio_device = pdev->dev.platform_data;
        logic_mdio->dev_name = logic_mdio_device->dev_name;
        logic_mdio->big_endian = logic_mdio_device->big_endian;
        logic_mdio->reg_access_mode = logic_mdio_device->reg_access_mode;
        logic_mdio->reg_width = logic_mdio_device->reg_width;
        logic_mdio->reg_offset = logic_mdio_device->reg_offset;
    }
    return 0;
}

static int wb_logic_mdio_probe(struct platform_device *pdev)
{
    struct mii_bus *bus;
    struct logic_mdio_info *logic_mdio;
    int ret;
    bool be;

    logic_mdio = devm_kzalloc(&pdev->dev, sizeof(*logic_mdio), GFP_KERNEL);
    if (!logic_mdio) {
        dev_err(&pdev->dev, "logic_mdio devm_kzalloc error.\n");
        return -ENOMEM;
    }

    ret = logic_mdio_config_init(pdev, logic_mdio);
    if (ret < 0) {
        return ret;
    }

    if (pdev->dev.of_node) {
        if (of_property_read_u32(pdev->dev.of_node, "big_endian", &logic_mdio->big_endian)) {
            be = 0;
        } else {
            be = logic_mdio->big_endian;
        }
    } else {
        be = logic_mdio->big_endian;
    }

    if (logic_mdio->reg_width == 0) {
        logic_mdio->reg_width = 4; /* Set to default value */
    }

    ret = find_intf_addr(&logic_mdio->write_intf_addr, &logic_mdio->read_intf_addr, logic_mdio->reg_access_mode);
    if (ret) {
        dev_err(&pdev->dev, "Failed to find_intf_addr func mode %u, ret: %d\n", logic_mdio->reg_access_mode, ret);
        return ret;
    }

    if (!logic_mdio->write_intf_addr || !logic_mdio->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined\n", logic_mdio->reg_access_mode);
        return -ENOSYS;
    }

    if (!logic_mdio->setreg || !logic_mdio->getreg) {
        switch (logic_mdio->reg_width) {
            case REG_WIDTH_1:
                logic_mdio->setreg = wb_logic_mdio_setreg_8;
                logic_mdio->getreg = wb_logic_mdio_getreg_8;
                break;

            case REG_WIDTH_2:
                logic_mdio->setreg = be ? wb_logic_mdio_setreg_16be : wb_logic_mdio_setreg_16;
                logic_mdio->getreg = be ? wb_logic_mdio_getreg_16be : wb_logic_mdio_getreg_16;
                break;

            case REG_WIDTH_4:
                logic_mdio->setreg = be ? wb_logic_mdio_setreg_32be : wb_logic_mdio_setreg_32;
                logic_mdio->getreg = be ? wb_logic_mdio_getreg_32be : wb_logic_mdio_getreg_32;
                break;

            default:
                dev_err(&pdev->dev, "Unsupported reg width (%d)\n", logic_mdio->reg_width);
                return -EINVAL;
        }
    }

    bus = mdiobus_alloc();
    if (!bus) {
        dev_err(&pdev->dev, "mdiobus_alloc error.\n");
        return -ENOMEM;
    }

    bus->priv = logic_mdio;
    bus->name = DRV_NAME;
    snprintf(bus->id, MII_BUS_ID_SIZE, "%s%d", pdev->name, pdev->id);
    bus->parent = &pdev->dev;
    bus->read = wb_logic_mdio_read;
    bus->write = wb_logic_mdio_write;
    /* bus->probe_capabilities = MDIOBUS_C45; */

    ret = of_mdiobus_register(bus, pdev->dev.of_node);
    if (ret) {
        mdiobus_free(bus);
        dev_err(&pdev->dev, "Cannot register MDIO bus!\n");
        return ret;
    }

    platform_set_drvdata(pdev, bus);

    return 0;
}

static int wb_logic_mdio_remove(struct platform_device *pdev)
{
    mdiobus_unregister(platform_get_drvdata(pdev));
    return 0;
}

static const struct of_device_id wb_logic_mdio_of_match[] = {
    { .compatible = "wb,wb-logic-mdio", },
    { },
};

static struct platform_driver wb_logic_mdio_driver = {
    .driver = {
        .name = DRV_NAME,
        .of_match_table = wb_logic_mdio_of_match,
    },
    .probe = wb_logic_mdio_probe,
    .remove = wb_logic_mdio_remove,
};

module_platform_driver(wb_logic_mdio_driver);

MODULE_AUTHOR("support");
MODULE_LICENSE("GPL");
