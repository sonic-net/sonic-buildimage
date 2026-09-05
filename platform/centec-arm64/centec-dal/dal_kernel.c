/**
 @file dal_kernal.c

 @date 2012-10-18

 @version v2.0
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/types.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#if defined(SOC_ACTIVE) || defined(ASW_ACTIVE)
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
#include <linux/irqdomain.h>
#endif
#include "dal_common.h"
#include "dal_kernel.h"
#include "dal_mpool.h"
#include <linux/slab.h>
#ifdef ASW_ACTIVE
#include <linux/of_mdio.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#endif
#include "ctc_mix.h"

#if defined(ZM_MODE)
MODULE_AUTHOR("ZEGMA Communications Co., Ltd.");
#else
MODULE_AUTHOR("Suzhou Centec Communications Co., Ltd.");
#endif
MODULE_DESCRIPTION("DAL kernel module");
MODULE_LICENSE("GPL");

/* DMA memory pool size */
static char* dma_pool_size = NULL;
module_param(dma_pool_size, charp, 0);
MODULE_PARM_DESC(dma_pool_size,
                 "Specify DMA memory pool size (default 4MB)");

static char* dal_io_user_mode;
module_param(dal_io_user_mode, charp, 0);
MODULE_PARM_DESC(dal_io_user_mode,
                 "Specify io mode(default mdio)");

/* dal debug param*/
static int dal_debug = 0;
module_param(dal_debug, int, 0);
MODULE_PARM_DESC(dal_debug, "Set debug level (default 0)");
/*****************************************************************************
 * defines
 *****************************************************************************/
#define MEM_MAP_RESERVE             SetPageReserved
#define MEM_MAP_UNRESERVE           ClearPageReserved

#define VIRT_TO_PAGE(p)             virt_to_page((p))
#define DAL_UNTAG_BLOCK             0
#define DAL_DISCARD_BLOCK           1
#define DAL_MATCHED_BLOCK           2
#define DAL_CUR_MATCH_BLOCk         3
#define DAL_SPI_OP_DELAY_US         5

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
#define virt_to_bus virt_to_phys
#define bus_to_virt phys_to_virt
#endif

#define VERIFY_CHIP_INDEX(n) (n < CTC_MAX_LOCAL_CHIP_NUM)

typedef irqreturn_t (*p_func) (int irq, void* dev_id);

#define DAL_INTR_HANDLER_TEMPLATE(idx) static irqreturn_t \
intr##idx##_handler(int irq, void* dev_id) \
{ \
    dal_isr_t* p_dal_isr = (dal_isr_t*)dev_id; \
    if(dal_glb.dal_isr[idx].poll_intr_trigger) \
    { \
        return IRQ_HANDLED; \
    } \
    disable_irq_nosync(irq); \
    dal_glb.dal_isr[idx].poll_intr_trigger = 1; \
    wake_up(&dal_glb.dal_isr[idx].poll_intr); \
    if (p_dal_isr->isr_knet) \
    { \
        p_dal_isr->isr_knet(p_dal_isr->isr_knet_data); \
    } \
    return IRQ_HANDLED; \
}

#define DAL_INTR_POLL_TEMPLATE(idx) static unsigned int \
linux_dal_poll##idx##_func(struct file* filp, struct poll_table_struct* p) \
{ \
    unsigned int mask = 0; \
    unsigned long flags; \
    poll_wait(filp, &dal_glb.dal_isr[idx].poll_intr, p); \
    local_irq_save(flags); \
    if (dal_glb.dal_isr[idx].poll_intr_trigger) \
    { \
        dal_glb.dal_isr[idx].poll_intr_trigger = 0; \
        mask |= POLLIN | POLLRDNORM; \
    } \
    local_irq_restore(flags); \
    return mask; \
}

#define DAL_INTR_RELEASE_TEMPLATE(idx) static int \
linux_dal_release##idx(struct inode* inode, struct file* filp)\
{\
    unsigned char str[16] = {0};\
    if (0 != dal_glb.dal_isr[idx].count && !dal_glb.dal_isr[idx].isr_knet)\
    {/* irq not free in dal_interrupt_unregister, free it here */\
        snprintf(str, 16, "%s%d", "dal_intr", idx);\
        unregister_chrdev(DAL_DEV_INTR_MAJOR_BASE + idx, str);\
        free_irq(dal_glb.dal_isr[idx].irq, &dal_glb.dal_isr[idx]);\
        printk("irq %d is not freed in SDK deinit, free it in kernel release\n", dal_glb.dal_isr[idx].irq);\
        dal_glb.dal_isr[idx].poll_intr_trigger = 0;\
        dal_glb.dal_isr[idx].irq = 0;\
        dal_glb.dal_isr[idx].count = 0;\
        dal_glb.dal_intr_num--;\
    }\
    return 0;\
}

#define _DEBUG_ 0
#if _DEBUG_
#define CTC_PRINTK(...) printk(__VA_ARGS__)
#else
#define CTC_PRINTK(...)
#endif

/*****************************************************************************
 * typedef
 *****************************************************************************/
/* Control Data */
typedef struct dal_isr_s
{
    int irq;
    void (* isr)(void*);
    void (* isr_knet)(void*);
    void* isr_data;
    void* isr_knet_data;
    int trigger;
    int count;
    wait_queue_head_t poll_intr;
    p_func intr_handler_fun;
    int poll_intr_trigger;
} dal_isr_t;
#if defined (ASW_ACTIVE)
typedef struct dal_kernel_asw_dev_s
{
	struct platform_device* p_dev;
	unsigned char	mode;
	struct dal_reg_ops	*reg_ops;
	struct mii_bus		*ext_mbus;
	dev_t devno;
	int	irq;
	struct spi_device *spi_device;
	struct i2c_adapter *i2c_adapt;
	unsigned int i2c_addr;
	unsigned int mdio_addr;
	unsigned int spi_addr;
} dal_kernel_asw_dev_t;
struct dal_reg_ops {
	int (*read_reg)(void *cfg, unsigned int addr, unsigned int *data);
	int (*write_reg)(void *cfg, unsigned int addr, unsigned int data);
};
#endif

typedef struct dal_kernel_dev_s
{
    struct list_head list;
    /*pcie device pointer struct pci_dev*, local pointer struct platform_device* */
    void* pci_dev;

    /* PCI I/O mapped base address */
    void __iomem * logic_address;

    /* Physical address */
    uintptr phys_address;
} dal_kernel_dev_t;

typedef struct _dma_segment
{
    struct list_head list;
    unsigned long req_size;     /* Requested DMA segment size */
    unsigned long blk_size;     /* DMA block size */
    unsigned long blk_order;    /* DMA block size in alternate format */
    unsigned long seg_size;     /* Current DMA segment size */
    unsigned long seg_begin;    /* Logical address of segment */
    unsigned long seg_end;      /* Logical end address of segment */
    unsigned long* blk_ptr;     /* Array of logical DMA block addresses */
    int blk_cnt_max;            /* Maximum number of block to allocate */
    int blk_cnt;                /* Current number of blocks allocated */
} dma_segment_t;

struct dal_kernel_glb_s
{
    unsigned char dal_chip_num;
    unsigned char dal_version;
    unsigned char dal_intr_num;
    unsigned char use_high_memory;
    unsigned int dma_mem_size;
	unsigned char dal_io_mode;
    struct class *dal_class;
    dal_isr_t dal_isr[DAL_MAX_INTR_NUM];
};
typedef struct dal_kernel_glb_s dal_kernel_glb_t;

struct dal_kernel_master_s
{
    dal_kernel_dev_t dal_dev;
    unsigned int* dma_virt_base;
    unsigned long long dma_phy_base;
    unsigned int msi_irq_base[DAL_MAX_INTR_NUM];
    unsigned int msi_irq_num;
    unsigned int msi_used;   /*0:none, 1: msi 2:msi-x*/
    unsigned int active_type;
    unsigned int dev_no;
};
typedef struct dal_kernel_master_s dal_kernel_master_t;
/***************************************************************************
 *declared
 ***************************************************************************/
static int dal_interrupt_connect(unsigned int irq, int prio, void (* isr)(void*), void* data);
static int dal_interrupt_disconnect(unsigned int irq);
static unsigned int linux_dal_poll0_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll1_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll2_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll3_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll4_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll5_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll6_func(struct file* filp, struct poll_table_struct* p);
static unsigned int linux_dal_poll7_func(struct file* filp, struct poll_table_struct* p);
static int linux_dal_release0(struct inode* inode, struct file* filp);
static int linux_dal_release1(struct inode* inode, struct file* filp);
static int linux_dal_release2(struct inode* inode, struct file* filp);
static int linux_dal_release3(struct inode* inode, struct file* filp);
static int linux_dal_release4(struct inode* inode, struct file* filp);
static int linux_dal_release5(struct inode* inode, struct file* filp);
static int linux_dal_release6(struct inode* inode, struct file* filp);
static int linux_dal_release7(struct inode* inode, struct file* filp);

/*****************************************************************************
 * global variables
 *****************************************************************************/
static dal_kernel_glb_t dal_glb;
static dal_kernel_master_t* dal_master[CTC_MAX_LOCAL_CHIP_NUM] = {NULL};
#ifdef ASW_ACTIVE
static unsigned int active_type[CTC_MAX_LOCAL_CHIP_NUM] = {0};
static void* dal_dev[CTC_MAX_LOCAL_CHIP_NUM]={0};
#endif
static LIST_HEAD(_dma_seg);

static struct pci_device_id dal_id_table[] =
{
    {PCI_DEVICE(DAL_VENDOR_VID, DAL_GREATBELT_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_GOLDENGATE_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID1, DAL_GOLDENGATE_DEVICE_ID1)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_DUET2_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_TSINGMA_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_TSINGMA_DX_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_TSINGMA_MX_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_TSINGMA_GX_DEVICE_ID)},   
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_ARCTIC_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_ARCTIC_M3_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_TSINGMA_DXL_DEVICE_ID)},
    {PCI_DEVICE(DAL_PCIE_VENDOR_ID, DAL_PACIFIC_M12_DEVICE_ID)},
    {0, },
};
#if defined(SOC_ACTIVE)
static const struct of_device_id linux_dal_of_match[] = {
    { .compatible = "centec,dal-localbus", .data = "tm_soc",},
    { .compatible = "centec,tsingma-dx-dal-localbus", .data = "tm_dx_soc",},        
    { .compatible = "ctc,tsingma-dx-dal-localbus", .data = "tm_tmdl_soc",},
    {},
};
MODULE_DEVICE_TABLE(of, linux_dal_of_match);
#endif
#if defined(ASW_ACTIVE)
static const struct of_device_id linux_dal_asw_of_match[] = {
       { .compatible = "centec,ctc2118" },
       {},
};
MODULE_DEVICE_TABLE(of, linux_dal_asw_of_match);
#endif

static dal_ops_t g_dal_ops =
{
    interrupt_connect:dal_interrupt_connect,
    interrupt_disconnect:dal_interrupt_disconnect,
};

static struct file_operations dal_intr_fops[DAL_MAX_INTR_NUM] =
{
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll0_func,
        .release = linux_dal_release0,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll1_func,
        .release = linux_dal_release1,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll2_func,
        .release = linux_dal_release2,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll3_func,
        .release = linux_dal_release3,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll4_func,
        .release = linux_dal_release4,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll5_func,
        .release = linux_dal_release5,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll6_func,
        .release = linux_dal_release6,
    },
    {
        .owner = THIS_MODULE,
        .poll = linux_dal_poll7_func,
        .release = linux_dal_release7,
    },
};

/*****************************************************************************
 * macros
 *****************************************************************************/
DAL_INTR_HANDLER_TEMPLATE(0)
DAL_INTR_HANDLER_TEMPLATE(1)
DAL_INTR_HANDLER_TEMPLATE(2)
DAL_INTR_HANDLER_TEMPLATE(3)
DAL_INTR_HANDLER_TEMPLATE(4)
DAL_INTR_HANDLER_TEMPLATE(5)
DAL_INTR_HANDLER_TEMPLATE(6)
DAL_INTR_HANDLER_TEMPLATE(7)

DAL_INTR_POLL_TEMPLATE(0)
DAL_INTR_POLL_TEMPLATE(1)
DAL_INTR_POLL_TEMPLATE(2)
DAL_INTR_POLL_TEMPLATE(3)
DAL_INTR_POLL_TEMPLATE(4)
DAL_INTR_POLL_TEMPLATE(5)
DAL_INTR_POLL_TEMPLATE(6)
DAL_INTR_POLL_TEMPLATE(7)

DAL_INTR_RELEASE_TEMPLATE(0)
DAL_INTR_RELEASE_TEMPLATE(1)
DAL_INTR_RELEASE_TEMPLATE(2)
DAL_INTR_RELEASE_TEMPLATE(3)
DAL_INTR_RELEASE_TEMPLATE(4)
DAL_INTR_RELEASE_TEMPLATE(5)
DAL_INTR_RELEASE_TEMPLATE(6)
DAL_INTR_RELEASE_TEMPLATE(7)

/*****************************************************************************
 * ftunction
 *****************************************************************************/
#define _KERNEL_INTERUPT_PROCESS
int
dal_interrupt_register(unsigned int irq, int prio, void (* isr)(void*), void* data)
{
    int ret;
    unsigned char str[16];
    unsigned char* int_name = NULL;
    unsigned int intr_num_tmp = 0;
    unsigned int intr_num = DAL_MAX_INTR_NUM;
    unsigned long irq_flags = 0;

    if (dal_glb.dal_intr_num >= DAL_MAX_INTR_NUM)
    {
        printk("Interrupt numbers exceeds max.\n");
        return -1;
    }

    int_name = "dal_intr";
    for (intr_num_tmp=0;intr_num_tmp < DAL_MAX_INTR_NUM; intr_num_tmp++)
    {
        if (irq == dal_glb.dal_isr[intr_num_tmp].irq)
        {
            dal_glb.dal_isr[intr_num_tmp].count++;
            printk("Interrupt irq %d register count %d.\n", irq, dal_glb.dal_isr[intr_num_tmp].count);
            return 0;
        }
        if ((0 == dal_glb.dal_isr[intr_num_tmp].irq) && (DAL_MAX_INTR_NUM == intr_num))
        {
            intr_num = intr_num_tmp;
            dal_glb.dal_isr[intr_num].count = 0;
        }
    }
    dal_glb.dal_isr[intr_num].irq = irq;
    dal_glb.dal_isr[intr_num].isr = isr;
    dal_glb.dal_isr[intr_num].isr_data = data;
    dal_glb.dal_isr[intr_num].count++;

    init_waitqueue_head(&dal_glb.dal_isr[intr_num].poll_intr);

    /* only user mode */
    if ((NULL == isr) && (NULL == data))
    {
        snprintf(str, 16, "%s%d", "dal_intr", intr_num);
        ret = register_chrdev(DAL_DEV_INTR_MAJOR_BASE + intr_num,
                              str, &dal_intr_fops[intr_num]);
        if (ret < 0)
        {
            printk("Register character device for irq %d failed, ret= %d", irq, ret);
            return ret;
        }
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
    irq_flags = 0;
#else
    irq_flags = IRQF_DISABLED;
#endif
    if ((ret = request_irq(irq,
                           dal_glb.dal_isr[intr_num].intr_handler_fun,
                           irq_flags,
                           int_name,
                           &dal_glb.dal_isr[intr_num])) < 0)
    {
        printk("Cannot request irq %d, ret %d.\n", irq, ret);
        unregister_chrdev(DAL_DEV_INTR_MAJOR_BASE + intr_num, str);
    }

    if (0 == ret)
    {
        dal_glb.dal_intr_num++;
    }

    return ret;
}

int
dal_interrupt_unregister(unsigned int irq)
{
    unsigned char str[16];
    int intr_idx = 0;
    int find_flag = 0;

    /* get intr device index */
    for (intr_idx = 0; intr_idx < DAL_MAX_INTR_NUM; intr_idx++)
    {
        if (dal_glb.dal_isr[intr_idx].irq == irq)
        {
            find_flag = 1;
            break;
        }
    }

    if (find_flag == 0)
    {
        printk ("irq%d is not registered! unregister failed \n", irq);
        return -1;
    }

    dal_glb.dal_isr[intr_idx].count--;
    if (0 != dal_glb.dal_isr[intr_idx].count)
    {
        printk("Interrupt irq %d unregister count %d.\n", irq, dal_glb.dal_isr[intr_idx].count);
        return -1;
    }
    snprintf(str, 16, "%s%d", "dal_intr", intr_idx);

    unregister_chrdev(DAL_DEV_INTR_MAJOR_BASE + intr_idx, str);

    free_irq(irq, &dal_glb.dal_isr[intr_idx]);

    dal_glb.dal_isr[intr_idx].irq = 0;

    dal_glb.dal_intr_num--;

    return 0;
}

static int
dal_interrupt_connect(unsigned int irq, int prio, void (* isr)(void*), void* data)
{
    int irq_idx = 0;

    for (irq_idx = 0; irq_idx < DAL_MAX_INTR_NUM; irq_idx++)
    {
        if (dal_glb.dal_isr[irq_idx].irq == irq)
        {
            dal_glb.dal_isr[irq_idx].isr_knet = isr;
            dal_glb.dal_isr[irq_idx].isr_knet_data = data;

            return 0;
        }
    }

    return -1;
}

static int
dal_interrupt_disconnect(unsigned int irq)
{
    int irq_idx = 0;

    for (irq_idx = 0; irq_idx < DAL_MAX_INTR_NUM; irq_idx++)
    {
        if (dal_glb.dal_isr[irq_idx].irq == irq)
        {
            dal_glb.dal_isr[irq_idx].isr_knet = NULL;
            dal_glb.dal_isr[irq_idx].isr_knet_data = NULL;

            return 0;
        }
    }

    return -1;
}

int
dal_get_dal_ops(dal_ops_t **dal_ops)
{
    *dal_ops = &g_dal_ops;

    return 0;
}

int
dal_interrupt_set_en(unsigned int irq, unsigned int enable)
{
    enable ? enable_irq(irq) : disable_irq_nosync(irq);
    return 0;
}

static int
_dal_set_msi_enabe(unsigned int lchip, unsigned int irq_num, unsigned int msi_type)
{
    int ret = 0;

#ifdef CONFIG_PCI_MSI
    if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
    {
        unsigned int index = 0;
        struct pci_dev* pci_dev = (struct pci_dev*)dal_master[lchip]->dal_dev.pci_dev;
        if (DAL_MSI_TYPE_MSI == msi_type)
        {
            if (irq_num == 1)
            {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
                ret = pci_alloc_irq_vectors(pci_dev, 1, 1, PCI_IRQ_MSI);
                if (ret != irq_num)
                {
                    printk ("msi enable failed!!! lchip = %d, irq_num = %d\n", lchip, irq_num);
                    pci_free_irq_vectors(pci_dev);
                    return -1;
                }
                dal_master[lchip]->msi_irq_base[0] = pci_irq_vector(pci_dev, 0);
                dal_master[lchip]->msi_irq_num = 1;
#else
                ret = pci_enable_msi(pci_dev);
                if (ret < 0)
                {
                    printk ("msi enable failed!!! lchip = %d, irq_num = %d\n", lchip, irq_num);
                    pci_disable_msi(pci_dev);
                    return -1;
                }
                dal_master[lchip]->msi_irq_base[0] = pci_dev->irq;
                dal_master[lchip]->msi_irq_num = 1;
#endif
            }
            else
            {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
                ret = pci_alloc_irq_vectors(pci_dev, 1, irq_num, PCI_IRQ_MSI);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 79))
                ret = pci_enable_msi_exact(pci_dev, irq_num);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 26, 32))
                ret = pci_enable_msi_block(pci_dev, irq_num);
#else
                ret = -1;
#endif
                if (ret < 0)
                {
                    printk ("msi enable failed!!! lchip = %d, irq_num = %d\n", lchip, irq_num);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
                    pci_free_irq_vectors(pci_dev);
#else
                    pci_disable_msi(pci_dev);
#endif
                    return -1;
                }
                dal_master[lchip]->msi_irq_num = ret;
                for (index=0; index<ret; index++)
                {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
                    dal_master[lchip]->msi_irq_base[index] = pci_irq_vector(pci_dev, index);
#else
                    dal_master[lchip]->msi_irq_base[index] = pci_dev->irq+index;
#endif
                }
            }
            dal_master[lchip]->msi_used = 1;
        }
        else
        {
            struct msix_entry entries[DAL_MAX_INTR_NUM];
            unsigned int index = 0;
            memset(entries, 0, sizeof(struct msix_entry)*DAL_MAX_INTR_NUM);
            for (index = 0; index < DAL_MAX_INTR_NUM; index++)
            {
                entries[index].entry = index;
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 1))
            ret = pci_enable_msix_range(pci_dev, entries, irq_num, irq_num);
            if (ret == irq_num)
            {
                dal_master[lchip]->msi_irq_num = irq_num;
                for (index=0; index<irq_num; index++)
                {
                    dal_master[lchip]->msi_irq_base[index]= entries[index].vector;
                    printk ("msix enable success!!! irq index %u, irq val %u\n", index, dal_master[lchip]->msi_irq_base[index]);
                }
                ret = 0;
            }
            else
            {
                printk ("msix enable failed!!! lchip = %d", lchip);
                return -1;
            }
#else
            ret = pci_enable_msix(pci_dev, entries, irq_num);
            if (ret > 0)
            {
                printk ("msix retrying interrupts = %d\n", ret);
                ret = pci_enable_msix(pci_dev, entries, ret);
                if (ret != 0)
                {
                    printk ("msix enable failed!!! lchip = %d, irq_num = %d\n", lchip, irq_num);
                    return -1;
                }
            }
            else if (ret < 0)
            {
                printk ("msix enable failed!!! lchip = %d, irq_num = %d\n", lchip, irq_num);
                return -1;
            }
            else
            {
                dal_master[lchip]->msi_irq_num = irq_num;
                for (index=0; index<irq_num; index++)
                {
                    dal_master[lchip]->msi_irq_base[index] = entries[index].vector;
                    printk ("msix enable success!!! irq index %u, irq val %u\n", index, dal_master[lchip]->msi_irq_base[index]);
                }
            }
#endif
			dal_master[lchip]->msi_used = 2;
        }
    }
#endif

    return ret;
}

static int
_dal_set_msi_disable(unsigned int lchip, unsigned int msi_type)
{
#ifdef CONFIG_PCI_MSI
    if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type && dal_master[lchip]->msi_used)
    {
        struct pci_dev* pci_dev = (struct pci_dev*)dal_master[lchip]->dal_dev.pci_dev;
        if (DAL_MSI_TYPE_MSI == msi_type)
        {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0))
            pci_free_irq_vectors(pci_dev);
#else
            pci_disable_msi(pci_dev);
#endif
        }
        else
        {
            pci_disable_msix(pci_dev);
        }
        memset(dal_master[lchip]->msi_irq_base, 0, sizeof(unsigned int)*DAL_MAX_INTR_NUM);
        dal_master[lchip]->msi_irq_num = 0;
        dal_master[lchip]->msi_used = 0;
    }
#endif

    return 0;
}

int
dal_set_msi_cap(unsigned long arg)
{
    int ret = 0;
    int index = 0;
    dal_msi_info_t msi_info;

    if (copy_from_user(&msi_info, (void*)arg, sizeof(dal_msi_info_t)))
    {
        return -EFAULT;
    }

    printk("####dal_set_msi_cap lchip %d base %d num:%d\n", msi_info.lchip, msi_info.irq_base[0], msi_info.irq_num);
    if (DAL_CPU_MODE_TYPE_PCIE == dal_master[msi_info.lchip]->active_type)
    {
        if (msi_info.irq_num > 0)
        {
            if (0 == dal_master[msi_info.lchip]->msi_used)
            {
                ret = _dal_set_msi_enabe(msi_info.lchip, msi_info.irq_num, msi_info.msi_type);
            }
            else if ((1 == dal_master[msi_info.lchip]->msi_used) && (msi_info.irq_num != dal_master[msi_info.lchip]->msi_irq_num))
            {
                for (index = 0; index < dal_master[msi_info.lchip]->msi_irq_num; index++)
                {
                    dal_interrupt_unregister(dal_master[msi_info.lchip]->msi_irq_base[index]);
                }
                _dal_set_msi_disable(msi_info.lchip, msi_info.msi_type);
                ret = _dal_set_msi_enabe(msi_info.lchip, msi_info.irq_num, msi_info.msi_type);
            }
        }
        else
        {
            for (index = 0; index < dal_master[msi_info.lchip]->msi_irq_num; index++)
            {
                if(dal_glb.dal_isr[index].count != 0)
                {
                    printk("####dal_set_msi_cap lchip %d, isr count is %d, not ready to disable msi", msi_info.lchip, dal_glb.dal_isr[index].count);
                    return 0;
                }
            }
            ret = _dal_set_msi_disable(msi_info.lchip, msi_info.msi_type);
        }
    }

    return ret;
}

int
dal_user_interrupt_register(unsigned long arg)
{
    dal_intr_parm_t intr_param;

    if (copy_from_user(&intr_param, (void*)arg, sizeof(dal_intr_parm_t)))
    {
        return -EFAULT;
    }
    printk("####register interrupt irq:%d\n", intr_param.irq);
    return dal_interrupt_register(intr_param.irq, 0, NULL, NULL);
}

int
dal_user_interrupt_unregister(unsigned long arg)
{
    int irq = 0;
    if (copy_from_user(&irq, (void*)arg, sizeof(int)))
    {
        return -EFAULT;
    }
    printk("####unregister interrupt irq:%d\n", irq);
    return dal_interrupt_unregister(irq);
}

int
dal_user_interrupt_set_en(unsigned long arg)
{
    dal_intr_parm_t dal_intr_parm;

    if (copy_from_user(&dal_intr_parm, (void*)arg, sizeof(dal_intr_parm_t)))
    {
        return -EFAULT;
    }

    return dal_interrupt_set_en(dal_intr_parm.irq, dal_intr_parm.enable);
}

#if !defined DMA_MEM_MODE_PLATFORM && !defined SOC_ACTIVE
/*
 * Function: _dal_dma_segment_free
 */

/*
 * Function: _find_largest_segment
 *
 * Purpose:
 *    Find largest contiguous segment from a pool of DMA blocks.
 * Parameters:
 *    dseg - DMA segment descriptor
 * Returns:
 *    0 on success, < 0 on error.
 * Notes:
 *    Assembly stops if a segment of the requested segment size
 *    has been obtained.
 *
 *    Lower address bits of the DMA blocks are used as follows:
 *       0: Untagged
 *       1: Discarded block
 *       2: Part of largest contiguous segment
 *       3: Part of current contiguous segment
 */
static int
_dal_find_largest_segment(dma_segment_t* dseg)
{
    int i, j, blks, found;
    unsigned long seg_begin;
    unsigned long seg_end;
    unsigned long seg_tmp;

    blks = dseg->blk_cnt;

    /* Clear all block tags */
    for (i = 0; i < blks; i++)
    {
        dseg->blk_ptr[i] &= ~3;
    }

    for (i = 0; i < blks && dseg->seg_size < dseg->req_size; i++)
    {
        /* First block must be an untagged block */
        if ((dseg->blk_ptr[i] & 3) == DAL_UNTAG_BLOCK)
        {
            /* Initial segment size is the block size */
            seg_begin = dseg->blk_ptr[i];
            seg_end = seg_begin + dseg->blk_size;
            dseg->blk_ptr[i] |= DAL_CUR_MATCH_BLOCk;

            /* Loop looking for adjacent blocks */
            do
            {
                found = 0;

                for (j = i + 1; j < blks && (seg_end - seg_begin) < dseg->req_size; j++)
                {
                    seg_tmp = dseg->blk_ptr[j];
                    /* Check untagged blocks only */
                    if ((seg_tmp & 3) == DAL_UNTAG_BLOCK)
                    {
                        if (seg_tmp == (seg_begin - dseg->blk_size))
                        {
                            /* Found adjacent block below current segment */
                            dseg->blk_ptr[j] |= DAL_CUR_MATCH_BLOCk;
                            seg_begin = seg_tmp;
                            found = 1;
                        }
                        else if (seg_tmp == seg_end)
                        {
                            /* Found adjacent block above current segment */
                            dseg->blk_ptr[j] |= DAL_CUR_MATCH_BLOCk;
                            seg_end += dseg->blk_size;
                            found = 1;
                        }
                    }
                }
            }
            while (found);

            if ((seg_end - seg_begin) > dseg->seg_size)
            {
                /* The current block is largest so far */
                dseg->seg_begin = seg_begin;
                dseg->seg_end = seg_end;
                dseg->seg_size = seg_end - seg_begin;

                /* Re-tag current and previous largest segment */
                for (j = 0; j < blks; j++)
                {
                    if ((dseg->blk_ptr[j] & 3) == DAL_CUR_MATCH_BLOCk)
                    {
                        /* Tag current segment as the largest */
                        dseg->blk_ptr[j] &= ~1;
                    }
                    else if ((dseg->blk_ptr[j] & 3) == DAL_MATCHED_BLOCK)
                    {
                        /* Discard previous largest segment */
                        dseg->blk_ptr[j] ^= 3;
                    }
                }
            }
            else
            {
                /* Discard all blocks in current segment */
                for (j = 0; j < blks; j++)
                {
                    if ((dseg->blk_ptr[j] & 3) == DAL_CUR_MATCH_BLOCk)
                    {
                        dseg->blk_ptr[j] &= ~2;
                    }
                }
            }
        }
    }

    return 0;
}

/*
 * Function: _alloc_dma_blocks
 */
static int
_dal_alloc_dma_blocks(dma_segment_t* dseg, int blks)
{
    int i, start;
    unsigned long addr;

    if (dseg->blk_cnt + blks > dseg->blk_cnt_max)
    {
        printk("No more DMA blocks\n");
        return -1;
    }

    start = dseg->blk_cnt;
    dseg->blk_cnt += blks;

    for (i = start; i < dseg->blk_cnt; i++)
    {
        addr = __get_free_pages(GFP_ATOMIC, dseg->blk_order);
        if (addr)
        {
            dseg->blk_ptr[i] = addr;
        }
        else
        {
            printk("DMA allocation failed\n");
            return -1;
        }
    }

    return 0;
}

/*
 * Function: _dal_dma_segment_alloc
 */
static dma_segment_t*
_dal_dma_segment_alloc(unsigned int size, unsigned int blk_size)
{
    dma_segment_t* dseg;
    int i, blk_ptr_size;
    unsigned long page_addr;
    struct sysinfo si;

    /* Sanity check */
    if (size == 0 || blk_size == 0)
    {
        return NULL;
    }

    /* Allocate an initialize DMA segment descriptor */
    if ((dseg = kmalloc(sizeof(dma_segment_t), GFP_ATOMIC)) == NULL)
    {
        return NULL;
    }

    memset(dseg, 0, sizeof(dma_segment_t));
    dseg->req_size = size;
    dseg->blk_size = PAGE_ALIGN(blk_size);

    while ((PAGE_SIZE << dseg->blk_order) < dseg->blk_size)
    {
        dseg->blk_order++;
    }

    si_meminfo(&si);
    dseg->blk_cnt_max = (si.totalram << PAGE_SHIFT) / dseg->blk_size;
    blk_ptr_size = dseg->blk_cnt_max * sizeof(unsigned long);
    /* Allocate an initialize DMA block pool */
    dseg->blk_ptr = kmalloc(blk_ptr_size, GFP_KERNEL);
    if (dseg->blk_ptr == NULL)
    {
        kfree(dseg);
        return NULL;
    }

    memset(dseg->blk_ptr, 0, blk_ptr_size);
    /* Allocate minimum number of blocks */
    _dal_alloc_dma_blocks(dseg, dseg->req_size / dseg->blk_size);

    /* Allocate more blocks until we have a complete segment */
    do
    {
        _dal_find_largest_segment(dseg);
        if (dseg->seg_size >= dseg->req_size)
        {
            break;
        }
    }
    while (_dal_alloc_dma_blocks(dseg, 8) == 0);

    /* Reserve all pages in the DMA segment and free unused blocks */
    for (i = 0; i < dseg->blk_cnt; i++)
    {
        if ((dseg->blk_ptr[i] & 3) == 2)
        {
            dseg->blk_ptr[i] &= ~3;

            for (page_addr = dseg->blk_ptr[i];
                 page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                 page_addr += PAGE_SIZE)
            {
                MEM_MAP_RESERVE(VIRT_TO_PAGE((void*)page_addr));
            }
        }
        else if (dseg->blk_ptr[i])
        {
            dseg->blk_ptr[i] &= ~3;
            free_pages(dseg->blk_ptr[i], dseg->blk_order);
            dseg->blk_ptr[i] = 0;
        }
    }

    return dseg;
}

/*
 * Function: _dal_dma_segment_free
 */
static void
_dal_dma_segment_free(dma_segment_t* dseg)
{
    int i;
    unsigned long page_addr;

    if (dseg->blk_ptr)
    {
        for (i = 0; i < dseg->blk_cnt; i++)
        {
            if (dseg->blk_ptr[i])
            {
                for (page_addr = dseg->blk_ptr[i];
                     page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                     page_addr += PAGE_SIZE)
                {
                    MEM_MAP_UNRESERVE(VIRT_TO_PAGE((void*)page_addr));
                }

                free_pages(dseg->blk_ptr[i], dseg->blk_order);
            }
        }

        kfree(dseg->blk_ptr);
        kfree(dseg);
    }
}

/*
 * Function: -dal_pgalloc
 */
static void*
_dal_pgalloc(unsigned int size)
{
    dma_segment_t* dseg;
    unsigned int blk_size;

    blk_size = (size < DMA_BLOCK_SIZE) ? size : DMA_BLOCK_SIZE;
    if ((dseg = _dal_dma_segment_alloc(size, blk_size)) == NULL)
    {
        return NULL;
    }

    if (dseg->seg_size < size)
    {
        /* If we didn't get the full size then forget it */
        printk("Notice: Can not get enough memory for requset!!\n");
        printk("actual size:0x%lx, request size:0x%x\n", dseg->seg_size, size);
         /*-_dal_dma_segment_free(dseg);*/
         /*-return NULL;*/
    }

    list_add(&dseg->list, &_dma_seg);
    return (void*)dseg->seg_begin;
}

/*
 * Function: _dal_pgfree
 */
static int
_dal_pgfree(void* ptr)
{
    struct list_head* pos;

    list_for_each(pos, &_dma_seg)
    {
        dma_segment_t* dseg = list_entry(pos, dma_segment_t, list);
        if (ptr == (void*)dseg->seg_begin)
        {
            list_del(&dseg->list);
            _dal_dma_segment_free(dseg);
            return 0;
        }
    }
    return -1;
}
#endif

#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
static struct device *_dma_alloc_coherent_dev = NULL;
static void* _dma_alloc_coherent_virt_base = NULL;
#endif

static void
dal_alloc_dma_pool(int lchip, int size)
{
#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
    struct device * dev = NULL;
#endif

    if (dal_glb.use_high_memory)
    {
        dal_master[lchip]->dma_phy_base = virt_to_bus(high_memory);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
        dal_master[lchip]->dma_virt_base = ioremap(dal_master[lchip]->dma_phy_base, size);
#else
        dal_master[lchip]->dma_virt_base = ioremap_nocache(dal_master[lchip]->dma_phy_base, size);
#endif
    }
    else
    {
#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
        if ((DAL_CPU_MODE_TYPE_PCIE != dal_master[lchip]->active_type) && (DAL_CPU_MODE_TYPE_LOCAL != dal_master[lchip]->active_type))
        {
            printk("active type %d error, not cpu and soc!\n", dal_master[lchip]->active_type);
            return;
        }

        if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
        {
            dev = &((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->dev;
        }
#if defined(SOC_ACTIVE)
        if (DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type)
        {
            dev = &((struct platform_device*)(dal_master[lchip]->dal_dev.pci_dev))->dev;
        }
#endif
        dal_master[lchip]->dma_virt_base = dma_alloc_coherent(dev, dal_glb.dma_mem_size,
                                                    &dal_master[lchip]->dma_phy_base, GFP_KERNEL);
        if (NULL == dal_master[lchip]->dma_virt_base)
        {
            printk("alloc coherent memory failed! request size is 0x%x\n", dal_glb.dma_mem_size);
        }
        _dma_alloc_coherent_dev = dev;
        _dma_alloc_coherent_virt_base = dal_master[lchip]->dma_virt_base;
        printk("lchip %u dma_phy_base 0x%llx dma_virt_base %"PRIADDR" \n", lchip, dal_master[lchip]->dma_phy_base, dal_master[lchip]->dma_virt_base);
        printk(KERN_WARNING "########Using DMA_MEM_MODE_PLATFORM \n");
#else
        /* Get DMA memory from kernel */
        dal_master[lchip]->dma_virt_base = _dal_pgalloc(size);
        dal_master[lchip]->dma_phy_base = virt_to_bus(dal_master[lchip]->dma_virt_base);
#endif
    }
}

static void
dal_free_dma_pool(int lchip)
{
    int ret = 0;
#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
    struct device * dev = NULL;
#endif

    ret = ret;
    if (dal_glb.use_high_memory)
    {
        iounmap(dal_master[lchip]->dma_virt_base);
    }
    else
    {
#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
        if ((DAL_CPU_MODE_TYPE_PCIE != dal_master[lchip]->active_type) && (DAL_CPU_MODE_TYPE_LOCAL != dal_master[lchip]->active_type))
        {
            return;
        }
        if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
        {
            dev = &((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->dev;
        }
#if defined(SOC_ACTIVE)
        if (DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type)
        {
            dev = &((struct platform_device*)(dal_master[lchip]->dal_dev.pci_dev))->dev;
        }
#endif
        dma_free_coherent(dev, dal_glb.dma_mem_size, dal_master[lchip]->dma_virt_base, dal_master[lchip]->dma_phy_base);
#else
        ret = _dal_pgfree(dal_master[lchip]->dma_virt_base);
        if(ret<0)
        {
            printk("Dma free memory fail !!!!!! \n");
        }
#endif
    }
}

#define _KERNEL_DAL_IO
static int
_dal_pci_read(unsigned char lchip, unsigned int offset, unsigned int* value)
{
    /*Notice:lchip here means pci_dev slot index*/
    if (!VERIFY_CHIP_INDEX(lchip))
    {
        return -1;
    }

    if ((DAL_CPU_MODE_TYPE_PCIE != dal_master[lchip]->active_type) && (DAL_CPU_MODE_TYPE_LOCAL != dal_master[lchip]->active_type)
		&& (DAL_CPU_MODE_TYPE_ASW != dal_master[lchip]->active_type))
    {
        return -1;
    }

    if ((DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type) || (DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type))
    {
        *value = *(volatile unsigned int*)(dal_master[lchip]->dal_dev.logic_address + offset);
    }

#if defined(ASW_ACTIVE)
    if (DAL_CPU_MODE_TYPE_ASW == active_type[lchip])
    {
        dal_kernel_asw_dev_t* dev = dal_dev[lchip];
        dev->reg_ops->read_reg(dev, offset, value);
    }
#endif

    return 0;
}

int
dal_create_irq_mapping(unsigned long arg)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))

#ifndef NO_IRQ
#define NO_IRQ (-1)
#endif
    dal_irq_mapping_t irq_map;

    if (copy_from_user(&irq_map, (void*)arg, sizeof(dal_irq_mapping_t)))
    {
        return -EFAULT;
    }

    irq_map.sw_irq = irq_create_mapping(NULL, irq_map.hw_irq);
    if (irq_map.sw_irq == NO_IRQ)
    {
        printk("IRQ mapping fail !!!!!! \n");
        return -1;
    }

    if (copy_to_user((dal_irq_mapping_t*)arg, (void*)&irq_map, sizeof(dal_irq_mapping_t)))
    {
        return -EFAULT;
    }
#endif
    return 0;
}

int
dal_pci_read(unsigned long arg)
{
    dal_chip_parm_t cmdpara_chip;

    if (copy_from_user(&cmdpara_chip, (void*)arg, sizeof(dal_chip_parm_t)))
    {
        return -EFAULT;
    }

    _dal_pci_read((unsigned char)cmdpara_chip.lchip, (unsigned int)cmdpara_chip.reg_addr,
                                                       (unsigned int*)(&(cmdpara_chip.value)));

    if (copy_to_user((dal_chip_parm_t*)arg, (void*)&cmdpara_chip, sizeof(dal_chip_parm_t)))
    {
        return -EFAULT;
    }

    return 0;
}

static int
_dal_pci_write(unsigned char lchip, unsigned int offset, unsigned int value)
{
    /*Notice:lchip here means pci_dev slot index*/
    if (!VERIFY_CHIP_INDEX(lchip))
    {
        return -1;
    }

    if ((DAL_CPU_MODE_TYPE_PCIE != dal_master[lchip]->active_type) && (DAL_CPU_MODE_TYPE_LOCAL != dal_master[lchip]->active_type)
		 && (DAL_CPU_MODE_TYPE_ASW != dal_master[lchip]->active_type))
    {
        return -1;
    }

     if ((DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type) || (DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type))
	 {
		*(volatile unsigned int*)(dal_master[lchip]->dal_dev.logic_address + offset) = value;
	 }

#if defined(ASW_ACTIVE)
    if (DAL_CPU_MODE_TYPE_ASW == active_type[lchip])
    {
        dal_kernel_asw_dev_t* dev = dal_dev[lchip];
        dev->reg_ops->write_reg(dev, offset, value);
    }
#endif

    return 0;
}

int
dal_pci_write(unsigned long arg)
{
    dal_chip_parm_t cmdpara_chip;

    if (copy_from_user(&cmdpara_chip, (void*)arg, sizeof(dal_chip_parm_t)))
    {
        return -EFAULT;
    }

    _dal_pci_write((unsigned char)cmdpara_chip.lchip, (unsigned int)cmdpara_chip.reg_addr,
                                                         (unsigned int)cmdpara_chip.value);

    return 0;
}

int
dal_pci_conf_read(unsigned char lchip, unsigned int offset, unsigned int* value)
{
    if (!VERIFY_CHIP_INDEX(lchip))
    {
        return -1;
    }

    if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
    {
        pci_read_config_dword(dal_master[lchip]->dal_dev.pci_dev, offset, value);
    }

    return 0;
}

int
dal_pci_conf_write(unsigned char lchip, unsigned int offset, unsigned int value)
{
    if (!VERIFY_CHIP_INDEX(lchip))
    {
        return -1;
    }

    if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
    {
        pci_write_config_dword(dal_master[lchip]->dal_dev.pci_dev, offset, value);
    }

    return 0;
}
int
dal_user_read_pci_conf(unsigned long arg)
{
    dal_pci_cfg_ioctl_t dal_cfg;

    if (copy_from_user(&dal_cfg, (void*)arg, sizeof(dal_pci_cfg_ioctl_t)))
    {
        return -EFAULT;
    }

    if (dal_pci_conf_read(dal_cfg.ldev, dal_cfg.offset, &dal_cfg.value))
    {
        printk("dal_pci_conf_read failed.\n");
        return -EFAULT;
    }

    if (copy_to_user((dal_pci_cfg_ioctl_t*)arg, (void*)&dal_cfg, sizeof(dal_pci_cfg_ioctl_t)))
    {
        return -EFAULT;
    }

    return 0;
}

int
dal_user_write_pci_conf(unsigned long arg)
{
    dal_pci_cfg_ioctl_t dal_cfg;

    if (copy_from_user(&dal_cfg, (void*)arg, sizeof(dal_pci_cfg_ioctl_t)))
    {
        return -EFAULT;
    }

    return dal_pci_conf_write(dal_cfg.ldev, dal_cfg.offset, dal_cfg.value);
}

static int
linux_get_device(unsigned long arg, int is_compat)
{
    dal_user_dev_t user_dev;
    dal_user32_dev_t user32_dev;
    int lchip = 0;
    int ret = 0;

    ret = is_compat ? copy_from_user(&user32_dev, (void*)arg, sizeof(dal_user32_dev_t)) : \
                      copy_from_user(&user_dev, (void*)arg, sizeof(dal_user_dev_t));
    if (ret)
    {
        return -EFAULT;
    }

    user_dev.chip_num = dal_glb.dal_chip_num;
    lchip = is_compat ? user32_dev.ldev : user_dev.ldev;

    if (lchip < CTC_MAX_LOCAL_CHIP_NUM && dal_master[lchip])
    {
        if (DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type)
        {
            user_dev.phy_base0 = (unsigned int)(dal_master[lchip]->dal_dev.phys_address);
#ifdef PHYS_ADDR_IS_64BIT
            user_dev.phy_base1 = (unsigned int)(dal_master[lchip]->dal_dev.phys_address >> 32);
#endif
            user_dev.domain_no = pci_domain_nr(((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->bus);
            user_dev.bus_no = ((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->bus->number;
            user_dev.dev_no = ((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->device;
            user_dev.fun_no = ((struct pci_dev*)(dal_master[lchip]->dal_dev.pci_dev))->devfn;
            user_dev.soc_active = 0;
        }

#if defined(SOC_ACTIVE)
        if (DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type)
        {
            user_dev.phy_base0 = (unsigned int)(dal_master[lchip]->dal_dev.phys_address);
            user_dev.phy_base1 = (unsigned int)(dal_master[lchip]->dal_dev.phys_address >> 32);
            user_dev.bus_no = 0;
            user_dev.dev_no = dal_master[lchip]->dev_no;    //-DAL_TSINGMA_DEVICE_ID;
            user_dev.fun_no = 0;
            user_dev.soc_active = 1;
        }
#endif
#if defined(ASW_ACTIVE)
        if (DAL_CPU_MODE_TYPE_ASW == active_type[lchip])
        {
            user_dev.phy_base0 = 0;
            user_dev.phy_base1 = 0;
            user_dev.dma_phy_base0 = 0;
            user_dev.dma_phy_base1 = 0;
            user_dev.bus_no = 0;
            user_dev.dev_no = DAL_TSINGMA_AX_DEVICE_ID;
            user_dev.fun_no = 0;
            user_dev.domain_no = 0;
            user_dev.soc_active = 0;
        }
#endif
    }

    memcpy(&user32_dev, &user_dev, (uintptr)&((dal_user_dev_t*)0)->virt_base);
    user32_dev.domain_no = user_dev.domain_no;
    ret = is_compat ? copy_to_user((dal_user32_dev_t*)arg, (void*)&user32_dev, sizeof(dal_user32_dev_t)) : \
                      copy_to_user((dal_user_dev_t*)arg, (void*)&user_dev, sizeof(dal_user_dev_t));
    if (ret)
    {
        return -EFAULT;
    }

    return 0;
}

/* set dal version, copy to user */
static int
linux_get_dal_version(unsigned long arg)
{
    int dal_ver = DAL_VER(DAL_CUR_VER, DAL_CUR_S_VER);    /* set dal version */
    
    if (copy_to_user((int*)arg, (void*)&dal_ver, sizeof(dal_ver)))
    {
        return -EFAULT;
    }

    dal_glb.dal_version = dal_ver;         /* up sw */

    return 0;
}

static int
linux_get_dma_info(unsigned long arg)
{
    dal_dma_info_t dma_para;

    if (copy_from_user(&dma_para, (void*)arg, sizeof(dal_dma_info_t)))
    {
        return -EFAULT;
    }

    if (dal_master[dma_para.ldev])
    {
    dma_para.phy_base = (unsigned int)dal_master[dma_para.ldev]->dma_phy_base;
    dma_para.phy_base_hi = dal_master[dma_para.ldev]->dma_phy_base >> 32;
    dma_para.virt_base = (uintptr)dal_master[dma_para.ldev]->dma_virt_base;
    dma_para.size = (NULL != dal_master[dma_para.ldev]->dma_virt_base) ? dal_glb.dma_mem_size : 0;

    printk("dal dma phy addr: 0x%llx, virt addr: %"PRIADDR".\n", dal_master[dma_para.ldev]->dma_phy_base, dal_master[dma_para.ldev]->dma_virt_base);
    }
    if (copy_to_user((dal_dma_info_t*)arg, (void*)&dma_para, sizeof(dal_dma_info_t)))
    {
        return -EFAULT;
    }

    return 0;
}

static int
dal_get_msi_info(unsigned long arg)
{
    dal_msi_info_t msi_para;
    unsigned int lchip = 0;
    unsigned int index = 0;

    /* get lchip form user mode */
    if (copy_from_user(&msi_para, (void*)arg, sizeof(dal_msi_info_t)))
    {
        return -EFAULT;
    }
    lchip = msi_para.lchip;

    if ((DAL_CPU_MODE_TYPE_PCIE == dal_master[lchip]->active_type
       || DAL_CPU_MODE_TYPE_LOCAL == dal_master[lchip]->active_type) && dal_master[lchip])
    {
        msi_para.irq_num = dal_master[lchip]->msi_irq_num;
        for (index=0; index<msi_para.irq_num; index++)
        {
            msi_para.irq_base[index] = dal_master[lchip]->msi_irq_base[index];
        }
    }

    /* send msi info to user mode */
    if (copy_to_user((dal_msi_info_t*)arg, (void*)&msi_para, sizeof(dal_msi_info_t)))
    {
        return -EFAULT;
    }

    return 0;
}

static int
dal_get_intr_info(unsigned long arg)
{
    dal_intr_info_t intr_para;
    unsigned int intr_num = 0;

    /* get lchip form user mode */
    if (copy_from_user(&intr_para, (void*)arg, sizeof(dal_intr_info_t)))
    {
        return -EFAULT;
    }

    intr_para.irq_idx = DAL_MAX_INTR_NUM;
    for (intr_num=0; intr_num< DAL_MAX_INTR_NUM; intr_num++)
    {
        if (intr_para.irq == dal_glb.dal_isr[intr_num].irq)
        {
            intr_para.irq_idx = intr_num;
            break;
        }
    }

    if (DAL_MAX_INTR_NUM == intr_para.irq_idx)
    {
        printk("Interrupt %d cann't find.\n", intr_para.irq);
    }
    /* send msi info to user mode */
    if (copy_to_user((dal_intr_info_t*)arg, (void*)&intr_para, sizeof(dal_intr_info_t)))
    {
        return -EFAULT;
    }

    return 0;
}

int
dal_cache_inval(unsigned long long ptr, unsigned int length)
{
#ifdef DMA_CACHE_COHERENCE_EN
    /*dma_cache_wback_inv((unsigned long)intr_para.ptr, intr_para.length);*/

    dma_sync_single_for_cpu(NULL, ptr, length, DMA_FROM_DEVICE);

    /*dma_cache_sync(NULL, (void*)bus_to_virt(intr_para.ptr), intr_para.length, DMA_FROM_DEVICE);*/
#endif
    return 0;
}

int
dal_cache_flush(unsigned long long ptr, unsigned int length)
{
#ifdef DMA_CACHE_COHERENCE_EN
    /*dma_cache_wback_inv(intr_para.ptr, intr_para.length);*/

    dma_sync_single_for_device(NULL, ptr, length, DMA_TO_DEVICE);

    /*dma_cache_sync(NULL, (void*)bus_to_virt(intr_para.ptr), intr_para.length, DMA_TO_DEVICE);*/
#endif
    return 0;
}

static int
dal_user_cache_inval(unsigned long arg)
{
#ifndef DMA_MEM_MODE_PLATFORM
    dal_dma_cache_info_t intr_para;

    if (copy_from_user(&intr_para, (void*)arg, sizeof(dal_dma_cache_info_t)))
    {
        return -EFAULT;
    }

    dal_cache_inval(intr_para.ptr, intr_para.length);
#endif
    return 0;
}

static int
dal_user_cache_flush(unsigned long arg)
{
#ifndef DMA_MEM_MODE_PLATFORM
    dal_dma_cache_info_t intr_para;

    if (copy_from_user(&intr_para, (void*)arg, sizeof(dal_dma_cache_info_t)))
    {
        return -EFAULT;
    }

    dal_cache_flush(intr_para.ptr, intr_para.length);
#endif
    return 0;
}

#ifdef ASW_ACTIVE
void
dal_smi_set_timeout_thrd(dal_kernel_asw_dev_t *cfg, int cnt)
{
    struct mii_bus *mbus = cfg->ext_mbus;
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_TIMEOUT_THRD_L, (unsigned short int)cnt);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_TIMEOUT_THRD_H, (unsigned short int)(cnt >> 16));
    CTC_PRINTK("DAL_SMI_TIMEOUT_THRD_L(0x%x), DAL_SMI_TIMEOUT_THRD_H(0x%x)\n",
                mbus->read(mbus, cfg->mdio_addr, DAL_SMI_TIMEOUT_THRD_L),
                mbus->read(mbus, cfg->mdio_addr, DAL_SMI_TIMEOUT_THRD_H));
}
void dal_smi_debug_status(dal_kernel_asw_dev_t *cfg)
{
    struct mii_bus *mbus = cfg->ext_mbus;
    unsigned short int debug_status = 0;
    unsigned char access_cnt, ack_cnt, timeout_cnt;
    debug_status = mbus->read(mbus, cfg->mdio_addr, DAL_SMI_DEBUG_STATUS);
    access_cnt = debug_status & 0xFF;
    ack_cnt = debug_status >> 8;
    timeout_cnt = access_cnt - ack_cnt;
    CTC_PRINTK("%s: debug_status is 0x%x, access cnt is %d, ack cnt is %d, timeout cnt is %d\n", __func__,
            debug_status, access_cnt, ack_cnt, timeout_cnt);
}
int dal_smi_read_reg(void *p_cfg, unsigned int addr, unsigned int *data)
{
    dal_kernel_asw_dev_t *cfg = p_cfg;
	struct mii_bus *mbus = cfg->ext_mbus;
	unsigned short int status = 0;
	unsigned short int data_l, data_h;
	unsigned long start_time;
	mutex_lock(&mbus->mdio_lock);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_ADDR_L, (unsigned short int)addr);
    CTC_PRINTK("DAL_SMI_ADDR_L(0x%x) w 0x%x\n", DAL_SMI_ADDR_L, (unsigned short int)addr);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_ADDR_H, (unsigned short int)(addr >> 16));
    CTC_PRINTK("DAL_SMI_ADDR_H(0x%x) w 0x%x\n", DAL_SMI_ADDR_H, (unsigned short int)(addr >> 16));
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_CMD, DAL_SMI_ACT);
    CTC_PRINTK("DAL_SMI_CMD(0x%x) w 0x%x\n", DAL_SMI_CMD, DAL_SMI_ACT);
    start_time = jiffies;
    while(1)
    {
        status = mbus->read(mbus, cfg->mdio_addr, DAL_SMI_STATUS);
        if (status & SMI_STATUS_ERROR)
        {
            dal_smi_debug_status(cfg);
            mutex_unlock(&mbus->mdio_lock);
            return -1;
        } else if ((status & SMI_STATUS_TIMEOUT) | (time_after(jiffies, start_time + DAL_SMI_TIMEOUT)))
        {
            dal_smi_debug_status(cfg);
            mutex_unlock(&mbus->mdio_lock);
            return -1;
        }
        else if (status & SMI_STATUS_END)
        {
            CTC_PRINTK("DAL_SMI_STATUS(0x%x) r 0x%x\n", DAL_SMI_STATUS, status);
            break;
        }
    }
    data_l = mbus->read(mbus, cfg->mdio_addr, DAL_SMI_RD_DATA_L);
    CTC_PRINTK("DAL_SMI_RD_DATA_L(0x%x) r 0x%x\n", DAL_SMI_RD_DATA_L, data_l);
    data_h = mbus->read(mbus, cfg->mdio_addr, DAL_SMI_RD_DATA_H);
    CTC_PRINTK("DAL_SMI_RD_DATA_H(0x%x) r 0x%x\n", DAL_SMI_RD_DATA_H, data_h);
    *data = data_h << 16 | data_l;
    CTC_PRINTK("ctc2118_mdio_read_reg, addr is 0x%x, data is 0x%x\n", addr, *data);
    mutex_unlock(&mbus->mdio_lock);
    return 0;
}
int dal_smi_write_reg(void *p_cfg, unsigned int addr, unsigned int data)
{
    dal_kernel_asw_dev_t *cfg = p_cfg;
    struct mii_bus *mbus = cfg->ext_mbus;
    unsigned short int status = 0;
    unsigned long start_time;
    mutex_lock(&mbus->mdio_lock);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_ADDR_L, (unsigned short int)addr);
    	CTC_PRINTK("DAL_SMI_ADDR_L(0x%x) w 0x%x\n", DAL_SMI_ADDR_L, (unsigned short int)addr);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_ADDR_H, (unsigned short int)(addr >> 16));
    CTC_PRINTK("DAL_SMI_ADDR_H(0x%x) w 0x%x\n", DAL_SMI_ADDR_H, (unsigned short int)(addr >> 16));
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_WR_DATA_L, (unsigned short int)data);
    CTC_PRINTK("DAL_SMI_WR_DATA_L(0x%x) w 0x%x\n", DAL_SMI_WR_DATA_L, (unsigned short int)data);
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_WR_DATA_H, (unsigned short int)(data >> 16));
    CTC_PRINTK("DAL_SMI_WR_DATA_H(0x%x) w 0x%x\n", DAL_SMI_WR_DATA_H, (unsigned short int)(data >> 16));
    mbus->write(mbus, cfg->mdio_addr, DAL_SMI_CMD, DAL_SMI_ACT | DAL_SMI_WRITE);
    CTC_PRINTK("DAL_SMI_CMD(0x%x) w 0x%x\n", DAL_SMI_CMD, DAL_SMI_ACT | DAL_SMI_WRITE);
    start_time = jiffies;
    while (1) 
    {
        status = mbus->read(mbus, cfg->mdio_addr, DAL_SMI_STATUS);
        if (status & SMI_STATUS_ERROR) 
        {
            dal_smi_debug_status(cfg);
            mutex_unlock(&mbus->mdio_lock);
            return -1;
        }
        else if ((status & SMI_STATUS_TIMEOUT) | (time_after(jiffies, start_time + DAL_SMI_TIMEOUT)))
        {
            dal_smi_debug_status(cfg);
            mutex_unlock(&mbus->mdio_lock);
            return -1;
        } 
        else if (status & SMI_STATUS_END) 
        {
            CTC_PRINTK("DAL_SMI_STATUS(0x%x) r 0x%x\n", DAL_SMI_STATUS, status);
            break;
        }
    }
    CTC_PRINTK("ctc2118_mdio_write_reg, addr is 0x%x, data is 0x%x\n", addr, data);
    mutex_unlock(&mbus->mdio_lock);
    return 0;
}
static struct dal_reg_ops smi_reg_ops = {
	.write_reg = dal_smi_write_reg,
	.read_reg = dal_smi_read_reg,
};
dal_kernel_asw_dev_t *dal_smi_probe_of(struct platform_device *pdev, dal_kernel_asw_dev_t *cfg)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *mdio_node;
    unsigned int ret,tmp;
    mdio_node = of_parse_phandle(np, "mii-bus", 0);
    if (!mdio_node) 
    {
        dev_err(&pdev->dev, "cannot find mdio node phandle");
        return NULL;
    }
    cfg->ext_mbus = of_mdio_find_bus(mdio_node);
    if (!cfg->ext_mbus)
    {
        dev_info(&pdev->dev, "cannot find mdio bus from bus handle (yet)");
        return NULL;
    }
    
    ret = of_property_read_u32(np, "mii-addr", &tmp);
    if (ret) {
    	CTC_PRINTK("ctc2118 cant get data from file dts\n");
    } else {
    	cfg->mdio_addr = tmp;
    }
    CTC_PRINTK("mdio slave id 0x%x,ret = 0x%x\n", tmp,ret);
    cfg->reg_ops = &smi_reg_ops;
    dal_smi_set_timeout_thrd(cfg, DAL_SMI_TIMEOUT_CNT);
    return cfg;
}
unsigned char i2c_write_byte(dal_kernel_asw_dev_t *cfg, unsigned char reg, unsigned char val)
{
    struct i2c_msg msgs;
    unsigned char buf[2] = { reg, val };
    msgs.flags = 0;
    msgs.addr = cfg->i2c_addr;
    msgs.len = 2;
    msgs.buf = buf;
    CTC_PRINTK("i2c write 0x%x 0x%x 0x%x\n", cfg->i2c_addr, reg, val);
    return i2c_transfer(cfg->i2c_adapt, &msgs, 1);
}
unsigned char i2c_read_byte(dal_kernel_asw_dev_t *cfg, unsigned char reg)
{
    struct i2c_msg msgs[2];
    unsigned char buf1[] = {0x00};
    unsigned char buf2[] = {0x00};
    buf1[0] = reg;
    msgs[0].flags = 0;
    msgs[0].addr = cfg->i2c_addr;
    msgs[0].len = 1;
    msgs[0].buf = buf1;
    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = cfg->i2c_addr;
    msgs[1].len = 1;
    msgs[1].buf = buf2;
    i2c_transfer(cfg->i2c_adapt, msgs, 2);
    CTC_PRINTK("i2c read 0x%x 0x%x, val is 0x%x\n", cfg->i2c_addr, reg, buf2[0]);
    return buf2[0];
}
int dal_i2c_device_id(dal_kernel_asw_dev_t *cfg)
{
    unsigned char buf[4] = {0x00, 0x00, 0x00, 0x00};
    i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDREAD | CHIPACCESS_DEVICEID);
    buf[0] = i2c_read_byte(cfg, DAL_I2C_RD_DATA3);
    buf[1] = i2c_read_byte(cfg, DAL_I2C_RD_DATA2);
    buf[2] = i2c_read_byte(cfg, DAL_I2C_RD_DATA1);
    buf[3] = i2c_read_byte(cfg, DAL_I2C_RD_DATA0);
    CTC_PRINTK("%s: i2cslave device id is 0x%x\n", __func__, buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);
    return 0;
}
int dal_i2c_status(dal_kernel_asw_dev_t *cfg)
{
    unsigned char buf[4] = {0x00, 0x00, 0x00, 0x00};
    unsigned long start_time;
    start_time = jiffies;
    while (1) 
    {
        i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDREAD | CHIPACCESS_STATUS);
        buf[0] = i2c_read_byte(cfg, DAL_I2C_RD_DATA3);
        buf[1] = i2c_read_byte(cfg, DAL_I2C_RD_DATA2);
        buf[2] = i2c_read_byte(cfg, DAL_I2C_RD_DATA1);
        buf[3] = i2c_read_byte(cfg, DAL_I2C_RD_DATA0);
        CTC_PRINTK("CHIPACCESS_Status is %x\n", buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3]);
        if (buf[0] & I2C_STATUS_OVERLAP) 
        {
            CTC_PRINTK("%s: i2cslave read overlap\n", __func__);
            return -1;
        } 
        else if (buf[0] & I2C_STATUS_ERROR)
        {
            CTC_PRINTK("%s: i2cslave read error\n", __func__);
            return -1;
        } 
        else if ((buf[0] & I2C_STATUS_TIMEOUT) | (time_after(jiffies, start_time + I2C_TIMEOUT)))
        {
            CTC_PRINTK("%s: i2cslave read timeout\n", __func__);
            return -1;
        }
        else if (buf[0] & I2C_STATUS_DONE) 
        {
            break;
        }
    }
    return 0;
}
int dal_i2c_read_reg(void *p_cfg, unsigned int addr, unsigned int *data)
{
    unsigned int ret;
    unsigned char buf[4] = {0x00, 0x00, 0x00, 0x00};
    dal_kernel_asw_dev_t *cfg = p_cfg;
    i2c_write_byte(cfg, DAL_I2C_WR_DATA3, (addr >> 24) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA2, (addr >> 16) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA1, (addr >> 8) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA0, (addr >> 0) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDWRITE | CHIPACCESS_ADDRCMD);
    ret = dal_i2c_status(cfg);
    if (ret)
        return -1;
    i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDREAD | CHIPACCESS_RDDATE);
    buf[0] = i2c_read_byte(cfg, DAL_I2C_RD_DATA3);
    buf[1] = i2c_read_byte(cfg, DAL_I2C_RD_DATA2);
    buf[2] = i2c_read_byte(cfg, DAL_I2C_RD_DATA1);
    buf[3] = i2c_read_byte(cfg, DAL_I2C_RD_DATA0);
    dal_i2c_status(cfg);
    *data = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
    CTC_PRINTK("ctc2118_i2c_read_reg, addr is 0x%x, data is 0x%x\n", addr, *data);	
    return 0;
}
int dal_i2c_write_reg(void *p_cfg, unsigned int addr, unsigned int data)
{
    unsigned int ret;
	dal_kernel_asw_dev_t *cfg = p_cfg;
    i2c_write_byte(cfg, DAL_I2C_WR_DATA3, (data >> 24) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA2, (data >> 16) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA1, (data >> 8) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA0, (data >> 0) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDWRITE | CHIPACCESS_WRDATA);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA3, (addr >> 24) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA2, (addr >> 16) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA1, (addr >> 8) & 0xff);
    i2c_write_byte(cfg, DAL_I2C_WR_DATA0, ((addr >> 0) & 0xff) | 0x1);
    i2c_write_byte(cfg, DAL_I2C_ADDR_CMD, CHIPACCESS_CMDWRITE | CHIPACCESS_ADDRCMD);
    ret = dal_i2c_status(cfg);
    if (ret)
        return -1;
    CTC_PRINTK("ctc2118_i2c_write_reg, addr is 0x%x, data is 0x%x\n", addr, data);
    return 0;
}
static struct dal_reg_ops i2c_reg_ops = {
    .write_reg = dal_i2c_write_reg,
    .read_reg = dal_i2c_read_reg,
};
dal_kernel_asw_dev_t *dal_i2c_probe_of(struct platform_device *pdev, dal_kernel_asw_dev_t *cfg)
{
    struct device_node *np = pdev->dev.of_node;
    struct device_node *i2c_node;
    unsigned int ret,tmp;
    i2c_node = of_parse_phandle(np, "i2c-bus", 0);
    if (!i2c_node)
    {
        dev_err(&pdev->dev, "cannot find i2c node phandle");
        return NULL;
    }
    cfg->i2c_adapt = of_find_i2c_adapter_by_node(i2c_node);
    if (!cfg->i2c_adapt)
    {
        dev_err(&pdev->dev, "cannot find i2c bus from bus handle (yet)");
        return NULL;
    } 
    else 
    {
        of_node_put(i2c_node);
    }
    ret = of_property_read_u32(np, "slave-addr", &tmp);
    if (ret) {
        CTC_PRINTK("ctc2118 cant get data from file dts\n");
    } else {
    	cfg->i2c_addr = tmp;
    }
    CTC_PRINTK("i2c slave id 0x%x ret = 0x%x\n", tmp,ret);
    dal_i2c_device_id(cfg);
    cfg->reg_ops = &i2c_reg_ops;
    return cfg;
}
int dal_spi_read_reg(void *p_cfg, unsigned int addr, unsigned int *data)
{
    struct spi_transfer t[4] = {};
    struct spi_message m;
    unsigned char tx_cmd[1];
    unsigned char tx_addr[4];
    unsigned char dummy[1];
    unsigned char rx_data[4];
    dal_kernel_asw_dev_t *cfg = p_cfg;
    spi_message_init(&m);
    tx_cmd[0] = CTC_SPI_CMD_READ;
    t[0].tx_buf = tx_cmd;
    t[0].len = 1;
    t[0].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[0], &m);
    tx_addr[0] = addr >> 24;
    tx_addr[1] = addr >> 16;
    tx_addr[2] = addr >> 8;
    tx_addr[3] = addr >> 0;
    t[1].tx_buf = tx_addr;
    t[1].len = 4;
    t[1].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[1], &m);
    dummy[0] = 0xff;
    t[2].tx_buf = dummy;
    t[2].len = 1;
    t[2].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[2], &m);
    t[3].rx_buf = rx_data;
    t[3].len = 4;
    t[3].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[3], &m);
    spi_sync(cfg->spi_device, &m);
    *data = rx_data[0] << 24 | rx_data[1] << 16 | rx_data[2] << 8 | rx_data[3];
    if (*data == CTC_SPI_STATUS_ERR) 
    {
        CTC_PRINTK("%s: spislave read error\n", __func__);
    }
    if (*data == CTC_SPI_STATUS_TIMEOUT) 
    {
        CTC_PRINTK("%s: spislave read timeout\n", __func__);
    }
    if (*data == CTC_SPI_STATUS_NODATA) 
    {
        CTC_PRINTK("%s: spislave read no data\n", __func__);
    }
    CTC_PRINTK("0x%x, 0x%x, 0x%x, 0x%x\n", rx_data[0], rx_data[1], rx_data[2], rx_data[3]);
    CTC_PRINTK("ctc2118_spi_read_reg, addr is 0x%x, data is 0x%x\n", addr, *data);
    return 0;
}
int dal_spi_write_reg(void *p_cfg, unsigned int addr, unsigned int data)
{
    struct spi_transfer t[3] = {};
    struct spi_message m;
    unsigned char tx_cmd[1];
    unsigned char tx_addr[4];
    unsigned char tx_data[4];
    dal_kernel_asw_dev_t *cfg = p_cfg;
    spi_message_init(&m);
    tx_cmd[0] = CTC_SPI_CMD_WRITE;
    t[0].tx_buf = tx_cmd;
    t[0].len = 1;
    t[0].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[0], &m);
    tx_addr[0] = addr >> 24;
    tx_addr[1] = addr >> 16;
    tx_addr[2] = addr >> 8;
    tx_addr[3] = addr >> 0;
    t[1].tx_buf = tx_addr;
    t[1].len = 4;
    t[1].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[1], &m);	
    tx_data[0] = data >> 24;
    tx_data[1] = data >> 16;
    tx_data[2] = data >> 8;
    tx_data[3] = data >> 0;
    t[2].tx_buf = tx_data;
    t[2].len = 4;
    t[2].delay_usecs = DAL_SPI_OP_DELAY_US;
    spi_message_add_tail(&t[2], &m);
    spi_sync(cfg->spi_device, &m);
    CTC_PRINTK("ctc2118_spi_write_reg, addr is 0x%x, data is 0x%x\n", addr, data);	
    return 0;
}
static struct dal_reg_ops spi_reg_ops = {
    .write_reg = dal_spi_write_reg,
    .read_reg = dal_spi_read_reg,
};
dal_kernel_asw_dev_t *dal_spi_probe_of(struct platform_device *pdev, dal_kernel_asw_dev_t *cfg)
{
    struct spi_master *master;
    unsigned int value, ret;
    struct device_node *np = pdev->dev.of_node;
    ret = of_property_read_u32(np, "bus-num", &value);
    if (ret) {
        dev_err(&pdev->dev, "has no valid 'bus-num' property (%d)\n", ret);
        return NULL;
    }
    master = spi_busnum_to_master(value);
    cfg->spi_device = spi_alloc_device(master);
    cfg->spi_device->mode = SPI_MODE_3;
    cfg->spi_device->dev.of_node = pdev->dev.of_node;
    ret = of_property_read_u32(np, "spi-max-frequency", &value);
    if (ret) {
        cfg->spi_device->max_speed_hz = 1000000;
    	CTC_PRINTK("ctc2118 cant get data from file dts\n");
    } else {
        cfg->spi_device->max_speed_hz = value;
    }
    CTC_PRINTK("max_speed_hz %d,ret = 0x%x\n", value,ret);
    ret = of_property_read_u32(np, "chip-select", &value);
    if (ret) {
        cfg->spi_device->chip_select = cfg->spi_addr;
        CTC_PRINTK("ctc2118 cant get data from file dts\n");
    } else {
        cfg->spi_device->chip_select = value;
    }
    CTC_PRINTK("spi slave id 0x%x,ret = 0x%x\n", value,ret);

    spi_add_device(cfg->spi_device);
    cfg->reg_ops = &spi_reg_ops;
	return cfg;
}
static int linux_dal_asw_probe(struct platform_device *pdev)
{
    dal_kernel_asw_dev_t* dev = NULL;
    int err = 0;
    int ret = 0;
    unsigned char lchip = 0;
    /*if cant get data from file dts,we should init by array*/
    unsigned int mdio_addr[CTC_MAX_LOCAL_CHIP_NUM] = {3,0,0,0,0,0,0,0};
    unsigned int i2c_addr[CTC_MAX_LOCAL_CHIP_NUM] = {0x3c,0,0,0,0,0,0,0};
    unsigned int spi_addr[CTC_MAX_LOCAL_CHIP_NUM] = {3,1,0,0,0,0,0,0};
    printk(KERN_WARNING "********found dal asw device deviceid:%d*****\n", dal_glb.dal_chip_num);
    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if (NULL == dal_master[lchip])
        {
            break;
        }
    }
    if (lchip >= CTC_MAX_LOCAL_CHIP_NUM)
    {
        printk("Exceed max local chip num\n");
        return -1;
    }

    dal_master[lchip] = kmalloc(sizeof(dal_kernel_master_t), GFP_ATOMIC);
    if (NULL == dal_master[lchip])
    {
        printk("no memory for dal cpu dev, lchip %d\n", lchip);
        return -1;
    }
    memset(dal_master[lchip], 0, sizeof(dal_kernel_master_t));

    if (dal_io_user_mode)
    {
        dal_glb.dal_io_mode = simple_strtoul(dal_io_user_mode, NULL, 0);
        printk("dal io mode: 0x%x \n", dal_glb.dal_io_mode);
    }
    if (NULL == dal_dev[lchip])
    {
        dal_dev[lchip] = kmalloc(sizeof(dal_kernel_asw_dev_t), GFP_ATOMIC);
        if (NULL == dal_dev[lchip])
        {
            printk("no memory for dal soc dev, lchip %d\n", lchip);
            ret = -1;
            goto roll_back_0;
        }
    }
    dev = dal_dev[lchip];
    dal_glb.dal_chip_num += 1;
    dev->p_dev = pdev;
    if (dal_glb.dal_io_mode == DAL_SMI_IO)
    {
        dev->mdio_addr = mdio_addr[lchip];
        dev = dal_smi_probe_of(pdev, dev);
    }
    else if (dal_glb.dal_io_mode == DAL_I2C_IO)
    {
        dev->i2c_addr = i2c_addr[lchip];
        dev = dal_i2c_probe_of(pdev, dev);
    }
    else if (dal_glb.dal_io_mode == DAL_SPI_IO)
    {
        dev->spi_addr = spi_addr[lchip];
        dev = dal_spi_probe_of(pdev, dev);
    }
    else
    {
        printk("%s: invalid dal_io_mode node\n", __func__);
        ret = -1;
        goto roll_back_1;
     }
#if 0
    for (i = 0; i < CTC_MAX_INTR_NUM; i++)
    {
        irq = platform_get_irq(pdev, i);
        if (irq < 0)
        {
            printk( "can't get irq number\n");
            kfree(dev);
            return irq;
        }
        dal_int[i].irq = irq;
        printk( "irq %d vector %d\n", i, irq);
    }
#endif
    dal_master[lchip]->active_type = active_type[lchip] = DAL_CPU_MODE_TYPE_ASW;
    printk(KERN_WARNING "linux_dal_probe end \n");
    return 0;
roll_back_1:
    kfree(dev);
roll_back_0:
    kfree(dal_master[lchip]);
    dal_master[lchip] = NULL;
    return ret;
}
static int
linux_dal_asw_remove(struct platform_device *pdev)
{
    unsigned int lchip = 0;
    unsigned int flag = 0;
    dal_kernel_asw_dev_t* dev = NULL;
    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        dev = dal_dev[lchip];
        if ((NULL != dev )&& (pdev == dev->p_dev))
        {
            flag = 1;
            break;
        }
    }
    if (flag)
    {
        if (dal_glb.dal_io_mode == DAL_SPI_IO)
        {
            spi_unregister_device(dev->spi_device);
        }
        dev->p_dev = NULL;
        kfree(dev);
        dev = NULL;
    }
    CTC_PRINTK("%s: linux_dal_remove end \n",);
    return 0;
}
#endif

#if defined(SOC_ACTIVE)
static int linux_dal_local_probe(struct platform_device *pdev)
{
    dal_kernel_dev_t* dev = NULL;
    unsigned int temp = 0;
    unsigned int lchip = 0;
    unsigned int address = 0x48;
    int ret = -1;
    int i = 0;
    int irq = 0;
    struct resource * res = NULL;
    struct resource * dma_res = NULL;
    pci_cmd_status_u_t cmd_status_u;
    unsigned int cnt = 0;
    unsigned int bar = 0;
    const char* data = NULL;

    printk(KERN_WARNING "********found soc dal device*****\n");

    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if (NULL == dal_master[lchip])
        {
            break;
        }
    }

    if (lchip >= CTC_MAX_LOCAL_CHIP_NUM)
    {
        printk("Exceed max local chip num\n");
        return -1;
    }

    dal_master[lchip] = kmalloc(sizeof(dal_kernel_master_t), GFP_ATOMIC);
    if (NULL == dal_master[lchip])
    {
        printk("no memory for dal soc dev, lchip %d\n", lchip);
        return -1;
    }
    memset(dal_master[lchip], 0, sizeof(dal_kernel_master_t));

    dev = &dal_master[lchip]->dal_dev;

    dal_glb.dal_chip_num += 1;
    dal_master[lchip]->dev_no = DAL_TSINGMA_DEVICE_ID;

    dev->pci_dev = (void*)pdev;

    data = of_device_get_match_data(&pdev->dev);
    if (data && !strcmp(data, "tm_dx_soc"))
    {
        printk("Detect tmdx device!\n");
        address = 0x240;
        bar = 4;   /*for tmdx using bar4 to get axisup base*/        
        dal_master[lchip]->dev_no = DAL_TSINGMA_DX_DEVICE_ID;
    }
    else if (data && !strcmp(data, "tm_tmdl_soc"))
    {
        printk("Detect tmdl device!\n");
        address = 0x240;
        bar = 4;   /*for tmdx using bar4 to get axisup base*/        
        dal_master[lchip]->dev_no = DAL_TSINGMA_DXL_DEVICE_ID;
    }
    
    res = platform_get_resource(pdev, IORESOURCE_MEM, bar);
    dev->phys_address = res->start;
    dev->logic_address = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(dev->logic_address))
    {
        goto error_rollback0;
    }

    for (i = 0; i < DAL_MAX_INTR_NUM; i++)
    {
        irq = platform_get_irq(pdev, i);
        if (irq < 0)
        {
            printk( "can't get irq number\n");
            goto error_rollback0;
        }
        dal_master[lchip]->msi_irq_base[i] = irq;
        printk( "irq %d vector %d\n", i, irq);
    }
    dal_master[lchip]->msi_irq_num = DAL_MAX_INTR_NUM;

    dal_master[lchip]->active_type = DAL_CPU_MODE_TYPE_LOCAL;
    _dal_pci_read(lchip, address, &temp);
    if (((temp >> 8) & 0xffff) == 0x3412)
    {
        _dal_pci_write(lchip, address, 0xFFFFFFFF);
    }
    printk("Little endian Cpu detected!!! \n");

    /* alloc dma_mem_size for every chip */
    if (dal_glb.dma_mem_size)
    {
        dal_alloc_dma_pool(lchip,  dal_glb.dma_mem_size);
        if(!dal_master[lchip]->dma_virt_base)
        {
            goto error_rollback0;
        }
        /*add check Dma memory pool cannot cross 4G space*/
        if ((dal_master[lchip]->dma_phy_base>>32) != ((dal_master[lchip]->dma_phy_base+dal_glb.dma_mem_size-1)>>32))
        {
            printk("Dma malloc memory cross 4G space!!!!!! \n");
            goto error_rollback1;
        }
    }

    cmd_status_u.val = 0;
    cmd_status_u.cmd_status.cmdReadType = 1;
    cmd_status_u.cmd_status.cmdEntryWords = 1;   /* normal operate only support 1 entry */
    cmd_status_u.cmd_status.cmdDataLen = 1;

    _dal_pci_write(lchip, 0, cmd_status_u.val);
    _dal_pci_write(lchip, 4, 0x0000000);

    do{
        _dal_pci_read(lchip, 0, &cmd_status_u.val);
    }while (!(cmd_status_u.cmd_status.reqProcDone) && (++cnt < 0x1FFF));

    if(!cmd_status_u.cmd_status.reqProcDone || cmd_status_u.cmd_status.reqProcError)
    {
        goto error_rollback1;
    }

    _dal_pci_read(lchip, 8, &temp);
    printk("###### DevId value 0x%x\n", temp);

    printk(KERN_WARNING "linux_dal_probe end*****\n");

    return 0;
error_rollback1:
    dal_free_dma_pool(lchip);
error_rollback0:
    kfree(dal_master[lchip]);
    dal_master[lchip] = NULL;
    return ret;
}
#endif

int linux_dal_pcie_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    dal_kernel_dev_t* dev = NULL;
    unsigned int temp = 0;
    unsigned int lchip = 0;
    unsigned int address = 0;
    int bar = 0;
    int ret = 0;
    int endian_mode = 0;
    pci_cmd_status_u_t cmd_status_u;
    unsigned int cnt = 0x0;

    printk(KERN_WARNING "********found cpu dal device*****\n");

    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if (NULL == dal_master[lchip])
        {
            break;
        }
    }

    if (lchip >= CTC_MAX_LOCAL_CHIP_NUM)
    {
        printk("Exceed max local chip num\n");
        return -1;
    }

    dal_master[lchip] = kmalloc(sizeof(dal_kernel_master_t), GFP_ATOMIC);
    if (NULL == dal_master[lchip])
    {
        printk("no memory for dal cpu dev, lchip %d\n", lchip);
        return -1;
    }
    memset(dal_master[lchip], 0, sizeof(dal_kernel_master_t));

    dev = &dal_master[lchip]->dal_dev;
    dal_glb.dal_chip_num += 1;

    dev->pci_dev =(void*) pdev;

    if ((pdev->device == DAL_TSINGMA_DEVICE_ID))
    {
        printk("use bar2 to config memory space\n");
        bar = 2;
    }

    if (pci_enable_device(pdev) < 0)
    {
        printk("Cannot enable PCI device: vendor id = %x, device id = %x\n",
               pdev->vendor, pdev->device);
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
#if defined(DMA_MEM_MODE_PLATFORM) || defined(SOC_ACTIVE)
    ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
    if (ret)
    {
        ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
        if (ret)
        {
            printk("Could not set PCI DMA Mask\n");
            goto error_rollback0;
        }
    }
#else
    ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
    if (ret)
    {
        ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
        if (ret)
        {
            printk("Could not set PCI DMA Mask\n");
            goto error_rollback0;
        }
    }
#endif
#else
    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
    if (ret)
    {
        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
        if (ret)
        {
            printk("Could not set PCI DMA Mask\n");
            goto error_rollback0;
        }
    }
#endif

    if (pci_request_regions(pdev, DAL_NAME) < 0)
    {
        printk("Cannot obtain PCI resources\n");
    }

    dev->phys_address = pci_resource_start(pdev, bar);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0))
    dev->logic_address = ioremap(dev->phys_address,
                                                pci_resource_len(pdev, bar));
#else
    dev->logic_address = ioremap_nocache(dev->phys_address,
                                                pci_resource_len(pdev, bar));
#endif
    /*0: little endian 1: big endian*/
    endian_mode = ((DAL_GREATBELT_DEVICE_ID == pdev->device) || (DAL_GOLDENGATE_DEVICE_ID == pdev->device) || 
                    (DAL_GOLDENGATE_DEVICE_ID1 == pdev->device) || (DAL_DUET2_DEVICE_ID == pdev->device))?1:0;

    dal_master[lchip]->active_type = DAL_CPU_MODE_TYPE_PCIE;
    
    address = ((DAL_GREATBELT_DEVICE_ID == pdev->device) || (DAL_GOLDENGATE_DEVICE_ID == pdev->device) || (DAL_GOLDENGATE_DEVICE_ID1 == pdev->device)
        || (DAL_DUET2_DEVICE_ID == pdev->device) || (DAL_TSINGMA_DEVICE_ID == pdev->device) || (DAL_TSINGMA_MX_DEVICE_ID == pdev->device)
        || (DAL_ARCTIC_DEVICE_ID == pdev->device) || (DAL_TSINGMA_GX_DEVICE_ID == pdev->device))? 0x48: 0x240;
    _dal_pci_read(lchip, address, &temp);
    if (((temp >> 8) & 0xffff) == 0x3412)
    {
        endian_mode = ((DAL_TSINGMA_DEVICE_ID == pdev->device) || (DAL_TSINGMA_DX_DEVICE_ID == pdev->device) || (DAL_TSINGMA_MX_DEVICE_ID == pdev->device)
                        || (DAL_ARCTIC_DEVICE_ID == pdev->device) || (DAL_ARCTIC_M3_DEVICE_ID == pdev->device) || (DAL_TSINGMA_GX_DEVICE_ID == pdev->device
                        || (DAL_PACIFIC_M12_DEVICE_ID == pdev->device)))?1:0;
        _dal_pci_write(lchip, address, 0xFFFFFFFF);
    }

    if (endian_mode)
    {
        printk("Big endian Cpu detected!!! \n");
    }
    else
    {
        printk("Little endian Cpu detected!!! \n");
    }

    pci_set_master(pdev);

    /* alloc dma_mem_size for every chip */
    if (dal_glb.dma_mem_size)
    {
        dal_alloc_dma_pool(lchip,  dal_glb.dma_mem_size);
        if(!dal_master[lchip]->dma_virt_base)
        {
            ret = -1;
            goto error_rollback0;
        }

        /*add check Dma memory pool cannot cross 4G space*/
        if ((dal_master[lchip]->dma_phy_base>>32) != ((dal_master[lchip]->dma_phy_base+dal_glb.dma_mem_size-1)>>32))
        {
            printk("Dma malloc memory cross 4G space!!!!!! \n");
            ret = -1;
            goto error_rollback1;
        }
    }

if ((DAL_GOLDENGATE_DEVICE_ID != pdev->device) && ((DAL_GOLDENGATE_DEVICE_ID1) != pdev->device))
{
    cmd_status_u.val = 0;
    cmd_status_u.cmd_status.cmdReadType = 1;
    cmd_status_u.cmd_status.cmdEntryWords = 1;   /* normal operate only support 1 entry */
    cmd_status_u.cmd_status.cmdDataLen = 1;

    _dal_pci_write(lchip, 0, cmd_status_u.val);
    _dal_pci_write(lchip, 4, 0x0000000);

    do{
        _dal_pci_read(lchip, 0, &cmd_status_u.val);
    }while (!(cmd_status_u.cmd_status.reqProcDone) && (++cnt < 0x1FFF));

    if(!cmd_status_u.cmd_status.reqProcDone || cmd_status_u.cmd_status.reqProcError)
    {
        ret = -1;
        goto error_rollback1;
    }

    _dal_pci_read(lchip, 8, &temp);
    printk("###### DevId value 0x%x\n", temp);
}
    printk(KERN_WARNING "linux_dal_probe end*****\n");

    return 0;
error_rollback1:
    dal_free_dma_pool(lchip);
error_rollback0:
    kfree(dal_master[lchip]);
    dal_master[lchip] = NULL;
    return ret;
}

#if defined(SOC_ACTIVE)
static int
linux_dal_local_remove(struct platform_device *pdev)
{
    unsigned int lchip = 0;
    unsigned int flag = 0;

    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if ((NULL != dal_master[lchip] )&& (pdev == dal_master[lchip]->dal_dev.pci_dev))
        {
            flag = 1;
            break;
        }
    }

    if (1 == flag)
    {
        dal_free_dma_pool(lchip);
        dal_glb.dal_chip_num--;
        dal_master[lchip]->active_type = DAL_CPU_MODE_TYPE_NONE;
        kfree(dal_master[lchip]);
        dal_master[lchip] = NULL;
    }

    return 0;
}
#endif

void
linux_dal_pcie_remove(struct pci_dev* pdev)
{
    unsigned char lchip = 0;
    unsigned int flag = 0;
    unsigned char index = 0;

    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if ((NULL != dal_master[lchip] )&& (pdev == dal_master[lchip]->dal_dev.pci_dev))
        {
            flag = 1;
            break;
        }
    }

    printk(KERN_WARNING "********dal remove device, lchip:%u find_flag:%u*****\n", lchip, flag);

    if (1 == flag)
    {
        if(dal_master[lchip]->msi_used)
        {
            for (index = 0; index < dal_master[lchip]->msi_irq_num; index++)
            {
                dal_interrupt_unregister(dal_master[lchip]->msi_irq_base[index]);
            }
            _dal_set_msi_disable(lchip, (1 == dal_master[lchip]->msi_used)?DAL_MSI_TYPE_MSI:DAL_MSI_TYPE_MSIX);
        }
        printk(KERN_WARNING "********dal free pci resource*****\n");
        dal_free_dma_pool(lchip);
        #ifdef CONFIG_PCI
        pci_clear_master(pdev);
        #endif
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        dal_glb.dal_chip_num--;
        dal_master[lchip]->active_type = DAL_CPU_MODE_TYPE_NONE;

        kfree(dal_master[lchip]);
        dal_master[lchip] = NULL;
    }
}

#ifdef CONFIG_COMPAT
static long
linux_dal_ioctl(struct file* file,
                unsigned int cmd, unsigned long arg)
#else

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
static long
linux_dal_ioctl(struct file* file,
                unsigned int cmd, unsigned long arg)
#else
static int
linux_dal_ioctl(struct inode* inode, struct file* file,
                unsigned int cmd, unsigned long arg)
#endif
#endif
{
    switch (cmd)
    {

    case CMD_READ_CHIP:
        return dal_pci_read(arg);

    case CMD_WRITE_CHIP:
        return dal_pci_write(arg);

    case CMD_GET_DEVICES:
        return linux_get_device(arg, 0);

    case CMD_GET_DAL_VERSION:
        return linux_get_dal_version(arg);

    case CMD_GET_DMA_INFO:
        return linux_get_dma_info(arg);

    case CMD_PCI_CONFIG_READ:
        return dal_user_read_pci_conf(arg);

    case CMD_PCI_CONFIG_WRITE:
        return dal_user_write_pci_conf(arg);

    case CMD_REG_INTERRUPTS:
        return dal_user_interrupt_register(arg);

    case CMD_UNREG_INTERRUPTS:
        return dal_user_interrupt_unregister(arg);

    case CMD_EN_INTERRUPTS:
        return dal_user_interrupt_set_en(arg);

    case CMD_SET_MSI_CAP:
        return dal_set_msi_cap(arg);

    case CMD_GET_MSI_INFO:
        return dal_get_msi_info(arg);

    case CMD_IRQ_MAPPING:
        return dal_create_irq_mapping(arg);

    case CMD_GET_INTR_INFO:
        return dal_get_intr_info(arg);

    case CMD_CACHE_INVAL:
        return dal_user_cache_inval(arg);

    case CMD_CACHE_FLUSH:
        return dal_user_cache_flush(arg);
    case CMD_GET_DAL_MMAP_MODE:
        {
#ifdef CONFIG_STRICT_DEVMEM
            int used_private_mmap = 1;    /* use private mmap via /dev/linux_sal, refer to linux_dal_mmap */
#else
            int used_private_mmap = 0;    /* use mmap via /dev/mem */
#endif

            if (copy_to_user((int*)arg, (void*)&used_private_mmap, sizeof(used_private_mmap)))
            {
                return -EFAULT;
            }
        }
        break;

    default:
        break;
    }

    return 0;
}

#ifdef CONFIG_COMPAT
static long
linux_dal_ioctl_compat(struct file* file,
                unsigned int cmd, unsigned long arg)
{
    return (cmd == CMD_GET_DEVICES) ? linux_get_device(arg, 1) : linux_dal_ioctl(file, cmd, arg);
}
#endif

#if defined(__arm__)
#define _PGPROT_NONCACHED(x) x = pgprot_noncached((x))
#elif defined(__aarch64__ )
#define _PGPROT_NONCACHED(x) x = pgprot_writecombine((x))
#else
#define _PGPROT_NONCACHED(x) x = pgprot_noncached((x))
#endif
/*
 * Some kernels are configured to prevent mapping of kernel RAM memory
 * into user space via the /dev/mem device.
 *
 * The function below provides a backdoor to mapping the DMA pool to
 * user space via /dev/linux_dal.
 */
int linux_dal_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long phys_addr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;

#if defined DMA_MEM_MODE_PLATFORM && (defined(__arm__) || defined(__aarch64__ ))
    printk("Use linux_dal_mmap to mapping DMA pool address to user space, virtal_base:0x%lx, phy_base:0x%lx, size:%lu\n", vma->vm_start, phys_addr, size);

    vma->vm_pgoff = 0;
    return dma_mmap_coherent(_dma_alloc_coherent_dev, vma, _dma_alloc_coherent_virt_base, phys_addr, size);
#endif

    _PGPROT_NONCACHED(vma->vm_page_prot);

    if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot)) {
        printk("Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
                phys_addr, phys_addr + size, vma->vm_start,vma->vm_end);
        return -EAGAIN;
    }
    printk("Use linux_dal_mmap to mapping DMA pool address to user space, virtal_base:0x%lx, phy_base:0x%lx\n", vma->vm_start, phys_addr);
    return 0;
}

static struct pci_driver linux_dal_pcie_driver =
{
    .name = DAL_NAME,
    .id_table = dal_id_table,
    .probe = linux_dal_pcie_probe,
    .remove = linux_dal_pcie_remove,
};
#if defined(SOC_ACTIVE)
static struct platform_driver linux_dal_local_driver =
{
    .probe = linux_dal_local_probe,
    .remove = linux_dal_local_remove,
    .driver = {
        .name = DAL_NAME,
        .of_match_table = of_match_ptr(linux_dal_of_match),
    },
};
#endif
#if defined(ASW_ACTIVE)
static struct platform_driver linux_dal_asw_driver =
{
    .driver = {
        .name       = DAL_NAME,
        .owner      = THIS_MODULE,
        .of_match_table = of_match_ptr(linux_dal_asw_of_match),
    },
    .probe      = linux_dal_asw_probe,
    .remove     = linux_dal_asw_remove,
};
#endif

static struct file_operations fops =
{
    .owner = THIS_MODULE,
#ifdef CONFIG_COMPAT
    .compat_ioctl = linux_dal_ioctl_compat,
    .unlocked_ioctl = linux_dal_ioctl,
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36))
    .unlocked_ioctl = linux_dal_ioctl,
#else
    .ioctl = linux_dal_ioctl,
#endif
#endif
    .mmap = linux_dal_mmap,
};

static int __init
linux_dal_init(void)
{
    int ret = 0;

    memset(&dal_glb, 0, sizeof(dal_glb));
#ifdef ASW_ACTIVE
	dal_glb.dal_io_mode = DAL_SMI_IO;
#endif
    dal_glb.dma_mem_size = 0x2000000;
    /* Get DMA memory pool size form dal.ok input param, or use default dma_mem_size */
    if (dma_pool_size)
    {
        if ((dma_pool_size[strlen(dma_pool_size) - 1] & ~0x20) == 'M')
        {
            dal_glb.dma_mem_size = simple_strtoul(dma_pool_size, NULL, 0);
            printk("dma_mem_size: 0x%x \n", dal_glb.dma_mem_size);

            dal_glb.dma_mem_size *= DAL_MB_SIZE;
        }
        else
        {
            printk("DMA memory pool size must be specified as e.g. dma_pool_size=8M\n");
        }

        if (dal_glb.dma_mem_size & (dal_glb.dma_mem_size - 1))
        {
            printk("dma_mem_size must be a power of 2 (1M, 2M, 4M, 8M etc.)\n");
            dal_glb.dma_mem_size = 0;
        }
    }

    ret = register_chrdev(DAL_DEV_MAJOR, DAL_NAME, &fops);
    if (ret < 0)
    {
        printk(KERN_WARNING "Register linux_dal device, ret %d\n", ret);
        return ret;
    }

    ret = pci_register_driver(&linux_dal_pcie_driver);
    if (ret < 0)
    {
        printk(KERN_WARNING "Register ASIC PCI driver failed, ret %d\n", ret);
        goto error_rollback0;
    }
#if defined(SOC_ACTIVE)
    ret = platform_driver_register(&linux_dal_local_driver);
    if (ret < 0)
    {
        printk(KERN_WARNING "Register ASIC LOCALBUS driver failed, ret %d\n", ret);
        goto error_rollback1;
    }
#endif
#if defined(ASW_ACTIVE)
    ret = platform_driver_register(&linux_dal_asw_driver);
    if (ret < 0)
    {
        printk(KERN_WARNING "Register ASIC LOCALBUS driver failed, ret %d\n", ret);
        goto error_rollback1;
    }
#endif

    /* alloc /dev/linux_dal node */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
    dal_glb.dal_class = class_create(DAL_NAME);
#else
    dal_glb.dal_class = class_create(THIS_MODULE, DAL_NAME);
#endif
    device_create(dal_glb.dal_class, NULL, MKDEV(DAL_DEV_MAJOR, 0), NULL, DAL_NAME);

    /* init interrupt function */
    dal_glb.dal_isr[0].intr_handler_fun = intr0_handler;
    dal_glb.dal_isr[1].intr_handler_fun = intr1_handler;
    dal_glb.dal_isr[2].intr_handler_fun = intr2_handler;
    dal_glb.dal_isr[3].intr_handler_fun = intr3_handler;
    dal_glb.dal_isr[4].intr_handler_fun = intr4_handler;
    dal_glb.dal_isr[5].intr_handler_fun = intr5_handler;
    dal_glb.dal_isr[6].intr_handler_fun = intr6_handler;
    dal_glb.dal_isr[7].intr_handler_fun = intr7_handler;

    return ret;
#if defined(SOC_ACTIVE) || defined(ASW_ACTIVE)
error_rollback1:
    pci_unregister_driver(&linux_dal_pcie_driver);
#endif
error_rollback0:
    unregister_chrdev(DAL_DEV_MAJOR, "linux_dal");
    return ret;
}

static void __exit
linux_dal_exit(void)
{
    unsigned char str[16];
    int intr_idx = 0;
    unsigned char lchip = 0;

    device_destroy(dal_glb.dal_class, MKDEV(DAL_DEV_MAJOR, 0));
    class_destroy(dal_glb.dal_class);
    unregister_chrdev(DAL_DEV_MAJOR, "linux_dal");
    pci_unregister_driver(&linux_dal_pcie_driver);
#if defined(SOC_ACTIVE)
    platform_driver_unregister(&linux_dal_local_driver);
#endif
#if defined(ASW_ACTIVE)
    platform_driver_unregister(&linux_dal_asw_driver);
#endif
    for (intr_idx = 0; intr_idx < DAL_MAX_INTR_NUM; intr_idx++)
    {    
        if (0 != dal_glb.dal_isr[intr_idx].count)
        {/* irq not free in dal_interrupt_unregister, free it here */
            snprintf(str, 16, "%s%d", "dal_intr", intr_idx);
            unregister_chrdev(DAL_DEV_INTR_MAJOR_BASE + intr_idx, str);
            free_irq(dal_glb.dal_isr[intr_idx].irq, &dal_glb.dal_isr[intr_idx]);

            dal_glb.dal_isr[intr_idx].irq = 0;
        }
    }
    dal_glb.dal_intr_num = 0;
    for (lchip = 0; lchip < CTC_MAX_LOCAL_CHIP_NUM; lchip ++)
    {
        if (NULL == dal_master[lchip])
        {
            continue;
        }

        if(dal_master[lchip]->msi_used)
        {
            _dal_set_msi_disable(lchip, (1 == dal_master[lchip]->msi_used)?DAL_MSI_TYPE_MSI:DAL_MSI_TYPE_MSIX);
        }
        dal_free_dma_pool(lchip);
        dal_glb.dal_chip_num--;
        dal_master[lchip]->active_type = DAL_CPU_MODE_TYPE_NONE;
        kfree(dal_master[lchip]);
        dal_master[lchip] = NULL;
    }

}

module_init(linux_dal_init);
module_exit(linux_dal_exit);
EXPORT_SYMBOL(dal_get_dal_ops);
EXPORT_SYMBOL(dal_cache_inval);
EXPORT_SYMBOL(dal_cache_flush);
