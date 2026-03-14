#include <linux/module.h>
#include <linux/acpi.h>

#define BMS_GPE_CLASS "bms_acpi"
#define BMS_GPE_DRIVER_NAME "bms_gpe"
#define BMS_GPE_DEVICE_NAME "bms_acpi_gpe"

static int bms_gpe[] = {0x01, 0x02, 0x47};
static int bms_gpe_num = sizeof(bms_gpe) / sizeof(bms_gpe[0]);

static u32 bms_gpe_handler(acpi_handle handle, u32 gpe_num, void *context)
{
    struct acpi_device *device = context;

    acpi_bus_generate_netlink_event(device->pnp.device_class,dev_name(&device->dev),
                                    gpe_num, 0);
    return ACPI_INTERRUPT_HANDLED; /* GPE will be disable afterward */
}

static int bms_gpe_add(struct acpi_device *device)
{
    acpi_status status;
    int i = 0;
    char info_str[60] = { 0 };
    char temp[6] = { 0 };

    if (!device) {
        printk("No device of BMS GPE\n");
        return -EINVAL;
    }

    strcpy(acpi_device_name(device), BMS_GPE_DEVICE_NAME);
    strcpy(acpi_device_class(device), BMS_GPE_CLASS);

    strncat(info_str, "Initialized GPE list = ", 23);
    for (i = 0; i < bms_gpe_num; i++) {
        status = acpi_install_gpe_handler(NULL, bms_gpe[i],
                                          ACPI_GPE_LEVEL_TRIGGERED,
                                          &bms_gpe_handler, device);
        if (status != AE_OK) {
            printk("Fail to claim BMS GPE%X (code:0x%X)\n",bms_gpe[i],status);
            return -EINVAL;
        }
        snprintf(temp, sizeof(temp), "0x%.2X ", bms_gpe[i]);
        strncat(info_str, temp, 6);
    }

    dev_info(&device->dev, "%s.\n", info_str);

    return 0;
}

static int bms_gpe_remove(struct acpi_device *device)
{
    int i = 0;
    for (i = 0; i < bms_gpe_num; i++) {
        acpi_remove_gpe_handler(NULL, bms_gpe[i], &bms_gpe_handler);
    }
    return 0;
}

static const struct acpi_device_id bms_acpi_device_ids[] = {
    { "PNP0C01", 0 },
    { /* END OF LIST */ }
};

static struct acpi_driver bms_gpe_driver = {
    .name = BMS_GPE_DRIVER_NAME,
    .class = BMS_GPE_CLASS,
    .ids = bms_acpi_device_ids,
    .ops = {
        .add = bms_gpe_add,
        .remove = bms_gpe_remove,
    },
};

static int __init bms_gpe_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
    return acpi_bus_register_driver(&bms_gpe_driver);
}

static void __exit bms_gpe_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    acpi_bus_unregister_driver(&bms_gpe_driver);
}

module_init(bms_gpe_init);
module_exit(bms_gpe_exit);
MODULE_AUTHOR("Yencheng Lin <yencheng.lin@mic.com.tw>");
MODULE_DESCRIPTION("mitac_ly1200_32x_gpe driver");
MODULE_LICENSE("GPL");
