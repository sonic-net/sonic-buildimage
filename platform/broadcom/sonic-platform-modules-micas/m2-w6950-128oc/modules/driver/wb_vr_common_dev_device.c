#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_vr_common.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define WB_VR_DEVICE_NAME(_bus, _addr) vr_device_data##_bus##_##_addr

#define WB_VR_DEVICE_DEFINE(_bus, _addr) \
    wb_vr_common_device_t WB_VR_DEVICE_NAME(_bus, _addr) = \
        { .i2c_bus = (_bus), .i2c_addr = (_addr), .device_flag = 0 }

static WB_VR_DEVICE_DEFINE(54 , 0x72);
static WB_VR_DEVICE_DEFINE(54 , 0x74);
static WB_VR_DEVICE_DEFINE(94 , 0x60);
static WB_VR_DEVICE_DEFINE(95 , 0x60);
static WB_VR_DEVICE_DEFINE(96 , 0x60);
static WB_VR_DEVICE_DEFINE(97 , 0x60);
static WB_VR_DEVICE_DEFINE(102, 0x60);
static WB_VR_DEVICE_DEFINE(103, 0x60);
static WB_VR_DEVICE_DEFINE(104, 0x60);
static WB_VR_DEVICE_DEFINE(105, 0x60);
static WB_VR_DEVICE_DEFINE(107, 0x60);
static WB_VR_DEVICE_DEFINE(108, 0x60);
static WB_VR_DEVICE_DEFINE(109, 0x60);
static WB_VR_DEVICE_DEFINE(110, 0x60);
static WB_VR_DEVICE_DEFINE(111, 0x60);
static WB_VR_DEVICE_DEFINE(112, 0x60);
static WB_VR_DEVICE_DEFINE(113, 0x60);
static WB_VR_DEVICE_DEFINE(114, 0x60);
static WB_VR_DEVICE_DEFINE(115, 0x60);
static WB_VR_DEVICE_DEFINE(116, 0x60);
static WB_VR_DEVICE_DEFINE(117, 0x60);
static WB_VR_DEVICE_DEFINE(118, 0x60);
static WB_VR_DEVICE_DEFINE(119, 0x60);
static WB_VR_DEVICE_DEFINE(120, 0x60);
static WB_VR_DEVICE_DEFINE(121, 0x60);
static WB_VR_DEVICE_DEFINE(122, 0x60);
static WB_VR_DEVICE_DEFINE(136, 0x60);

static void wb_io_dev_device_release(struct device *dev)
{
    return;
}

#define WB_VR_DEV_ENTRY(_id, _bus, _addr) \
    { \
        .name = "wb-vr-common", \
        .id = (_id), \
        .dev = { \
            .platform_data = &WB_VR_DEVICE_NAME(_bus, _addr), \
            .release = wb_io_dev_device_release, \
        }, \
    }

static struct platform_device vr_dev_devices[] = {
    WB_VR_DEV_ENTRY(1, 54, 0x72),
    WB_VR_DEV_ENTRY(2, 54, 0x74),
    WB_VR_DEV_ENTRY(3, 94, 0x60),
    WB_VR_DEV_ENTRY(4, 95, 0x60),
    WB_VR_DEV_ENTRY(5, 96, 0x60),
    WB_VR_DEV_ENTRY(6, 97, 0x60),
    WB_VR_DEV_ENTRY(7, 102, 0x60),
    WB_VR_DEV_ENTRY(8, 103, 0x60),
    WB_VR_DEV_ENTRY(9, 104, 0x60),
    WB_VR_DEV_ENTRY(10, 105, 0x60),
    WB_VR_DEV_ENTRY(11, 107, 0x60),
    WB_VR_DEV_ENTRY(12, 108, 0x60),
    WB_VR_DEV_ENTRY(13, 109, 0x60),
    WB_VR_DEV_ENTRY(14, 110, 0x60),
    WB_VR_DEV_ENTRY(15, 111, 0x60),
    WB_VR_DEV_ENTRY(16, 112, 0x60),
    WB_VR_DEV_ENTRY(17, 113, 0x60),
    WB_VR_DEV_ENTRY(18, 114, 0x60),
    WB_VR_DEV_ENTRY(19, 115, 0x60),
    WB_VR_DEV_ENTRY(20, 116, 0x60),
    WB_VR_DEV_ENTRY(21, 117, 0x60),
    WB_VR_DEV_ENTRY(22, 118, 0x60),
    WB_VR_DEV_ENTRY(23, 119, 0x60),
    WB_VR_DEV_ENTRY(24, 120, 0x60),
    WB_VR_DEV_ENTRY(25, 121, 0x60),
    WB_VR_DEV_ENTRY(26, 122, 0x60),
    WB_VR_DEV_ENTRY(27, 136, 0x60),
};

static int __init wb_vr_dev_device_init(void)
{
    int i;
    int ret = 0;
    wb_vr_common_device_t *vr_dev_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(vr_dev_devices); i++) {
        vr_dev_data = vr_dev_devices[i].dev.platform_data;
        ret = platform_device_register(&vr_dev_devices[i]);
        if (ret < 0) {
            vr_dev_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-vr-dev.%d register failed!\n", i + 1);
        } else {
            vr_dev_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_vr_dev_device_exit(void)
{
    int i;
    wb_vr_common_device_t *vr_dev_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(vr_dev_devices) - 1; i >= 0; i--) {
        vr_dev_data = vr_dev_devices[i].dev.platform_data;
        if (vr_dev_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&vr_dev_devices[i]);
        }
    }
}

module_init(wb_vr_dev_device_init);
module_exit(wb_vr_dev_device_exit);
MODULE_DESCRIPTION("VR DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
