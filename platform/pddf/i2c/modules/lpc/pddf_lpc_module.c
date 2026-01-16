#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/iomap.h>


#define DRIVER_NAME "lpc_cpld"
#define CPLD_REG_MAX_IDX   0x13
#define CPLD_LPC_BASE  0x900
#define CPLD_LPC_SIZE 0x100

#ifdef CONFIG_X86
#define LPC_PCI_VENDOR_ID_INTEL  0x8086
#define LPC_PCI_DEVICE_ID_INTEL 0x8c54
#define LPC_PCI_VENDOR_ID_ZX  0x1d17
#define LPC_PCI_DEVICE_ID_ZX 0x1001
#endif
#ifdef CONFIG_ARCH_PHYTIUM
#define PHYTIUM_LPC_BASE 0x20000000
#define PHYTIUM_LPC_CONFIG_BASE 0x27FFFF00
#define PHYTIUM_LPC_CONFIG_SIZE 0x100
#define INT_APB_SPCE_CONF 0xFC
#define CLK_LPC_RSTN_O    0xE4
#define NU_SERIRQ_CONFIG  0xE8
#define INT_MASK          0xD8
#define START_CYCLE       0xD4

#define GPIO_MUX_PAD_BASE      0x28180200
#define GPIO_MUX_PAD_SIZE   16
#define GPIO_MUX_PAD_1_ADDR 4
#define GPIO_MUX_PAD_1_VALUE 0x84888441
#define GPIO_MUX_PAD_2_ADDR 8
#define GPIO_MUX_PAD_2_VALUE 0x18800048

#define ACCESS_TYPE_IO 0
#define ACCESS_TYPE_MEM 1
#define ACCESS_TYPE_FIRMWARE  2
#define ACCESS_TYPE_DMA 3

void __iomem *g_cpld_lpc_base;
struct lpc_pdata {
   void __iomem *lpc_base;
   resource_size_t lpc_base_phy;
   size_t lpc_size ;
};
#endif

static struct mutex  lpc_lock;
static int dbg_enable = 0;
#ifdef CONFIG_X86
unsigned char lpc_cpld_read_reg(u16 address)
{
    unsigned char reg_val;

    mutex_lock(&lpc_lock);
    
    reg_val = inb(CPLD_LPC_BASE + (address & 0xff));
    if(dbg_enable) {
        printk(KERN_INFO "cpld_base:0x%x, address:0x%x, value:0x%02x\n", 
        CPLD_LPC_BASE, address, reg_val);
    }
    
    mutex_unlock(&lpc_lock);

    return reg_val;
}

void lpc_cpld_write_reg(u16 address, u8 reg_val)
{
    mutex_lock(&lpc_lock);
    
    outb((reg_val & 0xff), CPLD_LPC_BASE + (address & 0xff));
    if(dbg_enable) {
        printk(KERN_INFO "cpld_base:0x%x, address:0x%x, value:0x%02x\r\n", 
        CPLD_LPC_BASE, address, reg_val);
    }
    mutex_unlock(&lpc_lock);   
	 
    return;
}
#endif
#ifdef CONFIG_ARCH_PHYTIUM
unsigned char lpc_cpld_read_reg(u32 address)
{
    unsigned char reg_val;

    mutex_lock(&lpc_lock);
	
    reg_val =  readb(g_cpld_lpc_base + address);
    
    //printk(KERN_INFO "cpld_base:0x%p, address:0x%x, value:0x%02x\n", 
    //    g_cpld_lpc_base, address, reg_val);

    mutex_unlock(&lpc_lock); 
	
    return reg_val;
}

void lpc_cpld_write_reg(u32 address, u8 reg_val)
{
    mutex_lock(&lpc_lock);

    writeb(reg_val, g_cpld_lpc_base + address);
    
    //printk(KERN_INFO "cpld_base:0x%p, address:0x%x, value:0x%02x\r\n", 
     //   g_cpld_lpc_base, address, reg_val);

    mutex_unlock(&lpc_lock);   
	  
    return;
}
#endif
EXPORT_SYMBOL(lpc_cpld_read_reg);
EXPORT_SYMBOL(lpc_cpld_write_reg);

#ifdef CONFIG_X86
static int __init pddf_lpc_cpld_init(void)
{
    struct pci_dev *pdev = NULL;
    uint32_t status = 0;
    printk("pddf_lpc_cpld_init\n");

    pdev = pci_get_device(LPC_PCI_VENDOR_ID_INTEL, LPC_PCI_DEVICE_ID_INTEL, pdev);
    if (pdev) {
        /* 
     * LPC I/F Generic Decode Range 4 Register for cpld1 0x0900-0x09FF
     */
        status = pci_write_config_dword(pdev, 0x90, 0xfc0901);
        if(status)
        { 
            printk(KERN_ERR "pci_write_config_word error %d!\n",status);
            return status;
        } 
    } else {
        pdev = pci_get_device(LPC_PCI_VENDOR_ID_ZX, LPC_PCI_DEVICE_ID_ZX, pdev);
        if (pdev) {
            status = pci_write_config_word(pdev, 0x5C, 0x900);
            if(status)
            { 
                printk(KERN_ERR "pci_write_config_word error %d!\n",status);
                return status;
            }  
            status = pci_write_config_byte(pdev, 0x66, 0x01);
            if(status)
            { 
                printk(KERN_ERR "pci_write_config_byte error %d!\n",status);
                return status;
            }  
        } else {
            printk(KERN_INFO "there is no LPC controller\n");
            return 0;
        }

    }
    if (!request_region(CPLD_LPC_BASE, CPLD_LPC_SIZE, "lpc_sys_cpld")) {
        printk("request_region 0x%x failed!\n", CPLD_LPC_BASE);
		return -EBUSY;
	}
    return 0; 
}

static void __exit pddf_lpc_cpld_exit(void)
{
    release_region(CPLD_LPC_BASE, CPLD_LPC_SIZE);
}
#else
static int cpld_lpc_drv_probe(struct platform_device *pdev)
{
    uint32_t status = 0;
    struct resource *res;
    void __iomem *pad_base,*lpc_config_base;
    struct lpc_pdata *pdata = devm_kzalloc(&pdev->dev, sizeof(struct lpc_pdata),
                                            GFP_KERNEL);
    printk("clounix_lpc_probe\n");

    /* enable LPC*/
    pad_base = ioremap(GPIO_MUX_PAD_BASE, GPIO_MUX_PAD_SIZE);
    if (!pad_base) {
		printk(KERN_ERR  "@0x%x: Unable to map LPC PAD registers\n", GPIO_MUX_PAD_BASE);
		return -ENOMEM;
	}
    writel(GPIO_MUX_PAD_1_VALUE, pad_base + GPIO_MUX_PAD_1_ADDR);
    writel(GPIO_MUX_PAD_2_VALUE, pad_base + GPIO_MUX_PAD_2_ADDR);
    iounmap(pad_base);
    printk(KERN_INFO "Enable LPC PAD registers successful\n");
    lpc_config_base = ioremap(PHYTIUM_LPC_CONFIG_BASE, PHYTIUM_LPC_CONFIG_SIZE);
    if (!lpc_config_base) {
		printk(KERN_ERR  "@0x%x: Unable to map LPC PAD registers\n", PHYTIUM_LPC_CONFIG_BASE);
		return -ENOMEM;
	}
        /*select memory access*/
    writel(1, lpc_config_base + CLK_LPC_RSTN_O);
    writel(ACCESS_TYPE_MEM, lpc_config_base + INT_APB_SPCE_CONF);
    writel(0, lpc_config_base + INT_MASK);
    writel(0x15, lpc_config_base + START_CYCLE);

    iounmap(lpc_config_base);
    printk(KERN_INFO "Enable LPC config registers successful\n");
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (unlikely(!res)) {
        printk(KERN_ERR "Specified Resource Not Available...\n");
        return -ENODEV;
    }
    pdata->lpc_base_phy = res->start;
    pdata->lpc_size     = resource_size(res);
    pdata->lpc_base = devm_ioremap_resource(&pdev->dev, res);
	if (!pdata->lpc_base)
	{
        printk(KERN_ERR  "@%p: Unable to map LPC  registers\n", res->start);
        return -ENOMEM;
    }
    g_cpld_lpc_base = pdata->lpc_base;
    printk(KERN_INFO "@%p:  map LPC  registers succsess\n", pdata->lpc_base);
    platform_set_drvdata(pdev, pdata);

    return 0;
}
static int cpld_lpc_drv_remove(struct platform_device *pdev)
{
    struct lpc_pdata *pdata = platform_get_drvdata(pdev);

    printk("clounix_cpld_lpc_remove\n");
    if(pdata)
    {
        kfree(pdata);
    }
    return 0;
}

static void cpld_lpc_dev_release( struct device * dev)
{
    return;
}
static struct resource cpld_lpc_resources[] = {
    {
        .start  = PHYTIUM_LPC_BASE + CPLD_LPC_BASE,
        .end    = PHYTIUM_LPC_BASE + CPLD_LPC_BASE + CPLD_LPC_SIZE,
        .flags  = IORESOURCE_MEM,
    },
};

static struct platform_device cpld_lpc_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cpld_lpc_resources),
    .resource       = cpld_lpc_resources,
    .dev = {
        .release = cpld_lpc_dev_release,
    }
};

static struct platform_driver cpld_lpc_drv = {
    .probe  = cpld_lpc_drv_probe,
    .remove = cpld_lpc_drv_remove,
    .driver = {
        .name   = DRIVER_NAME,
    },
};

static int __init pddf_lpc_cpld_init(void)
{
    // Register platform device and platform driver
    platform_device_register(&cpld_lpc_dev);
    platform_driver_register(&cpld_lpc_drv);
    return 0;
}

static void __exit pddf_lpc_cpld_exit(void)
{
    // Unregister platform device and platform driver
    platform_driver_unregister(&cpld_lpc_drv);
    platform_device_unregister(&cpld_lpc_dev);
}
#endif
module_param(dbg_enable, uint, S_IRUGO|S_IWUSR);
module_init(pddf_lpc_cpld_init);
module_exit(pddf_lpc_cpld_exit);
MODULE_AUTHOR("Songqh <songqh@clounix.com>");
MODULE_DESCRIPTION("clounix_lpc_cpld driver");
MODULE_LICENSE("GPL");
