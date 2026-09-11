/*
 * wb_dev_resume.c
 *
 * The main function of the driver is to register the device exception recovery interface with
 * the framework layer through the platform driver, enabling exception recovery for the device
 * when an anomaly occurs.
 *
 * History
 *  [Version]        [Author]               [Date]            [Description]
 *    v1.0             xxx                 2022-04-01         Initial version
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/kallsyms.h>
#include <linux/synclink.h>
#include <linux/bug.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/kprobes.h>
#include <wb_logic_dev_common.h>

#define DRV_NAME        "dev-resume-driver"

/* define of C3000 CPU */
#define C3000_PS2B_PCIE_DEV_VID     (0x8086)
#define C3000_PS2B_PCIE_DEV_DID     (0x19dd)
#define GPIO_RX_DIS_BITS            (BIT9)
#define PAD_MODE_BITS               (BIT10 | BIT11 | BIT12)
#define PAD_TERM_BITS               (BIT10 | BIT11 | BIT12 | BIT13)

/* define of AST2500 */
#define AST2500_EMMC_128_CTRL0_REG      (0x1E740128)
#define AST2500_EMMC_128_CTRL0_MASK     (0x100)   /* bit8 */

/* define of AST2600 */
#define AST2600_EMMC_128_CTRL0_REG      (0x1E750128)
#define AST2600_EMMC_128_CTRL0_MASK     (0x100) /* bit8 */

/* define of AST2700 */
#define AST2700_EMMC_128_CTRL0_REG      (0x12090128)

#define RESUME_WAY_MAX_NUM          (8)
#define RESET_OP                    (0)
#define UNRESET_OP                  (1)
#define DISABLE_WR_PROTECT          (0)
#define ENABLE_WR_PROTECT           (1)

enum ast_emmc_clk_op_e {
    AST_EMMC_CLK_STOP = 0,
    AST_EMMC_CLK_START = 1,
};

enum dev_type_e {
    DEV_TYPE_EUSB,
    DEV_TYPE_MMC,
    DEV_TYPE_SSD,
    DEV_TYPE_END,
};

enum dev_resume_way_e {
    /* Recovery method: Device exception recovery is performed by writing to registers. Different device types have different bound interfaces */
    C3000_RESUME_WAY_WR_REG = 1,
    DEV_RESUME_WAY2_WR_REG = 2,

    /* Recovery method: Device exception recovery is performed by controlling the GPIO levels */
    DEV_RESUME_WAY2_CTL_GPIO = 3,

    /* Recovery method for AST2600 EMMC, mix reg and GPIO */
    DEV_RESUME_WAY_AST2600 = 4,

    /* Recovery method for AST2500 EMMC, mix reg and GPIO */
    DEV_RESUME_WAY_AST2500 = 5,

    /* Recovery method for AST2700 EMMC, same flow as AST2600 with different clk reg */
    DEV_RESUME_WAY_AST2700 = 6,

    DEV_RESUME_WAYX_END,
};

enum reg_access_mode_e {
    ACCESS_MODE_DEV_FILE = 1,
    ACCESS_MODE_IO,
    ACCESS_MODE_MEM,
    ACCESS_MODE_END,
};

typedef struct wr_pro_ctrl_info_s {
    u32 reg_addr;            /* reg addr */
    u32 reg_width;           /* red width */
    /* u32 is_le;               small or large endian configuration; reservation */ 
    u32 access_mode;         /* the register access pattern, refer to reg_access_mode_e */
    u32 enable_wr_pro_val;   /* the value used to enable write protection */
    u32 disable_wr_pro_val;  /* the value used to disable write protection */
    u32 mask;                /* bit mask */

    const char *dev_file_path;     /* dev file path used when access_mode is ACCESS_MODE_DEV_FILE */
    u64 map_base_addr;             /* base addr used when access_mode is ACCESS_MODE_MEM */
} wr_pro_ctrl_info_t;

typedef struct reset_ctrl_info_s {
    u32 reg_addr;
    u32 reg_width;
    u32 access_mode;
    u32 reset_val;
    u32 dereset_val;
    u32 reset_mask;
    u32 reset_delay;
    u32 dereset_delay;
    u32 is_dereset;

    const char *dev_file_path;
    u64 map_base_addr;
} reset_ctrl_info_t;

typedef struct reg_cfg_info_s {
    reset_ctrl_info_t reset_info;      /* configrations used to reset peripheral device */
    /* configurations of write protection */
    bool need_wr_pro;                   /* whether write protection is required  */
    wr_pro_ctrl_info_t wr_pro_info;    /* configrations used to toggle write protection */
} reg_cfg_info_t;

typedef struct gpio_cfg_info_s {
    u32 gpio_num;
    u32 active_level;
    u32 reset_delay;
    u32 dereset_delay;
} gpio_cfg_info_t;

typedef struct ast2500_emmc_cfg_info_s {
    u32 rst_gpio_num;           /* emmc rst pin gpio, 0 means not have rst gpio */
    u32 rst_active_level;
    u32 rst_gpio_flag;          /* rst gpio valid flag, 0 means invalid */

    u32 reset_delay;
    u32 dereset_delay;

    bool support_powercycle;     /* true: support; false: not support */
    reg_cfg_info_t reg_cfg;
} ast2500_emmc_cfg_info_t;

typedef enum power_ctrl_type_s {
    POWER_CTRL_GPIO = 0,
    POWER_CTRL_LOGIC = 1,
} power_ctrl_type_t;

typedef struct logic_attr_s {
    const char *dev_name;
    uint32_t offset;
    uint32_t mask;
    uint32_t power_on;
    uint32_t power_off;
    uint32_t width;
    uint32_t logic_func_mode;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
} logic_attr_t;

typedef struct gpio_attr_s {
    u32 power_gpio_num;     /* emmc power on/off gpio, 0 means not have gpio gpio */
    u32 power_active_level;
} power_gpio_attr_t;

typedef struct ast2600_emmc_cfg_info_s {
    u32 rst_gpio_num;       /* emmc rst pin gpio, 0 means not have rst gpio */
    u32 rst_active_level;
    u32 rst_gpio_flag;      /* rst gpio valid flag, 0 means invalid */
    u32 reset_delay;
    u32 dereset_delay;

    u32 power_ctrl_mode;    /* power up/down mode */
    union {
        power_gpio_attr_t gpio_attr;
        logic_attr_t logic_attr;
    } power_ctrl_attr;
 } ast2600_emmc_cfg_info_t;

typedef enum resume_power_type_e {
    POWER_ON,
    POWER_OFF,
}resume_power_type_t;

/* Store device configuration information */
typedef struct dev_cfg_info_s {
    u32 resume_way;                  /* Resume method - dev_resume_way_e */
    /* Configuration information required by different recovery methods */
    union {
        reg_cfg_info_t  reg_info;
        gpio_cfg_info_t gpio_info;
        ast2600_emmc_cfg_info_t ast2600_emmc_info;
        ast2500_emmc_cfg_info_t ast2500_emmc_info;
    } info;
} dev_cfg_info_t;

typedef int *(*dev_resume_func_t)(u32, u32);

typedef int (*reg_dev_resume_func_t)(dev_resume_func_t);

typedef int (*unreg_dev_resume_func_t)(void);

typedef struct dev_resume_info_s {
    struct list_head list;
    u32 dev_type;
    u32 slot_id;
    u32 resume_way_num;
    u32 bit_mask;
    u32 resume_enable;

    const char *reg_intf_name;
    unsigned long reg_intf_addr;

    const char *unreg_intf_name;
    unsigned long unreg_intf_addr;

    dev_cfg_info_t dev_cfg_info[RESUME_WAY_MAX_NUM];
    struct mutex resume_lock;
} dev_resume_info_t;

/* Manage all registered device recovery information through this linked list. */
static LIST_HEAD(dev_resume_info_list);

static DEFINE_SPINLOCK(dev_resume_info_list_lock);

/********************************** start: debug print ***********************************/
static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "control print info level. 0: none; 1: err; 2: err + warn; 3: err + warn +info; >3: all");
/********************************** start: defaul print ***********************************/
#define DEBUG(fmt, args...)                                                                 \
    do {                                                                                    \
        printk(KERN_DEBUG "[func:%s line:%d]  "fmt, __func__, __LINE__, ## args);           \
    } while (0)
/********************************** end: defaul print ***********************************/

static int dev_resume_info_register_init(struct platform_device *pdev, struct device_node *child,
            reg_cfg_info_t *p_reg_info);

static int noop_pre(struct kprobe *p, struct pt_regs *regs)
{
    return 0;
}
static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name",
};
unsigned long (*kallsyms_lookup_name_fun)(const char *name) = NULL;

/* Call kprobe to find the address location of kallsyms_lookup_name */
static int find_kallsyms_lookup_name(void)
{
    int ret = -1;

    kp.pre_handler = noop_pre;
    ret = register_kprobe(&kp);
    if (ret < 0) {
        DEBUG_ERROR("register_kprobe failed, error:%d\n", ret);
        return ret;
    }
    DEBUG_INFO("kallsyms_lookup_name addr: %p\n", kp.addr);
    kallsyms_lookup_name_fun = (void*)kp.addr;
    unregister_kprobe(&kp);

    return ret;
}

static int file_read_in_kernel(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        DEBUG_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }
    ret = 0;
    tmp_pos = (loff_t)pos;

    ret = kernel_read(filp, val, size, &tmp_pos);
    if (ret < 0) {
        DEBUG_ERROR("read kernel_read failed, ret=%d\r\n", ret);
        goto exit;
    }
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int file_write_in_kernel(const char *path, uint32_t pos, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t tmp_pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        DEBUG_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }
    ret = 0;
    tmp_pos = (loff_t)pos;

    ret = kernel_write(filp, val, size, &tmp_pos);
    if (ret < 0) {
        DEBUG_ERROR("write kernel_write failed, ret=%d\r\n", ret);
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

static int write_reg_accord_to_access_mode(u32 mode, const char *path,
    u64 map_base_addr, u32 addr, u8 *pval, size_t size)
{
    int ret;
    u32 ui_reg_val;
    u16 us_reg_val;
    u8  uc_reg_val;
    u8 __iomem *membase;

    if (pval == NULL) {
        DEBUG_ERROR("invalid para\n");
        return -EINVAL;
    }

    DEBUG_INFO("mode: %u, addr: 0x%x, size: 0x%zx, val: 0x%x\n", \
               mode, addr, size, *(u32*)pval);

    switch (mode) {
        case ACCESS_MODE_DEV_FILE:
            if (path == NULL) {
                DEBUG_ERROR("invalid para dev_file_name\n");
                return -EINVAL;
            }

            DEBUG_INFO("path: %s\n", path);
            ret = file_write_in_kernel(path, addr, pval, size);
            if (ret < 0) {
                DEBUG_ERROR("wr %s fail, ret: %d, addr: 0x%x, size: 0x%zx\n", path, \
                            ret, addr, size);
                return -EIO;
            }

            break;
        case ACCESS_MODE_IO:
            if (size == 4) {
                ui_reg_val = *(u32*)pval;
                outl(ui_reg_val, (unsigned long)(addr));
            } else if (size == 2) {
                us_reg_val = *(u16*)pval;
                outw(us_reg_val, (unsigned long)(addr));
            } else {
                uc_reg_val = *pval;
                outb(uc_reg_val, (unsigned long)(addr));
            }

            break;
        case ACCESS_MODE_MEM:
            membase = ioremap(map_base_addr, size);
            DEBUG_INFO("base_addr: 0x%llx, membase: 0x%p\n", map_base_addr, membase);
            if (membase == NULL) {
                DEBUG_ERROR("ioremap fail\n");
                return -ENOMEM;
            }

            if (size == 4) {
                ui_reg_val = *(u32*)pval;
                writel(ui_reg_val, (membase + addr));
            } else if (size == 2) {
                us_reg_val = *(u16*)pval;
                writew(us_reg_val, (membase + addr));
            } else {
                uc_reg_val = *pval;
                writeb(uc_reg_val, (membase + addr));
            }
            iounmap(membase);

            break;
        default:
            DEBUG_ERROR("invalid access mode: %u\n", mode);
            return -EFAULT;
    }

    return 0;
}

static int read_reg_accord_to_access_mode(u32 mode, const char *path,
    u64 map_base_addr, u32 addr, u8 *pval, size_t size)
{
    int ret;
    u8 __iomem *membase;

    if (pval == NULL) {
        DEBUG_ERROR("invalid para reg_info\n");
        return -EINVAL;
    }

    DEBUG_INFO("mode: %u, addr: 0x%x, size: 0x%zx\n", mode, addr, size);

    switch (mode) {
        case ACCESS_MODE_DEV_FILE:
            if (path == NULL) {
                DEBUG_ERROR("invalid para dev_file_name\n");
                return -EINVAL;
            }

            DEBUG_INFO("path: %s\n", path);
            ret = file_read_in_kernel(path, addr, pval, size);
            if (ret < 0) {
                DEBUG_ERROR("rd %s fail, ret: %d, addr: 0x%x, size: 0x%zx\n", path, \
                            ret, addr, size);
                return -EIO;
            }

            break;
        case ACCESS_MODE_IO:
            if (size == 4) {
                *(u32*)pval = inl((unsigned long)addr);
            } else if (size == 2) {
                *(u16*)pval = inw((unsigned long)addr);
            } else {
                *pval = inb((unsigned long)addr);
            }

            break;
        case ACCESS_MODE_MEM:
            membase = ioremap(map_base_addr, size);
            if (membase == NULL) {
                DEBUG_ERROR("ioremap fail\n");
                return -ENOMEM;
            }

            if (size == 4) {
                *(u32*)pval = readl(membase + addr);
            } else if (size == 2) {
                *(u16*)pval = readw(membase + addr);
            } else {
                *pval = readb(membase + addr);
            }

            iounmap(membase);

            break;
        default:
            DEBUG_ERROR("invalid access mode: %u\n", mode);
            return -EFAULT;
    }

    DEBUG_INFO("rd val: 0x%x", *(u32*)pval);
    return 0;
}

static int resume_logic_read(logic_attr_t *attr, uint32_t offset, uint8_t *buf)
{
    device_func_read pfunc;

    pfunc = (device_func_write)attr->read_intf_addr;
    return pfunc(attr->dev_name, offset, buf, attr->width);
}

static int resume_logic_write(logic_attr_t *logic_attr, uint32_t mask,
    uint32_t offset, uint32_t value)
{
    device_func_write pfunc;
    uint8_t read_value[4], write_value[4];
    int err;
    uint8_t tmp_read8;
    uint32_t tmp_read32, tmp_write32;

    err = resume_logic_read(logic_attr, offset, read_value);
    if (err < 0) {
        DEBUG_VERBOSE("1 byte read fail ret: %d\n", err);
        return err;
    }

    if (logic_attr->width == WIDTH_1Byte) {
        tmp_read8 = read_value[0];
        write_value[0] = ((tmp_read8 & (~mask)) | value) & 0xFF;
        DEBUG_VERBOSE("1 byte write val[0]: 0x%x\n",
            write_value[0]);
    } else {
        memcpy((uint8_t *)&tmp_read32, read_value, logic_attr->width);
        tmp_write32 = (tmp_read32 & (~logic_attr->mask)) | value;

        memcpy(write_value, (uint8_t *)&tmp_write32, logic_attr->width);
        DEBUG_VERBOSE("4 byte write val[0]:0x%x, val[1]:0x%x, val[2]:0x%x, val[3]:0x%x",
            write_value[0], write_value[1], write_value[2], write_value[3]);
    }

    pfunc = (device_func_write)logic_attr->write_intf_addr;
    return pfunc(logic_attr->dev_name, offset, write_value, logic_attr->width);
}

static int do_resume_wr_pro_ctrl(wr_pro_ctrl_info_t *wr_pro_info, u8 wr_pro_flag)
{
    int ret;
    u32 addr, buf, mask, mode;
    u8 *pval;
    size_t size;
    u32 op_val;
    u64 map_base_addr;

    /* Parameters assignment */
    buf = 0;
    pval = (u8 *)&buf;
    addr = wr_pro_info->reg_addr;
    size = wr_pro_info->reg_width;
    mask = wr_pro_info->mask;
    mode = wr_pro_info->access_mode;
    map_base_addr = wr_pro_info->map_base_addr;

    /* 1. Staging the value of the original register */
    ret = read_reg_accord_to_access_mode(mode, wr_pro_info->dev_file_path, 
                map_base_addr, addr, pval, size);
    if (ret != 0) {
        DEBUG_ERROR("failed to read registers of write protections, ret: %d\n", ret);
        return -EFAULT;
    }
    DEBUG_INFO("wr pro read: addr: 0x%x, width: 0x%zx, rd_val: 0x%x\n", addr, size, buf);

    /* 2. Calculate the target value */
    switch (wr_pro_flag) {
    case DISABLE_WR_PROTECT:
        op_val = wr_pro_info->disable_wr_pro_val;
        break;
    case ENABLE_WR_PROTECT:
        op_val = wr_pro_info->enable_wr_pro_val;
        break;
    default:
        DEBUG_ERROR("unknown resume wr pro flag:%u \n", wr_pro_flag);
        return -EFAULT;
    }

    buf &= ~(mask);
    buf |= op_val;
    ret = write_reg_accord_to_access_mode(mode, wr_pro_info->dev_file_path, 
                map_base_addr, addr, pval, size);
    if (ret != 0) {
        DEBUG_ERROR("write protection, failed to write, ret: %d\n", ret);
        return -EFAULT;
    }
    DEBUG_INFO("wr pro write: addr: 0x%x, width: 0x%zx, wr_val: 0x%x\n", addr, size, buf);

    return 0;
}

static int do_resume_by_op_reg_action(reg_cfg_info_t *p_reg_info, u8 reset_flag)
{
    reset_ctrl_info_t *reset_info;
    wr_pro_ctrl_info_t *wr_pro_info;
    int ret;
    u32 addr, buf, mask, mode;
    u8 *pval;
    size_t size;
    u64 map_base_addr;
    u32 op_val, op_delay;

    /* 1. read reg raw value first */
    ret = 0;
    reset_info = &p_reg_info->reset_info;
    buf = 0;
    pval = (u8 *)&buf;
    addr = reset_info->reg_addr;
    size = reset_info->reg_width;
    mask = reset_info->reset_mask;
    mode = reset_info->access_mode;
    map_base_addr = reset_info->map_base_addr;

    ret = read_reg_accord_to_access_mode(mode, reset_info->dev_file_path, map_base_addr,
        addr, pval, size);
    if (ret != 0) {
        DEBUG_ERROR("failed to read reg, ret: %d\n", ret);
        goto error;
    }
    DEBUG_INFO("read: addr: 0x%x, width: 0x%zx, rd_val: 0x%x\n", addr, size, buf);

    if (p_reg_info->need_wr_pro) {
        wr_pro_info = &p_reg_info->wr_pro_info;
        ret = do_resume_wr_pro_ctrl(wr_pro_info, DISABLE_WR_PROTECT);
        if (ret != 0) {
            DEBUG_ERROR("failed to disable wr protect , ret: %d\n", ret);
            goto error;
        }
    }

    switch (reset_flag) {
    case RESET_OP:
        op_val = reset_info->reset_val;
        op_delay = reset_info->reset_delay;
        break;
    case UNRESET_OP:
        op_val = reset_info->dereset_val;
        op_delay = reset_info->dereset_delay;
        break;
    default:
        DEBUG_ERROR("unknown resume reset flag:%u \n", reset_flag);
        ret = -EFAULT;
        goto enable_wr_protect;
    }

    /* 2. write reg to recovery */
    buf &= ~(mask);
    buf |= op_val;
    ret = write_reg_accord_to_access_mode(mode, reset_info->dev_file_path, map_base_addr,
        addr, pval, size);
    if (ret != 0) {
        DEBUG_ERROR("failed to wite reg, ret: %d\n", ret);
        goto enable_wr_protect;
    }
    DEBUG_INFO("write: addr: 0x%x, width: 0x%zx, wr_val: 0x%x\n", addr, size, buf);

    /* 3. reset delay */
    DEBUG_INFO("reset delay time: %u us\n", op_delay);
    if (op_delay != 0) {
        usleep_range(op_delay + 1, op_delay + 2);
    }
    ret = 0;

enable_wr_protect:
    if (p_reg_info->need_wr_pro) {
        ret = do_resume_wr_pro_ctrl(wr_pro_info, ENABLE_WR_PROTECT);
        if (ret != 0) {
            DEBUG_ERROR("failed to enable wr protect, ret: %d\n", ret);
        }
    }
error:
    return ret;
    
}

static int do_resume_by_op_reg(reg_cfg_info_t *p_reg_info)
{
    int ret;

    /* 1. resume first */
    ret = do_resume_by_op_reg_action(p_reg_info, RESET_OP);
    if (ret) {
        DEBUG_ERROR("failed to do reset action of resume, ret = %d\n", ret);
        return ret;
    }

    /* 2. unreset if needed */
    if (p_reg_info->reset_info.is_dereset) {
        ret = do_resume_by_op_reg_action(p_reg_info, UNRESET_OP);
        if (ret) {
            DEBUG_ERROR("failed to do unreset action of resume, ret = %d\n", ret);
            return ret;
        }
    }

    return 0;
}

static int do_resume_by_op_gpio(gpio_cfg_info_t *p_gpio_info)
{
    int ret;

    DEBUG_INFO("rst: gpio%u output %u level\n", p_gpio_info->gpio_num, p_gpio_info->active_level);
    ret = gpio_direction_output(p_gpio_info->gpio_num, p_gpio_info->active_level);
    if (ret != 0) {
        DEBUG_ERROR("gpio%u output %u fail, ret = %d\n", p_gpio_info->gpio_num, \
                    p_gpio_info->active_level, ret);
        return -EFAULT;
    }

    DEBUG_INFO("reset delay time: %u us\n", p_gpio_info->reset_delay);
    usleep_range(p_gpio_info->reset_delay + 1, p_gpio_info->reset_delay + 2);

    DEBUG_INFO("derst: gpio%u output %u level\n", p_gpio_info->gpio_num, !p_gpio_info->active_level);
    ret = gpio_direction_output(p_gpio_info->gpio_num, !p_gpio_info->active_level);
    if (ret != 0) {
        DEBUG_ERROR("gpio%u output %u fail, ret = %d\n", p_gpio_info->gpio_num, \
                    !p_gpio_info->active_level, ret);
        return -EFAULT;
    }

    DEBUG_INFO("dereset delay time: %u us\n", p_gpio_info->dereset_delay);
    usleep_range(p_gpio_info->reset_delay + 1, p_gpio_info->reset_delay + 2);

    return 0;
}

static dev_resume_info_t* get_dev_resume_info(u32 dev_type, u32 slot_id)
{
    struct list_head *n, *pos;
    dev_resume_info_t *p_dev_resume_info;

    spin_lock(&dev_resume_info_list_lock);
    list_for_each_safe(pos, n, &dev_resume_info_list) {
        p_dev_resume_info = list_entry(pos, dev_resume_info_t, list);
        DEBUG_INFO("dev_type: %u, slot_id: %u\n", p_dev_resume_info->dev_type, p_dev_resume_info->slot_id);
        if ((p_dev_resume_info->dev_type == dev_type) && (p_dev_resume_info->slot_id == slot_id)) {
            spin_unlock(&dev_resume_info_list_lock);
            return p_dev_resume_info;
        }
    }
    spin_unlock(&dev_resume_info_list_lock);

    return NULL;
}

static int c3000_mmc_pwr_resume_by_op_reg(reg_cfg_info_t *p_reg_info)
{
    struct pci_dev *pcidev;
    void __iomem *mmio_addr;
    unsigned long len;
    u32 vid, did;
    int i, ret;
    u32 *saved_reg_info;
    volatile void __iomem *addr;
    u32 reg_num, reg_val;
    static struct mutex lock;
    u32 reg_addr_arr[] = {0xc50728, 0xc5072c, 0xc50738, 0xc5073c,    /* reg: pad_cfg_dw0/1_emmc_cmd/clk */
                          0xc50740, 0xc50744, 0xc50748, 0xc5074c,    /* reg: pad_cfg_dw0/1_emmc_d0-d7 */
                          0xc50750, 0xc50754, 0xc50758, 0xc5075c,
                          0xc50760, 0xc50764, 0xc50768, 0xc5076c,
                          0xc50770, 0xc50774, 0xc50778, 0xc5077c,
                         };

    DEBUG_VERBOSE("start\n");

    vid = C3000_PS2B_PCIE_DEV_VID;
    did = C3000_PS2B_PCIE_DEV_DID;
    pcidev = pci_get_device(vid, did, NULL);
    if (pcidev == NULL) {
        DEBUG_ERROR("get pci dev fail, vid 0x%x, did 0x%x\n", vid, did);
        return -ENODEV;
    }

    len = pci_resource_len(pcidev, 0);
    mmio_addr = pcim_iomap(pcidev, 0, len);
    DEBUG_INFO("mmio_addr = 0x%p, len = 0x%lx\n", mmio_addr, len);
    if (mmio_addr == NULL) {
        DEBUG_ERROR("remap fail, bar0, len: 0x%lx\n", len);
        return -ENOMEM;
    }

    saved_reg_info = kzalloc(sizeof(reg_addr_arr), GFP_KERNEL);
    if (saved_reg_info == NULL) {
        DEBUG_ERROR("kzmalloc fail, size: 0x%zx\n", sizeof(reg_addr_arr));
        pcim_iounmap(pcidev, mmio_addr);
        return -ENOMEM;
    }

    mutex_init(&lock);
    mutex_lock(&lock);

    reg_num = ARRAY_SIZE(reg_addr_arr);
    for (i = 0; i < reg_num; i++) {
        addr = mmio_addr + reg_addr_arr[i];
        saved_reg_info[i] = readl(addr);
        DEBUG_INFO("r addr: 0x%p, info[0]= 0x%x\n", addr, saved_reg_info[i]);
    }

    for (i = 0; i < reg_num; i++) {
        if (i % 2 == 0) {
            reg_val = saved_reg_info[i] & (~(GPIO_RX_DIS_BITS | PAD_MODE_BITS));
        } else {
            reg_val = saved_reg_info[i] & (~PAD_TERM_BITS);
        }
        addr = mmio_addr + reg_addr_arr[i];
        writel(reg_val, addr);
        DEBUG_INFO("w addr: 0x%p, val: [%d]= 0x%x\n", addr, i, reg_val);
    }

    ret = do_resume_by_op_reg(p_reg_info);
    if (ret != 0) {
        DEBUG_ERROR("resume by operate reg fail, ret: %d\n", ret);
        ret = -EIO;
    }

    for (i = 0; i < reg_num; i++) {
        addr = mmio_addr + reg_addr_arr[i];
        reg_val = saved_reg_info[i];
        writel(reg_val, addr);
        DEBUG_INFO("w addr: 0x%p, val: [%d]= 0x%x\n", addr, i, reg_val);
    }

    mutex_unlock(&lock);

    kfree(saved_reg_info);
    pcim_iounmap(pcidev, mmio_addr);

    DEBUG_VERBOSE("end\n");
    return ret;
}

static int dev_resume_only_by_op_reg(reg_cfg_info_t *p_reg_info)
{
    int ret;

    DEBUG_VERBOSE("start\n");

    ret = do_resume_by_op_reg(p_reg_info);
    if (ret != 0) {
        DEBUG_ERROR("resume by operate reg fail, ret: %d\n", ret);
        return -EIO;
    }

    DEBUG_VERBOSE("end\n");
    return 0;
}

static int dev_resume_only_by_op_gpio(gpio_cfg_info_t *p_gpio_info)
{
    int ret;

    DEBUG_VERBOSE("start\n");

    ret = do_resume_by_op_gpio(p_gpio_info);
    if (ret != 0) {
        DEBUG_ERROR("resume by operate gpio fail, ret: %d\n", ret);
        return -EIO;
    }

    DEBUG_VERBOSE("end\n");
    return 0;
}

static int dev_resume_set_emmc_clk(resource_size_t phys_addr, uint32_t mask,
            enum ast_emmc_clk_op_e op)
{
    u8 __iomem *memaddr;
    uint32_t read_val, write_val;

    if ((op != AST_EMMC_CLK_START) && (op != AST_EMMC_CLK_STOP)) {
        DEBUG_ERROR("operation invalid, op = %d\n", op);
        return -EINVAL;
    }

    memaddr = ioremap(phys_addr, 4);
    if (memaddr == NULL) {
        DEBUG_ERROR("ioremap fail\n");
        return -ENOMEM;
    }

    DEBUG_INFO("org_addr: 0x%llx, memaddr: 0x%p\n", phys_addr, memaddr);

    read_val = readl(memaddr);
    if (op == AST_EMMC_CLK_STOP) {
        write_val = read_val & ~(mask);
    } else {
        write_val = read_val | mask;
    }

    DEBUG_INFO("read_val: 0x%x, write_val: 0x%x\n", read_val, write_val);
    writel(write_val, memaddr);
    iounmap(memaddr);

    return 0;
}

static int dev_resume_only_by_ast2500_emmc(ast2500_emmc_cfg_info_t *p_ast2500_emmc_info)
{
    int ret, rv;

    DEBUG_VERBOSE("start\n");

    rv = 0;
    /* Pull the reset (rst) line low to reset the eMMC. */
    ret = gpio_direction_output(p_ast2500_emmc_info->rst_gpio_num,
            p_ast2500_emmc_info->rst_active_level);
    if (ret != 0) {
        printk(KERN_WARNING "gpio%u output %u fail, ret = %d\n", p_ast2500_emmc_info->rst_gpio_num, 
            p_ast2500_emmc_info->rst_active_level, ret);
        rv = -1;
    }

    /* support power cycle by cpld */
    if (p_ast2500_emmc_info->support_powercycle) {
        /* stop emmc clk */
        ret = dev_resume_set_emmc_clk(AST2500_EMMC_128_CTRL0_REG, AST2500_EMMC_128_CTRL0_MASK,
                AST_EMMC_CLK_STOP);
        if (ret != 0) {
            printk(KERN_WARNING "set ast2500 emmc clk to gpio mode fail, ret = %d\n", ret);
            rv = -1;
        }

        /* power off emmc by cpld */
        ret = do_resume_by_op_reg_action(&(p_ast2500_emmc_info->reg_cfg), RESET_OP);
        if (ret != 0) {
            printk(KERN_WARNING "mmc rst resume fail, ret = %d\n", ret);
            rv = -1;
        }

        /* skip delay, process in do_resume_by_op_reg_action */
        DEBUG_INFO("reset delay time: %u us\n", p_ast2500_emmc_info->reset_delay);

        /* power on emmc by cpld */
        ret = do_resume_by_op_reg_action(&(p_ast2500_emmc_info->reg_cfg), UNRESET_OP);
        if (ret != 0) {
            printk(KERN_WARNING "mmc rst resume fail, ret = %d\n", ret);
            rv = -1;
        }

        /* start emmc clk */
        ret = dev_resume_set_emmc_clk(AST2500_EMMC_128_CTRL0_REG, AST2500_EMMC_128_CTRL0_MASK,
                AST_EMMC_CLK_START);
        if (ret != 0) {
            printk(KERN_WARNING "set ast2500 emmc clk to gpio mode fail, ret = %d\n", ret);
            rv = -1;
        }
    }

    DEBUG_INFO("reset delay time: %u us\n", p_ast2500_emmc_info->reset_delay);
    usleep_range(p_ast2500_emmc_info->reset_delay + 1, p_ast2500_emmc_info->reset_delay + 2);

    /* Pull the reset (rst) line up to reset the eMMC. */
    ret = gpio_direction_output(p_ast2500_emmc_info->rst_gpio_num,
            !p_ast2500_emmc_info->rst_active_level);
    if (ret != 0) {
        printk(KERN_WARNING "gpio%u output %u fail, ret = %d\n", p_ast2500_emmc_info->rst_gpio_num, 
            !p_ast2500_emmc_info->rst_active_level, ret);
        rv = -1;
    }

    DEBUG_INFO("dereset delay time: %u us\n", p_ast2500_emmc_info->dereset_delay);
    usleep_range(p_ast2500_emmc_info->dereset_delay + 1, p_ast2500_emmc_info->dereset_delay + 2);

    return rv;
}

static int ast2600_emmc_power_ctrl(ast2600_emmc_cfg_info_t *p_ast2600_emmc_info, resume_power_type_t power_ctrl_type)
{
    int ret;
    logic_attr_t *logic_attr;
    power_gpio_attr_t *gpio_attr;
    u32 power_value;

    /* power ctrl emmc */
    if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_GPIO) {
        gpio_attr = &p_ast2600_emmc_info->power_ctrl_attr.gpio_attr;
        if (power_ctrl_type == POWER_ON) {
            power_value = !gpio_attr->power_active_level;
        } else {
            power_value = gpio_attr->power_active_level;	
        }
        DEBUG_INFO("power ctrl: gpio%u output %u level\n", gpio_attr->power_gpio_num, gpio_attr->power_active_level);
        ret = gpio_direction_output(gpio_attr->power_gpio_num, power_value);
        if (ret != 0) {
            printk(KERN_WARNING "gpio%u output %u fail, ret = %d\n", gpio_attr->power_gpio_num, \
                        power_value, ret);
            ret = -1;
        }

        return ret;
    } else if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_LOGIC) {
        logic_attr = &p_ast2600_emmc_info->power_ctrl_attr.logic_attr;
        if (power_ctrl_type == POWER_ON) {
            power_value = logic_attr->power_on;
        } else {
            power_value = logic_attr->power_off;	
        }
        DEBUG_INFO("power off: logic name %s offset %u value %u\n", logic_attr->dev_name,
                    logic_attr->offset,  power_value);
        ret = resume_logic_write(logic_attr, logic_attr->mask,
                            logic_attr->offset, power_value);
        if (ret < 0) {
            printk(KERN_WARNING "logic %s offset %u value %u write fail, ret = %d\n", logic_attr->dev_name, \
                        logic_attr->offset, power_value, ret);
        }

        return ret;
    }

    printk(KERN_WARNING "not support mode power ctrl, mode %d\n", p_ast2600_emmc_info->power_ctrl_mode);
    return -EINVAL;
}


static int dev_resume_only_by_ast2600_emmc(ast2600_emmc_cfg_info_t *p_ast2600_emmc_info,
    u32 resume_way)
{
    int ret;
    int rv;
    resource_size_t emmc_clk_ctrl_reg;

    DEBUG_VERBOSE("start\n");

    switch (resume_way) {
    case DEV_RESUME_WAY_AST2600:
        emmc_clk_ctrl_reg = AST2600_EMMC_128_CTRL0_REG;
        break;
    case DEV_RESUME_WAY_AST2700:
        emmc_clk_ctrl_reg = AST2700_EMMC_128_CTRL0_REG;
        break;
    default:
        DEBUG_ERROR("unsupported ast26/27 emmc resume way: %u\n", resume_way);
        return -EINVAL;
    }

    rv = 0;
    /* write emmc128 bit8 0, stop emmc clk */
    ret = dev_resume_set_emmc_clk(emmc_clk_ctrl_reg, AST2600_EMMC_128_CTRL0_MASK,
            AST_EMMC_CLK_STOP);
    if (ret != 0) {
        printk(KERN_WARNING "stop ast26/27 emmc clk fail, ret = %d\n", ret);
        rv = -1;
    }

    /* reset emmc */
    if (p_ast2600_emmc_info->rst_gpio_flag) {
        DEBUG_INFO("reset: gpio%u output %u level\n", p_ast2600_emmc_info->rst_gpio_num, p_ast2600_emmc_info->rst_active_level);
        ret = gpio_direction_output(p_ast2600_emmc_info->rst_gpio_num, p_ast2600_emmc_info->rst_active_level);
        if (ret != 0) {
            printk(KERN_WARNING "gpio%u output %u fail, ret = %d\n", p_ast2600_emmc_info->rst_gpio_num, \
                        p_ast2600_emmc_info->rst_active_level, ret);
            rv = -1;
        }
    }

    /* power off emmc */
    ret = ast2600_emmc_power_ctrl(p_ast2600_emmc_info, POWER_OFF);
    if (ret < 0) {
        rv = -1;
    }
    /* delay, wait emmc power off */
    DEBUG_INFO("reset delay time: %u us\n", p_ast2600_emmc_info->reset_delay);
    usleep_range(p_ast2600_emmc_info->reset_delay + 1, p_ast2600_emmc_info->reset_delay + 2);

    /* power up emmc */
    ret = ast2600_emmc_power_ctrl(p_ast2600_emmc_info, POWER_ON);
    if (ret < 0) {
        rv = -1;
    }

    /* unreset emmc */
    if (p_ast2600_emmc_info->rst_gpio_flag) {
        DEBUG_INFO("unreset: gpio%u output %u level\n", p_ast2600_emmc_info->rst_gpio_num, !p_ast2600_emmc_info->rst_active_level);
        ret = gpio_direction_output(p_ast2600_emmc_info->rst_gpio_num, !p_ast2600_emmc_info->rst_active_level);
        if (ret != 0) {
            printk(KERN_WARNING "gpio%u output %u fail, ret = %d\n", p_ast2600_emmc_info->rst_gpio_num, \
                        !p_ast2600_emmc_info->rst_active_level, ret);
            rv = -1;
        }
    }

    /* write emmc128 bit8 1, start emmc clk */
    ret = dev_resume_set_emmc_clk(emmc_clk_ctrl_reg, AST2600_EMMC_128_CTRL0_MASK,
            AST_EMMC_CLK_START);
    if (ret != 0) {
        printk(KERN_WARNING "start ast26/27 emmc clk fail, ret = %d\n", ret);
        rv = -1;
    }

    /* delay, wait emmc chip ready */
    DEBUG_INFO("dereset delay time: %u us\n", p_ast2600_emmc_info->dereset_delay);
    usleep_range(p_ast2600_emmc_info->dereset_delay + 1, p_ast2600_emmc_info->dereset_delay + 2);

    return rv;
}

static int dev_resume_func(dev_resume_info_t *p_dev_resume_info)
{
    int i;
    int ret;
    ulong bit_mask;
    dev_cfg_info_t *p_dev_cfg_info;

    bit_mask = p_dev_resume_info->bit_mask;
    for_each_set_bit(i, &bit_mask, RESUME_WAY_MAX_NUM) {
        DEBUG_INFO("i = %d\n", i);
        p_dev_cfg_info = &p_dev_resume_info->dev_cfg_info[i];

        DEBUG_INFO("resume way = %u\n", p_dev_cfg_info->resume_way);
        switch (p_dev_cfg_info->resume_way) {
            case C3000_RESUME_WAY_WR_REG:
                ret = c3000_mmc_pwr_resume_by_op_reg(&p_dev_cfg_info->info.reg_info);
                if (ret != 0) {
                    DEBUG_ERROR("mmc pwr resume fail, ret = %d\n", ret);
                    ret = -EIO;
                    goto fail;
                }
                DEBUG_INFO("mmc pwr resume complete\n");

                break;
            case DEV_RESUME_WAY2_WR_REG:
                ret = dev_resume_only_by_op_reg(&p_dev_cfg_info->info.reg_info);
                if (ret != 0) {
                    DEBUG_ERROR("mmc rst resume fail, ret = %d\n", ret);
                    ret = -EIO;
                    goto fail;
                }
                DEBUG_INFO("mmc rst resume complete\n");

                break;

            case DEV_RESUME_WAY2_CTL_GPIO:
                ret = dev_resume_only_by_op_gpio(&p_dev_cfg_info->info.gpio_info);
                if (ret != 0) {
                    DEBUG_ERROR("mmc gpio rst resume fail, ret = %d\n", ret);
                    ret = -EIO;
                    goto fail;
                }
                DEBUG_INFO("mmc gpio resume complete\n");

                break;

            case DEV_RESUME_WAY_AST2600:
            case DEV_RESUME_WAY_AST2700:
                ret = dev_resume_only_by_ast2600_emmc(&p_dev_cfg_info->info.ast2600_emmc_info,
                        p_dev_cfg_info->resume_way);
                if (ret != 0) {
                    DEBUG_ERROR("AST26/27 emmc resume fail, resume_way = %u, ret = %d\n",
                        p_dev_cfg_info->resume_way, ret);
                    ret = -EIO;
                    goto fail;
                }
                DEBUG_INFO("AST26/27 emmc resume complete, resume_way = %u\n",
                    p_dev_cfg_info->resume_way);
                break;

            case DEV_RESUME_WAY_AST2500:
                ret = dev_resume_only_by_ast2500_emmc(&p_dev_cfg_info->info.ast2500_emmc_info);
                if (ret != 0) {
                    DEBUG_ERROR("AST2500 emmc resume fail, ret = %d\n", ret);
                    ret = -EIO;
                    goto fail;
                }
                DEBUG_INFO("AST2500 emmc resume complete\n");
                break;

            default:
                DEBUG_INFO("unachieved resume way: %u\n", p_dev_cfg_info->resume_way);
                ret = -ENOSYS;
                goto fail;
        }
    }

fail:
    return ret;
}

/************************** start: Externally registered interface **********************************/
static int dev_resume(u32 dev_type, u32 slot_id)
{
    dev_resume_info_t *p_dev_resume_info;
    int ret;

    DEBUG("start\n");
    DEBUG("dev_type: %u, slot_id: %u\n", dev_type, slot_id);

    /* 1. Interrupt context stack trace alert notification. */
    WARN(in_interrupt(), "should not used in interrupt context\n");

    /* 2. Obtain device recovery information. */
    p_dev_resume_info = get_dev_resume_info(dev_type, slot_id);
    if (p_dev_resume_info == NULL) {
        DEBUG_ERROR("dev info is not existed\n");
        return -EEXIST;
    }

    if ((p_dev_resume_info->resume_enable == 0)) {
        DEBUG_WARN("skip dev_resume for emmc when disabled, slot_id: %u\n",
                   p_dev_resume_info->slot_id);
        return 0;
    }

    mutex_lock(&p_dev_resume_info->resume_lock);
    /* 3. Traverse the bitmask of recovery methods to determine which recovery methods need to be executed, and bind the recovery methods to specific recovery interfaces. */
    ret = dev_resume_func(p_dev_resume_info);
    mutex_unlock(&p_dev_resume_info->resume_lock);
    if (ret != 0) {
        DEBUG_ERROR("dev_resume_func fail, ret = %d\n", ret);
    }

    DEBUG("end\n");
    return ret;
}

/************************** end: Externally registered interface **********************************/

static int dev_resume_write_pro_init(struct platform_device *pdev, struct device_node *dev_node, wr_pro_ctrl_info_t *wr_pro_info)
{
    int ret;
    u32 width;

    ret  = of_property_read_u32(dev_node, "wr_pro_reg_addr",  &wr_pro_info->reg_addr);
    ret |= of_property_read_u32(dev_node, "wr_pro_reg_width", &wr_pro_info->reg_width);
    ret |= of_property_read_u32(dev_node, "wr_pro_access_mode", &wr_pro_info->access_mode);
    DEBUG_INFO("wr pro access_mode: %u, addr: 0x%x, width: %u", wr_pro_info->access_mode, 
                wr_pro_info->reg_addr, wr_pro_info->reg_width);

    ret |= of_property_read_u32(dev_node, "disable_wr_pro_val", &wr_pro_info->disable_wr_pro_val);
    ret |= of_property_read_u32(dev_node, "enable_wr_pro_val",  &wr_pro_info->enable_wr_pro_val);
    ret |= of_property_read_u32(dev_node, "wr_pro_mask",      &wr_pro_info->mask);
    DEBUG_INFO("wr pro close val: 0x%x, open val: 0x%x, mask: 0x%x, ret: %d\n", 
                wr_pro_info->disable_wr_pro_val, wr_pro_info->enable_wr_pro_val, 
                wr_pro_info->mask, ret);
    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to obtain the write protection configuration, ret = %d\n", ret);
        return -EFAULT;
    }

    /* 1.3.2 parse file path & base addr */
    wr_pro_info->dev_file_path = NULL;
    wr_pro_info->map_base_addr = 0;
    (void)of_property_read_string(dev_node, "wr_pro_dev_file_path", &wr_pro_info->dev_file_path);
    (void)of_property_read_u64(dev_node,    "wr_pro_map_base_addr", &wr_pro_info->map_base_addr);

    /* 1.3.2.1 check parameter validity */
    if (wr_pro_info->access_mode >= ACCESS_MODE_END) {
        dev_err(&pdev->dev, "wr_pro_access_mode: %u is invalid\n", wr_pro_info->access_mode);
        return -EFAULT;
    }

    /* 1.3.2.2 check if the file path is configured when mode is ACCESS_MODE_DEV_FILE */
    if ((wr_pro_info->access_mode == ACCESS_MODE_DEV_FILE) && 
        (wr_pro_info->dev_file_path == NULL)) {
        dev_err(&pdev->dev, "invalid wr pro dev file name\n");
        return -EFAULT;
    }

    /* 1.3.2.3 check if the base addr is configured when mode is ACCESS_MODE_MEM */
    if ((wr_pro_info->access_mode == ACCESS_MODE_MEM) && 
        (wr_pro_info->map_base_addr == 0)) {
        dev_err(&pdev->dev, "invalid wr pro map base addr\n");
        return -EFAULT;
    }

    /* 1.3.2.4 The register bit width only supports 1/2/4 B */
    width = wr_pro_info->reg_width;
    if ((width != 4) && (width != 2) && (width != 1)) {
        dev_err(&pdev->dev, "invalid wr pro reg width: %u\n", width);
        return -EFAULT;
    }

    DEBUG_INFO("reg info wr pro all param init succeed\n");

    return 0;
}
static int mmc_resume_power_logic_init(struct platform_device *pdev, struct device_node *child,
        ast2600_emmc_cfg_info_t *p_ast2600_emmc_info)
{
    int ret;
    logic_attr_t *logic_attr;

    logic_attr = &p_ast2600_emmc_info->power_ctrl_attr.logic_attr;
    ret = of_property_read_string(child, "dev_name", &logic_attr->dev_name);
    ret |= of_property_read_u32(child, "offset", &logic_attr->offset);
    ret |= of_property_read_u32(child, "power_on", &logic_attr->power_on);
    ret |= of_property_read_u32(child, "power_off", &logic_attr->power_off);
    ret |= of_property_read_u32(child, "logic_func_mode", &logic_attr->logic_func_mode);
    ret |= of_property_read_u32(child, "mask", &logic_attr->mask);
    
    if (ret) {
        dev_err(&pdev->dev, "parse dts logic config fail, ret = %d\n", ret);
        return -EFAULT;
    }
    ret = of_property_read_u32(child, "width", &logic_attr->width);
    if (ret == 0) {
        if ((logic_attr->width != WIDTH_1Byte) && (logic_attr->width != WIDTH_4Byte)) {
            DEBUG_ERROR("logic config error, witdh: %u not support.\n", logic_attr->width);
            return -EINVAL;
        }
    } else {
        logic_attr->width = WIDTH_1Byte;
    }
    
    ret = find_intf_addr(&logic_attr->write_intf_addr, &logic_attr->read_intf_addr, logic_attr->logic_func_mode);
    if (ret) {
        DEBUG_ERROR("find_intf_addr func mode %d fail, rv: %d.\n", logic_attr->logic_func_mode, ret);
        return ret;
    }
    
    if (!logic_attr->write_intf_addr || !logic_attr->read_intf_addr) {
        DEBUG_ERROR("Fail: func mode %u rw symbol undefined.\n", logic_attr->logic_func_mode);
        return -ENOSYS;
    }
    DEBUG_VERBOSE("dev_name:%s, mask:0x%x, reset_on:0x%x, reset_off:0x%x, width: %u, logic_func_mode: %d.\n",
        logic_attr->dev_name, logic_attr->mask,
        logic_attr->power_on, logic_attr->power_off, logic_attr->width,
        logic_attr->logic_func_mode);

    return 0;
}

static int mmc_resume_ast2600_emmc_info_init(struct platform_device *pdev, struct device_node *child,
        ast2600_emmc_cfg_info_t *p_ast2600_emmc_info)
{
    int ret;

    ret = of_property_read_u32(child, "rst_gpio_flag", &p_ast2600_emmc_info->rst_gpio_flag);
    if (ret != 0) {
        dev_err(&pdev->dev, "parse dts rst_gpio_flag fail, ret = %d\n", ret);
        return -EFAULT;
    }
    if (p_ast2600_emmc_info->rst_gpio_flag) {
        ret = of_property_read_u32(child, "rst_gpio_num", &p_ast2600_emmc_info->rst_gpio_num);
        ret |= of_property_read_u32(child, "rst_active_level", &p_ast2600_emmc_info->rst_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "parse dts rst_gpio config fail, ret = %d\n", ret);
            return -EFAULT;
        }
        DEBUG_INFO("rst_gpio_flag: %u, rst_gpio_num: %u, rst_active_level: %u\n",
               p_ast2600_emmc_info->rst_gpio_flag, p_ast2600_emmc_info->rst_gpio_num,
               p_ast2600_emmc_info->rst_active_level);
    }

    ret = of_property_read_u32(child, "power_ctrl_mode", &p_ast2600_emmc_info->power_ctrl_mode);
    if (ret != 0) {
        p_ast2600_emmc_info->power_ctrl_mode = POWER_CTRL_GPIO;
    }
    if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_LOGIC) {
        DEBUG_VERBOSE("power ctrl by logic dev.\n");
        ret = mmc_resume_power_logic_init(pdev, child, p_ast2600_emmc_info);
        if (ret != 0) {
            DEBUG_ERROR("mmc_resume_power_logic_init fail, rv: %d.\n", ret);
            return ret;
        }
    } else if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_GPIO) {
        ret = of_property_read_u32(child, "power_gpio_num", &p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num);
        ret |= of_property_read_u32(child, "power_active_level", &p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "parse dts power_gpio config fail, ret = %d\n", ret);
            return -EFAULT;
        }
        DEBUG_INFO("power_ctrl_mode: %u, power_gpio_num: %u, power_active_level: %u\n",
               p_ast2600_emmc_info->power_ctrl_mode, p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num,
               p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_active_level);
    } else {
        dev_err(&pdev->dev, "not support power ctrl mode, mode = %d\n", p_ast2600_emmc_info->power_ctrl_mode);
        return -EINVAL;
    }

    ret = of_property_read_u32(child, "reset_delay", &p_ast2600_emmc_info->reset_delay);
    ret |= of_property_read_u32(child, "dereset_delay", &p_ast2600_emmc_info->dereset_delay);
    DEBUG_INFO("rst_gpio_flag: %u, power_ctrl_mode: %u, reset_delay: %u, dereset_delay: %u\n",
               p_ast2600_emmc_info->rst_gpio_flag, p_ast2600_emmc_info->power_ctrl_mode,
               p_ast2600_emmc_info->reset_delay, p_ast2600_emmc_info->dereset_delay);
    if (ret != 0) {
        dev_err(&pdev->dev, "parse dts prop fail, ret = %d\n", ret);
        return -EFAULT;
    }

    if (p_ast2600_emmc_info->rst_gpio_flag) {
        ret = gpio_request(p_ast2600_emmc_info->rst_gpio_num, "mmc_rst_gpio");
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_rst_gpio gpio%u request fail, ret = %d\n", p_ast2600_emmc_info->rst_gpio_num, ret);
            return -EFAULT;
        }

        ret = gpio_direction_output(p_ast2600_emmc_info->rst_gpio_num, !p_ast2600_emmc_info->rst_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_rst_gpio gpio%u output %u fail, ret = %d\n", p_ast2600_emmc_info->rst_gpio_num, \
                    !p_ast2600_emmc_info->rst_active_level, ret);
            goto free_rst_gpio;
        }
    }

    if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_GPIO) {
        ret = gpio_request(p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num, "mmc_power_gpio");
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_power_gpio gpio%u request fail, ret = %d\n", p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num, ret);
            goto free_rst_gpio;
        }

        ret = gpio_direction_output(p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num, !p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_power_gpio gpio%u output %u fail, ret = %d\n", p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num, \
                    !p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_active_level, ret);
            goto free_power_gpio;
        }
    }

    return 0;

free_power_gpio:
    if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_GPIO) {
        gpio_free(p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num);
    }

free_rst_gpio:
    if (p_ast2600_emmc_info->rst_gpio_flag) {
        gpio_free(p_ast2600_emmc_info->rst_gpio_num);
    }
    return -EFAULT;
}

static int mmc_resume_ast2500_emmc_info_init(struct platform_device *pdev, 
            struct device_node *child, ast2500_emmc_cfg_info_t *p_ast2500_emmc_info)
{
    int ret;
    struct device_node *grandchild;

    ret = of_property_read_u32(child, "rst_gpio_flag", &p_ast2500_emmc_info->rst_gpio_flag);
    if (ret != 0) {
        dev_err(&pdev->dev, "parse dts rst_gpio_flag fail, ret = %d\n", ret);
        return -EFAULT;
    }
    if (p_ast2500_emmc_info->rst_gpio_flag) {
        ret = of_property_read_u32(child, "rst_gpio_num", &p_ast2500_emmc_info->rst_gpio_num);
        ret |= of_property_read_u32(child, "rst_active_level", 
                    &p_ast2500_emmc_info->rst_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "parse dts rst_gpio config fail, ret = %d\n", ret);
            return -EFAULT;
        }
        DEBUG_INFO("rst_gpio_flag: %u, rst_gpio_num: %u, rst_active_level: %u\n",
               p_ast2500_emmc_info->rst_gpio_flag, p_ast2500_emmc_info->rst_gpio_num,
               p_ast2500_emmc_info->rst_active_level);
    }

    if (of_property_read_bool(child, "support_powercycle")) {
        p_ast2500_emmc_info->support_powercycle = true;
    } else {
        p_ast2500_emmc_info->support_powercycle = false;
    }

    if (p_ast2500_emmc_info->support_powercycle) {
        grandchild = of_get_next_child(child, NULL);
        if (grandchild == NULL) {
            dev_err(&pdev->dev, "parse dts cpld config node fail, ret = %d\n", ret);
            return -EFAULT;
        }

        ret = dev_resume_info_register_init(pdev, grandchild, 
                &(p_ast2500_emmc_info->reg_cfg));
        of_node_put(grandchild);
        if (ret != 0) {
            dev_err(&pdev->dev, "parse dts reg cfg info config fail, ret = %d\n", ret);
            return -EFAULT;
        }
    }

    ret = of_property_read_u32(child, "reset_delay", &p_ast2500_emmc_info->reset_delay);
    ret |= of_property_read_u32(child, "dereset_delay", &p_ast2500_emmc_info->dereset_delay);
    DEBUG_INFO("rst_gpio_flag: %u, reset_delay: %u, dereset_delay: %u\n",
               p_ast2500_emmc_info->rst_gpio_flag, p_ast2500_emmc_info->reset_delay,
               p_ast2500_emmc_info->dereset_delay);
    if (ret != 0) {
        dev_err(&pdev->dev, "parse dts prop fail, ret = %d\n", ret);
        return -EFAULT;
    }

    if (!(p_ast2500_emmc_info->rst_gpio_flag)) {
        dev_err(&pdev->dev, "rst_gpio_flag %u, invalid, init fail.\n",
                p_ast2500_emmc_info->rst_gpio_flag);
        return -EFAULT;
    }

    if (p_ast2500_emmc_info->rst_gpio_flag) {
        ret = gpio_request(p_ast2500_emmc_info->rst_gpio_num, "mmc_rst_gpio");
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_rst_gpio gpio%u request fail, ret = %d\n", p_ast2500_emmc_info->rst_gpio_num, ret);
            return -EFAULT;
        }

        ret = gpio_direction_output(p_ast2500_emmc_info->rst_gpio_num,
                !p_ast2500_emmc_info->rst_active_level);
        if (ret != 0) {
            dev_err(&pdev->dev, "mmc_rst_gpio gpio%u output %u fail, ret = %d\n",
                p_ast2500_emmc_info->rst_gpio_num, !p_ast2500_emmc_info->rst_active_level, ret);
            goto free_rst_gpio;
        }
    }

    return 0;
free_rst_gpio:
    if (p_ast2500_emmc_info->rst_gpio_flag) {
        gpio_free(p_ast2500_emmc_info->rst_gpio_num);
    }
    return -EFAULT;
}

/*
 * Used to initialize the reset configuration for writing register method. 
 */
static int dev_resume_info_register_init(struct platform_device *pdev, struct device_node *child, reg_cfg_info_t *p_reg_info)
{
    reset_ctrl_info_t *p_reset_info;
    u32 width;
    int ret;

    p_reset_info = &p_reg_info->reset_info;
    ret  = of_property_read_u32(child, "reg_addr",      &p_reset_info->reg_addr);
    ret |= of_property_read_u32(child, "reg_width",     &p_reset_info->reg_width);
    ret |= of_property_read_u32(child, "access_mode",   &p_reset_info->access_mode);
    DEBUG_INFO("access_mode: %u, addr: 0x%x, width: %u", p_reset_info->access_mode,
                p_reset_info->reg_addr, p_reset_info->reg_width);

    ret |= of_property_read_u32(child, "reset_val",     &p_reset_info->reset_val);
    ret |= of_property_read_u32(child, "dereset_val",   &p_reset_info->dereset_val);
    ret |= of_property_read_u32(child, "reset_mask",    &p_reset_info->reset_mask);
    ret |= of_property_read_u32(child, "reset_delay",   &p_reset_info->reset_delay);
    ret |= of_property_read_u32(child, "dereset_delay", &p_reset_info->dereset_delay);
    DEBUG_INFO("rst_val: 0x%x, derst_val: 0x%x, mask: 0x%x, rst_delay: %u, " \
                "derst_delay: %u, ret: %d\n",
                p_reset_info->reset_val, p_reset_info->dereset_val, p_reset_info->reset_mask,
                p_reset_info->reset_delay, p_reset_info->dereset_delay, ret);

    if (ret != 0) {
        dev_err(&pdev->dev, "failed to parse props of dts file, ret: %d\n", ret);
        return -EFAULT;
    }

    /* 1.3.2 Parse the optional information */
    p_reset_info->dev_file_path = NULL;
    p_reset_info->map_base_addr = 0;
    p_reset_info->is_dereset = 1;
    (void)of_property_read_string(child, "dev_file_path", &p_reset_info->dev_file_path);
    (void)of_property_read_u64(child,    "map_base_addr", &p_reset_info->map_base_addr);
    (void)of_property_read_u32(child,    "is_dereset",    &p_reset_info->is_dereset);

    /* 1.3.2 Validity check */
    if (p_reset_info->access_mode >= ACCESS_MODE_END) {
        dev_err(&pdev->dev, "access_mode: %u is invalid\n", p_reset_info->access_mode);
        return -EFAULT;
    }

    if ((p_reset_info->access_mode == ACCESS_MODE_DEV_FILE) &&
        (p_reset_info->dev_file_path == NULL)) {
        dev_err(&pdev->dev, "invalid dev file name\n");
        return -EFAULT;
    }

    if ((p_reset_info->access_mode == ACCESS_MODE_MEM) &&
        (p_reset_info->map_base_addr == 0)) {
        dev_err(&pdev->dev, "invalid map base addr\n");
        return -EFAULT;
    }

    width = p_reset_info->reg_width;
    if ((width != 4) && (width != 2) && (width != 1)) {
        dev_err(&pdev->dev, "invalid reg width: %u\n", width);
        return -EFAULT;
    }

    DEBUG_INFO("reg info basic param init succeed\n");
    p_reg_info->need_wr_pro = of_property_read_bool(child, "need_wr_pro");
    if (p_reg_info->need_wr_pro) {
        ret = dev_resume_write_pro_init(pdev, child, &p_reg_info->wr_pro_info);
    }

    if (ret != 0) {
        dev_err(&pdev->dev, "Failed to initialize the resume info parameters, ret = %d\n", ret);
        return ret;
    }
    return 0;
}

static int dev_resume_info_init(struct platform_device *pdev)
{
    dev_resume_info_t *p_dev_resume_info;
    struct device_node *child;
    int i, ret;
    gpio_cfg_info_t *p_gpio_info;
    ast2600_emmc_cfg_info_t *p_ast2600_emmc_info;
    ast2500_emmc_cfg_info_t *p_ast2500_emmc_info;
    dev_cfg_info_t *p_dev_cfg_info;
    u32 resume_way;

    DEBUG_VERBOSE("start\n");

    p_dev_resume_info = platform_get_drvdata(pdev);

    /* 1. Traverse all recovery methods of the device and initialize the corresponding device recovery configuration information. */
    i = 0;
    for_each_child_of_node(pdev->dev.of_node, child) {
        /* 1.1 Obtain the recovery methods of the device and assess their validity. */
        resume_way = 0;
        ret = of_property_read_u32(child, "resume_way", &resume_way);
        DEBUG_INFO("i:%d, resume_way: %u\n", i, resume_way);
        if (ret != 0) {
            dev_err(&pdev->dev, "parse resume_way fail\n");
            return -EINVAL;
        }

        if (resume_way >= DEV_RESUME_WAYX_END) {
            dev_err(&pdev->dev, "resume_way: %u is invalid\n", resume_way);
            return -EINVAL;
        }

        /* 1.2 Obtain the dev configuration information and initialize it */
        p_dev_cfg_info = &p_dev_resume_info->dev_cfg_info[i];
        p_dev_cfg_info->resume_way = resume_way;

        /* 1.3 Based on the recovery methods, fill in the necessary configuration information for the corresponding dev recovery methods. */
        switch (resume_way) {
            case C3000_RESUME_WAY_WR_REG:
            case DEV_RESUME_WAY2_WR_REG:
                /* 1.3.1 Parse the essential information */
                ret = dev_resume_info_register_init(pdev, child, &p_dev_cfg_info->info.reg_info);
                if (ret != 0) {
                    dev_err(&pdev->dev, "failed to parse dts prop, resume_way: %d ret = %d\n", resume_way, ret);
                    return -EFAULT;
                }
                break;
            case DEV_RESUME_WAY2_CTL_GPIO:
                p_gpio_info = &p_dev_cfg_info->info.gpio_info;
                ret  = of_property_read_u32(child, "gpio_num",      &p_gpio_info->gpio_num);
                ret |= of_property_read_u32(child, "active_level",  &p_gpio_info->active_level);
                ret |= of_property_read_u32(child, "reset_delay",   &p_gpio_info->reset_delay);
                ret |= of_property_read_u32(child, "dereset_delay", &p_gpio_info->dereset_delay);
                DEBUG_INFO("gpio_num: %u, active_level: %u, reset_delay: %u, dereset_delay: %u\n",      \
                           p_gpio_info->gpio_num, p_gpio_info->active_level, p_gpio_info->reset_delay, \
                           p_gpio_info->dereset_delay);
                if (ret != 0) {
                    dev_err(&pdev->dev, "parse dts prop fail, ret = %d\n", ret);
                    return -EFAULT;
                }

                ret = gpio_request(p_gpio_info->gpio_num, "dev_rst_gpio");
                if (ret != 0) {
                    dev_err(&pdev->dev, "gpio%u request fail, ret = %d\n", p_gpio_info->gpio_num, ret);
                    return -EFAULT;
                }

                ret = gpio_direction_output(p_gpio_info->gpio_num, !p_gpio_info->active_level);
                if (ret != 0) {
                    gpio_free(p_gpio_info->gpio_num);
                    dev_err(&pdev->dev, "gpio%u output %u fail, ret = %d\n", p_gpio_info->gpio_num, \
                            !p_gpio_info->active_level, ret);
                    return -EFAULT;
                }

                break;
            case DEV_RESUME_WAY_AST2600:
            case DEV_RESUME_WAY_AST2700:
                p_ast2600_emmc_info = &p_dev_cfg_info->info.ast2600_emmc_info;
                ret = mmc_resume_ast2600_emmc_info_init(pdev, child, p_ast2600_emmc_info);
                if (ret != 0) {
                    dev_err(&pdev->dev, "mmc_resume_ast2600_emmc_info_init fail, ret = %d\n", ret);
                    return -EFAULT;
                }
                break;
            case DEV_RESUME_WAY_AST2500:
                p_ast2500_emmc_info = &p_dev_cfg_info->info.ast2500_emmc_info;
                ret = mmc_resume_ast2500_emmc_info_init(pdev, child, p_ast2500_emmc_info);
                if (ret != 0) {
                    dev_err(&pdev->dev, "mmc_resume_ast2500_emmc_info_init fail, ret = %d\n", ret);
                    return -EFAULT;
                }
                break;
            default:
                dev_err(&pdev->dev, "unknown resume type: %u\n", resume_way);
                return -EFAULT;
        }

        i++;
    }

    DEBUG_VERBOSE("end\n");
    return 0;
}

static void dev_resume_info_deinit(struct platform_device *pdev)
{
    dev_resume_info_t *p_dev_resume_info;
    gpio_cfg_info_t *p_gpio_info;
    ast2600_emmc_cfg_info_t *p_ast2600_emmc_info;
    ast2500_emmc_cfg_info_t *p_ast2500_emmc_info;
    dev_cfg_info_t *p_dev_cfg_info;
    u32 resume_way;
    int i;

    DEBUG_VERBOSE("start\n");

    p_dev_resume_info = platform_get_drvdata(pdev);

    DEBUG_INFO("resume_way_num: %u\n", p_dev_resume_info->resume_way_num);
    for (i = 0; i < p_dev_resume_info->resume_way_num; i++) {
        p_dev_cfg_info = &p_dev_resume_info->dev_cfg_info[i];
        resume_way =  p_dev_cfg_info->resume_way;
        switch (resume_way) {
            case DEV_RESUME_WAY2_CTL_GPIO:
                p_gpio_info = &p_dev_cfg_info->info.gpio_info;
                DEBUG_INFO("i: %d, resume_way: %u, gpio_num: %u\n", i, resume_way,
                           p_gpio_info->gpio_num);
                gpio_free(p_gpio_info->gpio_num);
                break;
            case DEV_RESUME_WAY_AST2600:
            case DEV_RESUME_WAY_AST2700:
                p_ast2600_emmc_info = &p_dev_cfg_info->info.ast2600_emmc_info;
                DEBUG_INFO("i: %d, resume_way: %u\n", i, resume_way);
                if (p_ast2600_emmc_info->rst_gpio_flag) {
                    DEBUG_INFO("free rst_gpio_num%u\n", p_ast2600_emmc_info->rst_gpio_num);
                    gpio_free(p_ast2600_emmc_info->rst_gpio_num);
                }
                if (p_ast2600_emmc_info->power_ctrl_mode == POWER_CTRL_GPIO) {
                    DEBUG_INFO("free power_gpio_num%u\n", p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num);
                    gpio_free(p_ast2600_emmc_info->power_ctrl_attr.gpio_attr.power_gpio_num);
                }
                break;
            case DEV_RESUME_WAY_AST2500:
                p_ast2500_emmc_info = &p_dev_cfg_info->info.ast2500_emmc_info;
                DEBUG_INFO("i: %d, resume_way: %u\n", i, resume_way);
                if (p_ast2500_emmc_info->rst_gpio_flag) {
                    DEBUG_INFO("free rst_gpio_num%u\n", p_ast2500_emmc_info->rst_gpio_num);
                    gpio_free(p_ast2500_emmc_info->rst_gpio_num);
                }
                break;
            default:
                DEBUG_INFO("no need deinit's resume way: %u\n", resume_way);
        }
    }

    DEBUG_VERBOSE("end\n");
    return;
}

static int reg_intf_accord_to_dev_type(struct platform_device *pdev)
{
    int ret;
    dev_resume_info_t *p_dev_resume_info;
    reg_dev_resume_func_t pfunc;

    DEBUG_VERBOSE("start\n");

    p_dev_resume_info = platform_get_drvdata(pdev);
    DEBUG_INFO("dev_type: %u, reg_func: %s\n", \
               p_dev_resume_info->dev_type, p_dev_resume_info->reg_intf_name);

    pfunc = (reg_dev_resume_func_t)p_dev_resume_info->reg_intf_addr;
    ret = pfunc((dev_resume_func_t)dev_resume);
    if (ret != 0) {
        dev_err(&pdev->dev, "failed to resume intf ret: %d\n", ret);
        return -EFAULT;
    }    

    DEBUG_VERBOSE("end\n");
    return 0;
}

static int unreg_intf(struct platform_device *pdev)
{
    int ret;
    dev_resume_info_t *p_dev_resume_info;
    unreg_dev_resume_func_t pfunc;

    p_dev_resume_info = platform_get_drvdata(pdev);
    DEBUG_INFO("unreg_func: %s\n", p_dev_resume_info->unreg_intf_name);

    pfunc = (unreg_dev_resume_func_t)p_dev_resume_info->unreg_intf_addr;
    ret = pfunc();
    if (ret != 0) {
        dev_err(&pdev->dev, "unreg intf: %s fail, ret: %d, \n", p_dev_resume_info->unreg_intf_name, ret);
        return -EFAULT;
    }

    return 0;
}

/********************************* start: sysfs-test ****************************************/
static ssize_t wb_show_resume_bit_mask(struct device *dev, struct device_attribute *attr, char *buf)
{
    dev_resume_info_t *p_info;
    ssize_t size;

    p_info = (dev_resume_info_t *)dev_get_drvdata(dev);
    DEBUG_INFO("p_info: 0x%p, dev_type: %u, slot_id: %u\n", p_info, p_info->dev_type, p_info->slot_id);

    size = snprintf(buf, PAGE_SIZE, "dev_type: %u, slot_id: %u, bit_mask: 0x%x\n", \
                    p_info->dev_type, p_info->slot_id, p_info->bit_mask);

    return size;
}

static ssize_t wb_set_resume_bit_mask(struct device *dev, struct device_attribute *attr,
                                      const char *buf, size_t count)
{
    dev_resume_info_t *p_info;

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    p_info = (dev_resume_info_t *)dev_get_drvdata(dev);
    DEBUG_INFO("p_info: 0x%p, dev_type: %u, slot_id: %u\n", p_info, p_info->dev_type, p_info->slot_id);

    DEBUG_INFO("current bit_mask: 0x%x\n", p_info->bit_mask);
    p_info->bit_mask = (u32)simple_strtoul(buf, NULL, 0);
    DEBUG_INFO("setted bit_mask: 0x%x\n", p_info->bit_mask);

    return count;
}

static ssize_t wb_show_resume_enable(struct device *dev,
                                          struct device_attribute *attr, char *buf)
{
    dev_resume_info_t *p_info;

    p_info = (dev_resume_info_t *)dev_get_drvdata(dev);
    DEBUG_INFO("p_info: 0x%p, dev_type: %u, slot_id: %u, resume_enable: %u\n",
               p_info, p_info->dev_type, p_info->slot_id, p_info->resume_enable);

    return snprintf(buf, PAGE_SIZE, "%u\n", p_info->resume_enable);
}

static ssize_t wb_set_resume_enable(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t count)
{
    dev_resume_info_t *p_info;
    unsigned long value;
    int ret;

    if (count == 0) {
        DEBUG_ERROR("input is null\n");
        return -EINVAL;
    }

    p_info = (dev_resume_info_t *)dev_get_drvdata(dev);
    ret = kstrtoul(buf, 0, &value);
    if (ret < 0) {
        DEBUG_ERROR("parse resume_enable fail, ret: %d\n", ret);
        return ret;
    }

    if ((value != 0) && (value != 1)) {
        dev_warn(dev, "Unsupported input: %lu, only 0 or 1\n", value);
        return -EINVAL;
    }

    mutex_lock(&p_info->resume_lock);
    p_info->resume_enable = (u32)value;
    mutex_unlock(&p_info->resume_lock);
    dev_info(dev, "resume_enable set to %u\n", p_info->resume_enable);

    return count;
}

/* Used to test the external exception recovery interface */
static ssize_t wb_resume_intf_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    dev_resume_info_t *p_info;
    int ret;
    long value;

    value = 0;
    ret = kstrtol(buf, 0, &value);
    if (ret < 0) {
        DEBUG_INFO("input value fail, ret %d\n", ret);
        return ret;
    }

    if (value != 1) {
        dev_warn(dev, "Unsupported input\n");
        return -EINVAL;
    }

    p_info = (dev_resume_info_t *)dev_get_drvdata(dev);
    DEBUG_INFO("p_info: 0x%p, dev_type: %u, slot_id: %u\n", p_info, p_info->dev_type, p_info->slot_id);
    dev_warn(dev, "Execute dev_resume test\n");

    ret = dev_resume(p_info->dev_type, p_info->slot_id);
    if (ret != 0) {
        DEBUG_ERROR("failed to do dev_resume test, dev_type: %d ret: %d\n",
            p_info->dev_type, ret);
        return ret;
    }

    return count;
}

static struct device_attribute dev_attr_resume_bit_mask;
static struct device_attribute dev_attr_dev_resume;
static struct device_attribute dev_attr_resume_enable;

static DEVICE_ATTR(resume_bit_mask, S_IWUSR | S_IRUSR | S_IRGRP, \
                   wb_show_resume_bit_mask, wb_set_resume_bit_mask);
static DEVICE_ATTR(dev_resume, S_IWUSR, NULL, wb_resume_intf_test);
static DEVICE_ATTR(resume_enable, S_IWUSR | S_IRUSR | S_IRGRP,
                   wb_show_resume_enable, wb_set_resume_enable);

static struct attribute *dev_attrs[] = {
    &dev_attr_resume_bit_mask.attr,
    &dev_attr_dev_resume.attr,
    &dev_attr_resume_enable.attr,
    NULL,
};

static struct attribute_group dev_attr_group = {
    .attrs = dev_attrs,
};
/********************************* end: sysfs-test ****************************************/

static int dev_resume_probe(struct platform_device *pdev)
{
    dev_resume_info_t *p_dev_resume_info, *p_info;
    u32 dev_type, slot_id, resume_way_num, bit_mask;
    u32 resume_enable;
    const char *reg_intf_name, *unreg_intf_name;
    unsigned long reg_intf_addr, unreg_intf_addr;
    int ret;

    DEBUG_VERBOSE("start\n");

    if (of_have_populated_dt() == 0) {
        dev_err(&pdev->dev, "unsupport dts parse\n");
        return -EFAULT;
    }

    dev_type = 0;
    slot_id  = 0;
    bit_mask = 0;
    resume_enable = 1;
    reg_intf_name   = NULL;
    unreg_intf_name = NULL;
    ret  = of_property_read_u32(pdev->dev.of_node, "dev_type", &dev_type);
    ret |= of_property_read_u32(pdev->dev.of_node, "slot_id",  &slot_id);
    ret |= of_property_read_u32(pdev->dev.of_node, "bit_mask", &bit_mask);
    ret |= of_property_read_string(pdev->dev.of_node, "reg_intf_name", &reg_intf_name);
    ret |= of_property_read_string(pdev->dev.of_node, "unreg_intf_name", &unreg_intf_name);
    DEBUG_INFO("dev_type: %u, slot_id: %u, bit_mask: 0x%x, reg_intf_name: %s, "     \
               "unreg_intf_name: %s, ret = %d\n",                                   \
               dev_type, slot_id, bit_mask, reg_intf_name ? reg_intf_name : "null", \
               unreg_intf_name ? unreg_intf_name : "null", ret);
    if (ret != 0) {
        dev_err(&pdev->dev, "read dts propert fail, ret = %d\n", ret);
        return -EFAULT;
    }

    if (dev_type >= DEV_TYPE_END) {
        dev_err(&pdev->dev, "dev_type is invalid, %u\n", dev_type);
        return -EINVAL;
    }

    if ((bit_mask == 0) || (bit_mask >= (1 << RESUME_WAY_MAX_NUM))) {
        dev_err(&pdev->dev, "bit_mask is invalid, %u\n", bit_mask);
        return -EINVAL;
    }

    reg_intf_addr = (unsigned long)kallsyms_lookup_name_fun(reg_intf_name);
    if (reg_intf_addr == 0) {
        dev_err(&pdev->dev, "reg_intf %s is not exist\n", reg_intf_name);
        return -EFAULT;
    }

    unreg_intf_addr = (unsigned long)kallsyms_lookup_name_fun(unreg_intf_name);
    if (unreg_intf_addr == 0) {
        dev_err(&pdev->dev, "unreg_intf %s is not exist\n", unreg_intf_name);
        return -EFAULT;
    }

    /* Confirm how many recovery methods are available for this device */
    resume_way_num = of_get_child_count(pdev->dev.of_node);
    DEBUG_INFO("resume_way_num: %u\n", resume_way_num);
    if ((resume_way_num == 0) || (resume_way_num >= RESUME_WAY_MAX_NUM)) {
        dev_err(&pdev->dev, "resume_way_num: %u is invalid\n", resume_way_num);
        return -EFAULT;
    }

    /* serach resume_info_list check if the device information already exists */
    spin_lock(&dev_resume_info_list_lock);
    list_for_each_entry(p_info, &dev_resume_info_list, list) {
        if ((p_info->dev_type == dev_type) && (p_info->slot_id == slot_id)) {
            dev_warn(&pdev->dev, "dev_type%u slot_id%u has already existed\n", dev_type, slot_id);
            spin_unlock(&dev_resume_info_list_lock);
            return -EFAULT;
        }
    }
    spin_unlock(&dev_resume_info_list_lock);

    p_dev_resume_info = devm_kzalloc(&pdev->dev, sizeof(dev_resume_info_t), GFP_KERNEL);
    if (!p_dev_resume_info) {
        dev_err(&pdev->dev, "alloc mem fail\n");
        return -ENOMEM;
    }
    DEBUG_INFO("p_dev_resume_info: 0x%p \n", p_dev_resume_info);
    mutex_init(&p_dev_resume_info->resume_lock);

    platform_set_drvdata(pdev, p_dev_resume_info);
    p_dev_resume_info->dev_type   = dev_type;
    p_dev_resume_info->slot_id    = slot_id;
    p_dev_resume_info->bit_mask   = bit_mask;
    p_dev_resume_info->resume_way_num = resume_way_num;
    p_dev_resume_info->resume_enable = resume_enable;

    p_dev_resume_info->reg_intf_name   = reg_intf_name;
    p_dev_resume_info->reg_intf_addr   = reg_intf_addr;
    p_dev_resume_info->unreg_intf_name = unreg_intf_name;
    p_dev_resume_info->unreg_intf_addr = unreg_intf_addr;

    ret = dev_resume_info_init(pdev);
    if (ret != 0) {
        dev_err(&pdev->dev, "dev resume info init fail, ret = %d\n", ret);
        return -ENODEV;
    }

    ret = reg_intf_accord_to_dev_type(pdev);
    if (ret != 0) {
        dev_err(&pdev->dev, "reg intf fail, ret = %d\n", ret);
        return -ENODEV;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &dev_attr_group);
    if (ret != 0) {
        dev_err(&pdev->dev, "add sysfs prop fail, ret = %d\n", ret);
        return -ENODEV;
    }

    spin_lock(&dev_resume_info_list_lock);
    list_add_tail(&p_dev_resume_info->list, &dev_resume_info_list);
    spin_unlock(&dev_resume_info_list_lock);

    DEBUG_VERBOSE("end\n");
    dev_info(&pdev->dev, "The devices are probed successfully\n");
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
static void dev_resume_remove(struct platform_device *pdev)
#else
static int dev_resume_remove(struct platform_device *pdev)
#endif
{
    dev_resume_info_t *p_dev_resume_info;
    int ret;

    DEBUG_VERBOSE("start\n");

    p_dev_resume_info = platform_get_drvdata(pdev);

    spin_lock(&dev_resume_info_list_lock);
    list_del(&p_dev_resume_info->list);
    spin_unlock(&dev_resume_info_list_lock);

    sysfs_remove_group(&pdev->dev.kobj, &dev_attr_group);

    ret = unreg_intf(pdev);
    if (ret != 0) {
        dev_err(&pdev->dev, "unreg intf fail, ret = %d\n", ret);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
        return;
#else
        return -ENODEV;
#endif
    }

    dev_resume_info_deinit(pdev);

    DEBUG_VERBOSE("end\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static struct of_device_id dev_resume_of_match[] = {
    { .compatible = "dev_resume_drv", },
    { /* Sentinel */ }
};

static struct platform_driver dev_resume_pdrv = {
    .driver = {
        .name  = DRV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = dev_resume_of_match,
    },
    .probe  = dev_resume_probe,
    .remove = dev_resume_remove,
};

static int __init wb_dev_resume_init(void)
{
    int ret;

    ret = find_kallsyms_lookup_name();
    if (ret < 0) {
        DEBUG_ERROR("find kallsyms_lookup_name failed\n");
        return -ENXIO;
    }
    DEBUG_INFO("find kallsyms_lookup_name ok\n");
    return platform_driver_register(&dev_resume_pdrv);
}

static void __exit wb_dev_resume_exit(void)
{
    platform_driver_unregister(&dev_resume_pdrv);
}

module_init(wb_dev_resume_init);
module_exit(wb_dev_resume_exit);

MODULE_AUTHOR("xxx");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("register a dev resumed interface to let dev escape from unexpected exceptions");
