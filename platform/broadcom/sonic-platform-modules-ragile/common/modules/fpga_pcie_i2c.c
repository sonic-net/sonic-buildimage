#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/msi.h>
#include <linux/mfd/core.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include "fpga_i2c_ocores.h"
#include "fpga_pcie_i2c.h"
#include "fpga_reg_defs.h"

#include <linux/i2c-mux.h>
#include <linux/i2c-mux.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 15, 0)
#include <linux/platform_data/i2c-mux-gpio.h>
#else
#include <linux/i2c-mux-gpio.h>
#endif
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c-smbus.h>


#ifdef FPGA_PCIE_I2C_DEBUG
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/timex.h>
#include <linux/rtc.h>

char *enum_log="/home/pciuio-log";

void filewrite(char* filename, char* data)
{
    struct file *filp;
    mm_segment_t fs;
    filp = filp_open(filename, O_RDWR|O_APPEND|O_CREAT, 0644);
    if(IS_ERR(filp))
    {
        printk("<0>""open file error...\n");
        return;
    }

    fs=get_fs();
    set_fs(KERNEL_DS);
    filp->f_op->write(filp, data, strlen(data),&filp->f_pos);
    set_fs(fs);
    filp_close(filp,NULL);
}

void enum_time_log(char *log)
{
    struct timex  txc;
    struct rtc_time tm;
    char time_str[64];
    int ret = 0;

    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec,&tm);
    memset(time_str, 0x0, 64);
    ret = sprintf(time_str, "UTC time:%d-%d-%d %d:%d:%d ",
            tm.tm_year+1900, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    filewrite(enum_log, time_str);
    filewrite(enum_log, log);
}

void enum_notime_log(char *log)
{
    filewrite(enum_log, log);
}
#else
void enum_time_log(char *log)
{
    return;
}
void enum_notime_log(char *log)
{
    return;
}
#endif


static void __iomem *g_fpga_pcie_mem_base = NULL;

int g_fpga_pcie_debug = 0;
int g_fpga_pcie_error = 0;
int g_fpga_pcie_reset_en = 0;
int ocore_ctl_startbus = 1;
int ocore_ctl_numbers = 14;
module_param(g_fpga_pcie_reset_en, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_pcie_debug, int, S_IRUGO | S_IWUSR);
module_param(g_fpga_pcie_error, int, S_IRUGO | S_IWUSR);
module_param(ocore_ctl_startbus, int, S_IRUGO | S_IWUSR);
module_param(ocore_ctl_numbers, int, S_IRUGO | S_IWUSR);


#define FPGA_PCIE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_fpga_pcie_debug) { \
        printk(KERN_ERR "[FPGA_PCIE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_PCIE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_fpga_pcie_error) { \
        printk(KERN_ERR "[FPGA_PCIE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_MSI_IRQ_NUM            (ocore_ctl_numbers)
#define FPGA_MSI_IRQ_BEGIN          (0)
#define FPGA_MSI_IRQ_END            ((FPGA_MSI_IRQ_BEGIN) + (FPGA_MSI_IRQ_NUM))
#define FPGA_I2C_OCORE_START_BASE               (0x800)
#define FPGA_I2C_OCORE_END_BASE                 (0x81f)
#define FPGA_I2C_OCORE_CTRL_SIZE                (0x20)
#define FPGA_I2C_OCORE_CTRL_START(id)           ((FPGA_I2C_OCORE_START_BASE) + (id) * (FPGA_I2C_OCORE_CTRL_SIZE))
#define FPGA_I2C_OCORE_CTRL_END(id)             ((FPGA_I2C_OCORE_END_BASE) + (id) * (FPGA_I2C_OCORE_CTRL_SIZE))
#define FPGA_I2C_OCORE_CTRL_IRQ(id)             (id)


#define DEFINE_FPGA_PCIE_OCORE_DATA(_id) \
    static struct rg_ocores_i2c_platform_data rg_i2c_ocore_pdata_##_id = {        \
        .reg_shift = 0,          										    \
        .reg_io_width = 4,       	                                       \
        .clock_khz = 125000,        	                                   \
        .num_devices = 0,                                          \
    };

DEFINE_FPGA_PCIE_OCORE_DATA(0);
DEFINE_FPGA_PCIE_OCORE_DATA(1);
DEFINE_FPGA_PCIE_OCORE_DATA(2);
DEFINE_FPGA_PCIE_OCORE_DATA(3);
DEFINE_FPGA_PCIE_OCORE_DATA(4);
DEFINE_FPGA_PCIE_OCORE_DATA(5);
DEFINE_FPGA_PCIE_OCORE_DATA(6);
DEFINE_FPGA_PCIE_OCORE_DATA(7);
DEFINE_FPGA_PCIE_OCORE_DATA(8);
DEFINE_FPGA_PCIE_OCORE_DATA(9);
DEFINE_FPGA_PCIE_OCORE_DATA(10);
DEFINE_FPGA_PCIE_OCORE_DATA(11);
DEFINE_FPGA_PCIE_OCORE_DATA(12);
DEFINE_FPGA_PCIE_OCORE_DATA(13);
DEFINE_FPGA_PCIE_OCORE_DATA(14);
DEFINE_FPGA_PCIE_OCORE_DATA(15);
DEFINE_FPGA_PCIE_OCORE_DATA(16);
DEFINE_FPGA_PCIE_OCORE_DATA(17);
DEFINE_FPGA_PCIE_OCORE_DATA(18);
DEFINE_FPGA_PCIE_OCORE_DATA(19);
DEFINE_FPGA_PCIE_OCORE_DATA(20);
DEFINE_FPGA_PCIE_OCORE_DATA(21);
DEFINE_FPGA_PCIE_OCORE_DATA(22);
DEFINE_FPGA_PCIE_OCORE_DATA(23);
DEFINE_FPGA_PCIE_OCORE_DATA(24);
DEFINE_FPGA_PCIE_OCORE_DATA(25);
DEFINE_FPGA_PCIE_OCORE_DATA(26);
DEFINE_FPGA_PCIE_OCORE_DATA(27);

#define DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(_id) \
    static const struct resource fpga_pcie_i2c_ocores_resources_##_id[] = {      \
        {                                                                           \
            .start	= FPGA_I2C_OCORE_CTRL_START(_id),                                 \
            .end	= FPGA_I2C_OCORE_CTRL_END(_id),                                   \
            .flags	= IORESOURCE_MEM,                                               \
        },                                                                          \
        {                                                                           \
            .start 	= FPGA_I2C_OCORE_CTRL_IRQ(_id),                                  \
            .end	= FPGA_I2C_OCORE_CTRL_IRQ(_id),                                  \
            .flags	= IORESOURCE_IRQ,                                               \
        },                                                                          \
    }

DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(0);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(1);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(2);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(3);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(4);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(5);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(6);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(7);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(8);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(9);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(10);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(11);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(12);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(13);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(14);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(15);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(16);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(17);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(18);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(19);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(20);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(21);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(22);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(23);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(24);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(25);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(26);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(27);

#define DEFINE_FPGA_PCIE_MFD_CELL_CFG(_id)                                   \
{                                                                       \
    .name = "rg-i2c-ocores",                                            \
    .id            = (_id),                                                \
    .num_resources = ARRAY_SIZE(fpga_pcie_i2c_ocores_resources_##_id),   \
    .resources = fpga_pcie_i2c_ocores_resources_##_id,                   \
    .platform_data = &rg_i2c_ocore_pdata_##_id,                               \
    .pdata_size = sizeof(rg_i2c_ocore_pdata_##_id),                           \
}


static const struct mfd_cell fpga_pcie_cells_bar0_cfg0[] = {
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(0),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(1),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(2),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(3),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(4),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(5),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(6),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(7),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(8),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(9),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(10),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(11),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(12),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(13),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(14),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(15),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(16),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(17),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(18),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(19),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(20),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(21),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(22),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(23),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(24),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(25),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(26),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(27),
};

struct rgde_dev {
    struct uio_info info;
    struct pci_dev *pdev;
    struct list_head list;
    enum xdk_intr_mode mode;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,36)
/* XXX taken from uio.c, just for dumping */
struct uio_device {
    struct module		*owner;
    struct device		*dev;
    int			minor;
    atomic_t		event;
    struct fasync_struct	*async_queue;
    wait_queue_head_t	wait;
    struct uio_info		*info;
    struct kobject		*map_dir;
    struct kobject		*portio_dir;
};
#else
/*  do noting add tjm */
#endif


static char *intr_mode;
static enum xdk_intr_mode intr_mode_preferred = XDK_INTR_MODE_MSIX;


static struct list_head rgde_dev_que;

static int rgde_dev_list_dump(void)
{
    char str[256];
    struct rgde_dev *node, *tmp;
    struct uio_device *udev;

    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        udev = node->info.uio_dev;
        memset(str, 0x0, 256);
        sprintf(str, "pciuio device minor:%d\n", udev->minor);
        enum_notime_log(str);
    }
    return 0;
}

void rgde_dev_que_add(struct rgde_dev *uiodev)
{
    struct rgde_dev *node, *tmp;

    if (uiodev == NULL) {
        return;
    }

    if (list_empty(&rgde_dev_que)) {
        list_add(&uiodev->list, &rgde_dev_que);
        return;
    }

    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (((node->info).uio_dev)->minor > ((uiodev->info).uio_dev)->minor) {
            break;
        }
    }
    list_add_tail(&uiodev->list, &node->list);

    return;
}


void rgde_dev_que_del(struct rgde_dev *uiodev)
{
    struct rgde_dev *node, *tmp;

    if (uiodev == NULL) {
        return;
    }

    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (((node->info).uio_dev)->minor == ((uiodev->info).uio_dev)->minor) {
            list_del(&node->list);
            break;
        }
    }

    return;
}


struct pci_dev *rgde_to_pci_device(int minor)
{

    struct rgde_dev *node, *tmp;

    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (node->info.uio_dev->minor == minor) {
            return node->pdev;
        }

        if (node->info.uio_dev->minor < minor) {
            return NULL;
        }
    }

    return NULL;
}
EXPORT_SYMBOL(rgde_to_pci_device);

int pkt_get_mod(int logic_dev, int *mod)
{
    *mod = 0;
    return 0;
}
EXPORT_SYMBOL(pkt_get_mod);

int pkt_get_port(int logic_dev, int *port)
{
    *port = 1;
    return 0;
}
EXPORT_SYMBOL(pkt_get_port);

static int rgde_intr_mode_config(char *intr_str)
{
#if 0
    /* default intr mode : msix */
    if (!intr_str) {
        return 0;
    }

    if (!strcmp(intr_str, INTR_MODE_MSIX_NAME)) {
        intr_mode_preferred = XDK_INTR_MODE_MSIX;
        return 0;
    }

    if (!strcmp(intr_str, INTR_MODE_LEGACY_NAME)) {
        intr_mode_preferred = XDK_INTR_MODE_LEGACY;
        return 0;
    }

    /* For now, msix & legacy mode supported only. */
    printk("<0>""Error: bad parameter - %s\n", intr_str);
    return -EINVAL;
#else
    intr_mode_preferred = XDK_INTR_MODE_LEGACY;
    return 0;
#endif
}

/* Remap pci resources described by bar #pci_bar in uio resource n. */
static int rgde_setup_iomem(struct pci_dev *dev, struct uio_info *info,
        int n, int pci_bar, const char *name)
{
    unsigned long addr, len;
    void *internal_addr;

    if (n >= ARRAY_SIZE(info->mem)) {
        return -EINVAL;
    }

    addr = pci_resource_start(dev, pci_bar);
    FPGA_PCIE_DEBUG_VERBOSE("iomem phys addr:%lx\n", addr);
    len = pci_resource_len(dev, pci_bar);
    if (addr == 0 || len == 0) {
        return -1;
    }


    internal_addr = ioremap(addr, len);
    FPGA_PCIE_DEBUG_VERBOSE("iomem phys addr:0x%lx, len 0x%lx, internal_addr %p.\n", addr, len, internal_addr);

    if (internal_addr == NULL) {
        return -1;
    }

    FPGA_PCIE_DEBUG_VERBOSE("iomem internal_addr:%p\n", internal_addr);
    if (pci_bar == 0) {

        g_fpga_pcie_mem_base = internal_addr;
        FPGA_PCIE_DEBUG_VERBOSE("pci_bar %d, set g_fpga_pcie_mem_base %p\n", pci_bar, g_fpga_pcie_mem_base);
    }
    info->mem[n].name = name;
    info->mem[n].addr = addr;
    info->mem[n].internal_addr = internal_addr;
    info->mem[n].size = len;
    info->mem[n].memtype = UIO_MEM_PHYS;

    return 0;
}

/* Unmap previously ioremap'd resources */
static void rgde_release_iomem(struct uio_info *info)
{
    int i;

    for (i = 0; i < MAX_UIO_MAPS; i++) {
        if (info->mem[i].internal_addr) {
            iounmap(info->mem[i].internal_addr);
        }
    }
}

/* Get pci port io resources described by bar #pci_bar in uio resource n. */
static int rgde_setup_ioport(struct pci_dev *dev, struct uio_info *info,
        int n, int pci_bar, const char *name)
{
    unsigned long addr, len;

    if (n >= ARRAY_SIZE(info->port)) {
        return -EINVAL;
    }

    addr = pci_resource_start(dev, pci_bar);
    len = pci_resource_len(dev, pci_bar);
    if (addr == 0 || len == 0) {
        return -EINVAL;
    }

    info->port[n].name = name;
    info->port[n].start = addr;
    info->port[n].size = len;
    /* skl : FIX me */
    info->port[n].porttype = UIO_PORT_X86;

    return 0;
}

static int rgde_setup_bars(struct pci_dev *dev, struct uio_info *info)
{
    int i, iom, iop, ret;
    unsigned long flags;
    static const char *bar_names[PCI_STD_RESOURCE_END + 1]  = {
        "BAR0", "BAR1", "BAR2", "BAR3", "BAR4", "BAR5",
    };
    iom = 0;
    iop = 0;

    for (i = 0; i < ARRAY_SIZE(bar_names); i++) {
        if (pci_resource_len(dev, i) != 0 && pci_resource_start(dev, i) != 0) {

            flags = pci_resource_flags(dev, i);
            FPGA_PCIE_DEBUG_VERBOSE("flags:%lx\n", flags);
            if (flags & IORESOURCE_MEM) {
                ret = rgde_setup_iomem(dev, info, iom, i, bar_names[i]);
                if (ret != 0) {
                    return ret;
                }
                iom++;
            } else if (flags & IORESOURCE_IO) {
                ret = rgde_setup_ioport(dev, info, iop, i, bar_names[i]);
                if (ret != 0) {
                    return ret;
                }
                iop++;
            }
        }
    }

    return (iom != 0 || iop != 0) ? ret : -ENOENT;
}

/**
 * This is interrupt handler which will check if the interrupt is for the right device.
 * If yes, disable it here and will be enable later.
 */
static irqreturn_t rgde_irqhandler(int irq, struct uio_info *info)
{
    struct rgde_dev *udev = info->priv;

    if (udev->mode == XDK_INTR_MODE_LEGACY /*&& !pci_check_and_mask_intx(udev->pdev)*/) {
        return IRQ_NONE;
    }

    return IRQ_HANDLED;
}

/*
 * It masks the msix on/off of generating MSI-X messages.
 */
static void rgde_msix_mask_irq(struct msi_desc *desc, int32_t state)
{
    u32 mask_bits = desc->masked;
    unsigned offset = desc->msi_attrib.entry_nr * PCI_MSIX_ENTRY_SIZE +
        PCI_MSIX_ENTRY_VECTOR_CTRL;

    if (state != 0) {
        mask_bits &= ~PCI_MSIX_ENTRY_CTRL_MASKBIT;
    } else {
        mask_bits |= PCI_MSIX_ENTRY_CTRL_MASKBIT;
    }

    if (mask_bits != desc->masked) {
        writel(mask_bits, desc->mask_base + offset);
        readl(desc->mask_base);
        desc->masked = mask_bits;
    }
}

/**
 * This is the irqcontrol callback to be registered to uio_info.
 * It can be used to disable/enable interrupt from user space processes.
 *
 * @param info
 *  pointer to uio_info.
 * @param irq_state
 *  state value. 1 to enable interrupt, 0 to disable interrupt.
 *
 * @return
 *  - On success, 0.
 *  - On failure, a negative value.
 */
static int rgde_irqcontrol(struct uio_info *info, s32 irq_state)
{
    struct rgde_dev *udev = info->priv;
    struct pci_dev *pdev = udev->pdev;

    /* pci_cfg_access_lock(pdev); */

    if (udev->mode == XDK_INTR_MODE_LEGACY) {
        pci_intx(pdev, !!irq_state);
    } else if (udev->mode == XDK_INTR_MODE_MSIX) {
        struct msi_desc *desc;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 3, 0))
        list_for_each_entry(desc, &pdev->msi_list, list) {
            rgde_msix_mask_irq(desc, irq_state);
        }
#else
        list_for_each_entry(desc, &pdev->dev.msi_list, list) {
            rgde_msix_mask_irq(desc, irq_state);
        }
#endif
    }

    //pci_cfg_access_unlock(pdev);

    return 0;
}

int rgde_reg32_read(int minor, uint64_t offset, uint32_t *data)
{
    struct rgde_dev *node, *tmp;
    struct rgde_dev *uiodev;

    FPGA_PCIE_DEBUG_VERBOSE("enter rgde_reg32_read\n");
    uiodev = NULL;
    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (((node->info).uio_dev)->minor == minor) {
            uiodev = node;
            break;
        }
    }

    if (uiodev == NULL) {
        return -1;
    }

    if (uiodev->info.mem[0].internal_addr == NULL) {
        return -1;
    }

#if 0
    FPGA_PCIE_DEBUG_VERBOSE("internal_addr:%x\n", uiodev->info.mem[0].internal_addr);

    memcpy(ioval, (uint8_t *)uiodev->info.mem[0].internal_addr + offset, sizeof(ioval));
    for (i = 0; i < sizeof(ioval); i++) {
        FPGA_PCIE_DEBUG_VERBOSE("mem[%x]:%02x\n", (uint32_t)(offset + i), ioval[i]);
    }
#endif

    *data = (*((uint32_t *)((uint8_t *)(uiodev->info.mem[0].internal_addr) + offset)));
    return 0;
}
EXPORT_SYMBOL(rgde_reg32_read);

int rgde_reg32_write(int minor, uint64_t offset, uint32_t data)
{
    struct rgde_dev *node, *tmp;
    struct rgde_dev *uiodev;

    uiodev = NULL;
    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (((node->info).uio_dev)->minor == minor) {
            uiodev = node;
            break;
        }
    }

    if (uiodev == NULL) {
        return -1;
    }

    if (uiodev->info.mem[0].internal_addr == NULL) {
        return -1;
    }

    FPGA_PCIE_DEBUG_VERBOSE("enter rgde_reg32_write\n");
    FPGA_PCIE_DEBUG_VERBOSE("internal_addr:%p,offset:%llx,data:%x\n", uiodev->info.mem[0].internal_addr, offset, data);

    *((uint32_t *)((uint8_t *)(uiodev->info.mem[0].internal_addr) + offset)) = (data);
    FPGA_PCIE_DEBUG_VERBOSE("rgde_reg32_write ok!\n");
    return 0;
}
EXPORT_SYMBOL(rgde_reg32_write);

#if 0
static void rgde_dump_global_regs(int minor)
{
    struct rgde_dev *node, *tmp;
    struct rgde_dev *uiodev;
    uint8_t ioval[4];
    int i, j;


    uiodev = NULL;
    list_for_each_entry_safe(node, tmp, &rgde_dev_que, list) {
        if (((node->info).uio_dev)->minor == minor) {
            uiodev = node;
            break;
        }
    }

    if (uiodev == NULL) {
        return ;
    }

    if (uiodev->info.mem[0].internal_addr == NULL) {
        return ;
    }

    FPGA_PCIE_DEBUG_VERBOSE("internal_addr:%p\n", uiodev->info.mem[0].internal_addr);
    for (j = 0; j < sizeof(uint32_t) * 6; j += sizeof(uint32_t)) {
        memcpy(ioval, (uint8_t *)uiodev->info.mem[0].internal_addr + j, sizeof(ioval));
        for (i = 0; i < sizeof(ioval); i++) {
            FPGA_PCIE_DEBUG_VERBOSE("mem[%d]:%02x\n", (uint32_t)(j + i), ioval[i]);
        }
    }

    return;
}
#endif

#if 1

#define FPGA_PCIE_TEST_REG              (0x08)
#define FPGA_PCIE_TEST_VAL              (0x5A)

#define FPGA_PCIE_RESET_PCA9548_BASE    (0x20)
#define FPGA_PCIE_RESET_PCA9548_NUM     (0x4)
#define FPGA_PCIE_RESET_OCORE_BASE      (0x100)
#define FPGA_PCIE_RESET_OCORE_NUM       (ocore_ctl_numbers)

#define FPGA_PCIE_RESET_CPLD_I2C_BASE   (0x40)
#define FPGA_PCIE_RESET_CPLD_I2C_NUM    (0x4)


#define FPGA_PCIE_REG_STEP              (0x4)

#define DFD_CPLD_I2C_RETRY_TIMES    3
#define DFD_CPLD_I2C_RETRY_DELAY    100  /* ms */

#define PCA9548_MAX_CPLD_NUM   (32)

typedef struct fpga_pcie_pca9548_cfg_info_s {
    int pca9548_bus;
    int pca9548_addr;
    int cfg_offset;
} fpga_pcie_pca9548_cfg_info_t;

typedef struct fpga_pcie_card_info_s {
    int dev_type;
    fpga_pcie_pca9548_cfg_info_t pca9548_cfg_info[PCA9548_MAX_CPLD_NUM];
} fpga_pcie_card_info_t;

static fpga_pcie_card_info_t g_fpga_pcie_card_info[] = {
    {
        /* RA-B6510-32C */
        .dev_type      = 0x404b,
        .pca9548_cfg_info  = {
            {
                .pca9548_bus      = 12,
                .pca9548_addr     = 0x70,
                .cfg_offset       = 0x20,
            },
            {
                .pca9548_bus      = 12,
                .pca9548_addr     = 0x71,
                .cfg_offset       = 0x20,
            },
            {
                .pca9548_bus      = 12,
                .pca9548_addr     = 0x72,
                .cfg_offset       = 0x20,
            },
            {
                .pca9548_bus      = 12,
                .pca9548_addr     = 0x73,
                .cfg_offset       = 0x20,
            },
        },
    },
};

extern void pca954x_hw_do_reset_func_register(void* func);
extern int dfd_get_my_card_type(void);

static void fpga_pcie_setreg_32(int offset, u32 data)
{
    if (g_fpga_pcie_mem_base) {
        *((uint32_t *)((uint8_t *)(g_fpga_pcie_mem_base) + offset)) = (data);
    } else {
        FPGA_PCIE_DEBUG_ERROR("g_fpga_pcie_mem_base is null.\n");
    }
    return;
}


static inline u32 fpga_pcie_getreg_32(int offset)
{
    u32 data = 0;

    if (g_fpga_pcie_mem_base) {
        data = (*((uint32_t *)((uint8_t *)(g_fpga_pcie_mem_base) + offset)));
    } else {
        FPGA_PCIE_DEBUG_ERROR("g_fpga_pcie_mem_base is null.\n");
    }
    return data;
}

static void fpga_do_cpld_i2c_ctrl(int en)
{
#if 0
    int i;
    int offset;

    for (i = 0; i < FPGA_PCIE_RESET_CPLD_I2C_NUM; i++) {
        offset = FPGA_PCIE_RESET_CPLD_I2C_BASE + i * FPGA_PCIE_REG_STEP;
        FPGA_PCIE_DEBUG_VERBOSE("offset 0x%x, write en 0x%x.\n", offset, en);
        fpga_pcie_setreg_32(offset, en);
    }
#endif
    return;
}


static void fpga_do_ocore_ctrl(int en)
{
    int i;
    int offset;

    for (i = 0; i < FPGA_PCIE_RESET_OCORE_NUM; i++) {
        offset = FPGA_PCIE_RESET_OCORE_BASE + i * FPGA_PCIE_REG_STEP;
        FPGA_PCIE_DEBUG_VERBOSE("offset 0x%x, write en 0x%x.\n", offset, en);
        fpga_pcie_setreg_32(offset, en);
    }
}

static void fpga_do_9548_ctrl(int en)
{
    int i;
    int offset;

    for (i = 0; i < FPGA_PCIE_RESET_PCA9548_NUM; i++) {
        offset = FPGA_PCIE_RESET_PCA9548_BASE + i * FPGA_PCIE_REG_STEP;
        FPGA_PCIE_DEBUG_VERBOSE("offset 0x%x, write en 0x%x.\n", offset, en);
        fpga_pcie_setreg_32(offset, en);
    }

}

static void fpga_reset_ocore_i2c(void)
{
    u32 data;


    if (g_fpga_pcie_reset_en == 0) {
        FPGA_PCIE_DEBUG_VERBOSE("g_fpga_pcie_reset_en is 0, do nothing.\n");
        return;
    }

    data = fpga_pcie_getreg_32(FPGA_PCIE_TEST_REG);
    FPGA_PCIE_DEBUG_VERBOSE("BEGIN FPGA_PCIE_TEST_REG=[0x%x], write 0x%x.\n", data, FPGA_PCIE_TEST_VAL);
    fpga_pcie_setreg_32(FPGA_PCIE_TEST_REG, FPGA_PCIE_TEST_VAL);
    data = fpga_pcie_getreg_32(FPGA_PCIE_TEST_REG);
    FPGA_PCIE_DEBUG_VERBOSE("END FPGA_PCIE_TEST_REG=[0x%x].\n", data);



    fpga_do_9548_ctrl(0);
    fpga_do_ocore_ctrl(0);
    fpga_do_cpld_i2c_ctrl(0);

    mdelay(500);


    fpga_do_9548_ctrl(1);
    fpga_do_ocore_ctrl(1);
    fpga_do_cpld_i2c_ctrl(1);

    return;
}

static void fpga_do_pca9548_reset_ctrl(int offset, int en)
{
    FPGA_PCIE_DEBUG_VERBOSE("offset 0x%x, write en 0x%x.\n", offset, en);
    fpga_pcie_setreg_32(offset, en);
}

fpga_pcie_card_info_t* fpga_pcie_get_card_info(int dev_type)
{
    int i;
    int size;

    size = ARRAY_SIZE(g_fpga_pcie_card_info);

    FPGA_PCIE_DEBUG_VERBOSE("Enter dev_type 0x%x size %d.\n", dev_type, size);
    for (i = 0; i < size; i++) {
        if (g_fpga_pcie_card_info[i].dev_type == dev_type) {
            FPGA_PCIE_DEBUG_VERBOSE("match dev_type 0x%x.\n", dev_type);
            return &g_fpga_pcie_card_info[i];
        }
    }

    FPGA_PCIE_DEBUG_VERBOSE("dismatch dev_type 0x%x.\n", dev_type);
    return NULL;
}

fpga_pcie_pca9548_cfg_info_t* fpga_pcie_get_pca9548_cfg_info(int bus, int addr)
{
    int dev_type;
    fpga_pcie_card_info_t *info;
    fpga_pcie_pca9548_cfg_info_t *pca9548_cfg_info;
    int i;
    int size;

    dev_type = dfd_get_my_card_type();
    if (dev_type < 0) {
        FPGA_PCIE_DEBUG_ERROR("drv_get_my_dev_type failed ret %d.\n", dev_type);
        return NULL;
    }

    info = fpga_pcie_get_card_info(dev_type);
    if (info == NULL) {
        FPGA_PCIE_DEBUG_ERROR("fpga_pcie_get_card_info dev_type %d failed.\n", dev_type);
        return NULL;
    }

    size = PCA9548_MAX_CPLD_NUM;
    for (i = 0; i < size; i++) {
        pca9548_cfg_info = &(info->pca9548_cfg_info[i]);
        if ((pca9548_cfg_info->pca9548_bus == bus) && (pca9548_cfg_info->pca9548_addr == addr)) {
            FPGA_PCIE_DEBUG_VERBOSE("match dev_type 0x%x bus %d addr 0x%x.\n", dev_type, bus, addr);
            return pca9548_cfg_info;
        }
    }

    FPGA_PCIE_DEBUG_VERBOSE("dismatch dev_type 0x%x bus %d addr 0x%x.\n", dev_type, bus, addr);
    return NULL;
}


void fpga_do_pca954x_reset_func(int bus, int addr)
{
    fpga_pcie_pca9548_cfg_info_t *cfg_info;

    cfg_info = fpga_pcie_get_pca9548_cfg_info(bus, addr);
    if (cfg_info == NULL) {
        FPGA_PCIE_DEBUG_VERBOSE("fpga_do_pca954x_reset_func do nothing.\n");
        return;
    }

    FPGA_PCIE_DEBUG_VERBOSE("bus %d addr 0x%x, cfg_info.offset:0x%x.\n", bus, addr, cfg_info->cfg_offset);

    fpga_do_pca9548_reset_ctrl(cfg_info->cfg_offset, 0);
    mdelay(250);
    fpga_do_pca9548_reset_ctrl(cfg_info->cfg_offset, 1);
}

static void fpga_do_pca954x_reset_func_reg(void)
{
    pca954x_hw_do_reset_func_register(fpga_do_pca954x_reset_func);
}

#endif


static int fpga_i2c_ocore_device_init(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int ret, index;
    struct rg_ocores_i2c_platform_data *init_nr_ocores;

    for (index = 0 ; index < ARRAY_SIZE(fpga_pcie_cells_bar0_cfg0); index++) {
        init_nr_ocores = fpga_pcie_cells_bar0_cfg0[index].platform_data;
        init_nr_ocores->nr = ocore_ctl_startbus + index;
    }
    FPGA_PCIE_DEBUG_VERBOSE("Enter.\n");
    FPGA_PCIE_DEBUG_VERBOSE("Begin mfd_add_devices.\n");
    ret = mfd_add_devices(&pdev->dev, 0,
            fpga_pcie_cells_bar0_cfg0,
            ocore_ctl_numbers > ARRAY_SIZE(fpga_pcie_cells_bar0_cfg0) ? ARRAY_SIZE(fpga_pcie_cells_bar0_cfg0) : ocore_ctl_numbers  ,
            &pdev->resource[0], pdev->irq, NULL);
    FPGA_PCIE_DEBUG_VERBOSE("End mfd_add_devices ret %d.\n", ret);
    if (ret) {
        dev_err(&pdev->dev, "mfd_add_devices failed: %d\n", ret);
        return -1;
    }

    fpga_do_pca954x_reset_func_reg();
    FPGA_PCIE_DEBUG_VERBOSE("Call fpga_do_pca954x_reset_func_reg.\n");
    return 0;
}

static void fpga_pcie_recover(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct resource *mem_base;
    u32 bar0_val;
    int ret;

    mem_base = &pdev->resource[0];
    ret = pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0, &bar0_val);
    if (ret) {
        FPGA_PCIE_DEBUG_ERROR("pci_read_config_dword failed ret %d.\n", ret);
        return;
    }
    FPGA_PCIE_DEBUG_VERBOSE("mem_base->start[0x%llx], bar0_val[0x%x], ret %d.\n",
            mem_base->start, bar0_val, ret);

    if (bar0_val != mem_base->start) {
        ret = pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0, mem_base->start);
        if (ret) {
            FPGA_PCIE_DEBUG_ERROR("pci_write_config_dword mem_base->start[0x%llx], failed ret %d.\n", mem_base->start, ret);
            return;
        }
        FPGA_PCIE_DEBUG_VERBOSE("pci_write_config_dword mem_base->start[0x%llx] success.\n", mem_base->start);
    } else {
        FPGA_PCIE_DEBUG_VERBOSE("mem_base->start[0x%llx], bar0_val[0x%x], do nothing.\n",
                mem_base->start, bar0_val);
    }
}

static int fpga_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    struct rgde_dev *rdev = NULL;

    FPGA_PCIE_DEBUG_VERBOSE("Enter vendor 0x%x, subsystem_vendor 0x%x.\n", pdev->vendor, pdev->subsystem_vendor);

    /* skl : FIX me */
    /*
       if ((pdev->vendor != ) || (pdev->subsystem_vendor != )) {
       err = -ENODEV;
       goto dev_suppport_err:;
       }*/


    fpga_pcie_recover(pdev, id);

    /* enable device: ask low-level code to enable I/O and memory */
    FPGA_PCIE_DEBUG_VERBOSE("start pci_enable_device!\n");
    err = pci_enable_device(pdev);
    if (err) {
        FPGA_PCIE_DEBUG_ERROR("pci_enable_device failed: %d\n", err);
        goto dev_ebable_err;
    }

    FPGA_PCIE_DEBUG_VERBOSE("start pci_set_master!\n");
    pci_set_master(pdev);

    rdev = kzalloc(sizeof(struct rgde_dev), GFP_KERNEL);
    if (!rdev) {
        err = -ENOMEM;
        goto kzalloc_err;
    }


    FPGA_PCIE_DEBUG_VERBOSE("start rgde_setup_bars!\n");
    err = rgde_setup_bars(pdev, &rdev->info);
    if (err != 0) {
        goto setup_bars_err;
    }

    rdev->info.name = "fpga_pcie";
    rdev->info.version = "0.1";
    rdev->info.handler = rgde_irqhandler;
    rdev->info.irqcontrol = rgde_irqcontrol;
    rdev->info.priv = rdev;
    rdev->pdev = pdev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
    err = pci_alloc_irq_vectors(pdev,FPGA_MSI_IRQ_BEGIN + 1, ocore_ctl_numbers, PCI_IRQ_MSI);
#else
    err = pci_enable_msi_range(pdev, FPGA_MSI_IRQ_BEGIN + 1, ocore_ctl_numbers);
#endif
    if (err != ocore_ctl_numbers) {
        FPGA_PCIE_DEBUG_ERROR("pci_enable_msi_block err %d FPGA_MSI_IRQ_NUM %d.\n", err,
                ocore_ctl_numbers);
        goto uio_register_err;
    }

    FPGA_PCIE_DEBUG_VERBOSE("before pci_set_drvdata.\n");

    pci_set_drvdata(pdev, rdev);
    FPGA_PCIE_DEBUG_VERBOSE("after pci_set_drvdata.\n");
    enum_time_log("rgde_dev_que_add\n");

    mdelay(100);

    fpga_reset_ocore_i2c();

    fpga_i2c_ocore_device_init(pdev, id);
    return 0;

uio_register_err:
    /* udev_irq_err: */
setup_bars_err:
    rgde_release_iomem(&rdev->info);
    pci_disable_msi(rdev->pdev);
    pci_release_regions(pdev);
    kfree(rdev);
kzalloc_err:
    /* request_region_err: */
    pci_disable_device(pdev);
dev_ebable_err:
    /* dev_suppport_err: */
    return err;
}

static void fpga_pcie_remove(struct pci_dev *pdev)
{
    struct rgde_dev *rdev = pci_get_drvdata(pdev);

    FPGA_PCIE_DEBUG_VERBOSE("fpga_pcie_remove.\n");
#if 0
    enum_time_log("rgde_dev_que_del\n");
    printk("<0>""uio device %d del.\n", rdev->info.uio_dev->minor);
#endif
    rgde_dev_que_del(rdev);
    rgde_dev_list_dump();
#if 0
    uio_unregister_device(&rdev->info);
#endif
    mfd_remove_devices(&pdev->dev);
    rgde_release_iomem(&rdev->info);
    pci_disable_msi(rdev->pdev);
    //pci_release_regions(pdev);
    pci_disable_device(pdev);
    kfree(rdev);
}

/* static DEFINE_PCI_DEVICE_TABLE(fpga_pci_ids) = { */

static const struct pci_device_id fpga_pci_ids[] = {
    { PCI_DEVICE(0x10ee, 0x7022)},
    {0}
};
MODULE_DEVICE_TABLE(pci, fpga_pci_ids);


static struct pci_driver fpga_pcie_driver = {
    .name = "fpga_pcie",
    .id_table = fpga_pci_ids,/* only dynamic id's */
    .probe = fpga_pcie_probe,
    .remove = fpga_pcie_remove,
};

static int __init fpga_pcie_init(void)
{
    int ret;

    FPGA_PCIE_DEBUG_VERBOSE("fpga_pcie_init enter!\n");
    ret = rgde_intr_mode_config(intr_mode);
    if (ret < 0) {
        return ret;
    }

    INIT_LIST_HEAD(&rgde_dev_que);

    return pci_register_driver(&fpga_pcie_driver);
}

static void __exit fpga_pcie_exit(void)
{
    FPGA_PCIE_DEBUG_VERBOSE("fpga_pcie_exit enter!\n");
    pci_unregister_driver(&fpga_pcie_driver);
}

module_init(fpga_pcie_init);
module_exit(fpga_pcie_exit);
module_param(intr_mode, charp, S_IRUGO);
MODULE_PARM_DESC(intr_mode,
        "pci_uio interrupt mode (default=msix):\n"
        "    " INTR_MODE_MSIX_NAME "       Use MSIX interrupt\n"
        "    " INTR_MODE_LEGACY_NAME "     Use Legacy interrupt\n"
        "\n");
MODULE_DESCRIPTION("UIO Driver for PCI Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support <support@ragile.com>");
