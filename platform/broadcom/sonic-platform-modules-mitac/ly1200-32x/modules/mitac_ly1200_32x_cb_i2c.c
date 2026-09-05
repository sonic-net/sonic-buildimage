#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/delay.h>
#include "bms_i2c.h"

#define BMS_CB_I2C_CLIENT_NUM 5
#define BMS_CB_ADAPTER_BASE 0

static struct i2c_client *bms_cb_clients[BMS_CB_I2C_CLIENT_NUM] = {NULL};
static int bms_cb_client_index = 0;

static int __init __find_i2c_adap(struct device *dev, void *data)
{
    const char *name = data;
    static const char *prefix = "i2c-";
    struct i2c_adapter *adapter;

    if (strncmp(dev_name(dev), prefix, strlen(prefix)) != 0)
    {
        return 0;
    }
    adapter = to_i2c_adapter(dev);

    return (strncmp(adapter->name, name, strlen(name)) == 0);
}

static int __init find_i2c_adapter_num(enum i2c_adapter_type type)
{
    struct device *dev = NULL;
    struct i2c_adapter *adapter;
    const char *name = bms_i2c_adapter_names[type];

    /* find the adapter by name */
    dev = bus_find_device(&i2c_bus_type, NULL, (void *)name,
                  __find_i2c_adap);
    if (!dev) {
        pr_err("%s: i2c adapter %s not found on system.\n",
               __func__, name);
        return -ENODEV;
    }
    adapter = to_i2c_adapter(dev);

    return adapter->nr;
}


static __init struct i2c_client *bms_cb_setup_spd(
        struct i2c_adapter *adap, int addr)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("spd", addr),
    };

    return i2c_new_device(adap, &info_spd);
}


static __init struct i2c_client *bms_cb_setup_eeprom_24c02(
        struct i2c_adapter *adap, int addr)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("24c02", addr),
    };

    return i2c_new_device(adap, &info_spd);
}

static __init struct i2c_client *bms_cb_setup_tmp75(
        struct i2c_adapter *adap, int addr)
{
    struct i2c_board_info info_spd = {
        I2C_BOARD_INFO("tmp75", addr),
    };

    return i2c_new_device(adap, &info_spd);
}

static __init struct i2c_client *bms_cb_setup_system_cpld(struct i2c_adapter *adap)
{
    struct i2c_board_info info = {
        I2C_BOARD_INFO("system_cpld", 0x31),
    };

    return i2c_new_device(adap, &info);
}

static int __init bms_cb_setup_devices_i801(void)
{
    struct i2c_adapter *adap;
    int adap_num = find_i2c_adapter_num(I2C_ADAPTER_I801);

    if (adap_num < 0)
        return adap_num;

    adap = i2c_get_adapter(adap_num);
    if (!adap) {
        pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        goto exit;
    }

    bms_cb_clients[bms_cb_client_index++] = bms_cb_setup_spd(adap, 0x50);
    bms_cb_clients[bms_cb_client_index++] = bms_cb_setup_spd(adap, 0x52);
    bms_cb_clients[bms_cb_client_index++] = bms_cb_setup_tmp75(adap, 0x4e);

exit:
    return 0;
}

static int __init bms_cb_setup_devices_ismt(void)
{
    struct i2c_adapter *adap;
    int adap_num = find_i2c_adapter_num(I2C_ADAPTER_ISMT);

    if (adap_num < 0)
        return adap_num;

    adap = i2c_get_adapter(adap_num);
    if (!adap) {
        pr_err("%s failed to get i2c adap %d.\n", __func__, adap_num);
        return 0;
    }

    bms_cb_clients[bms_cb_client_index++] = bms_cb_setup_system_cpld(adap);
    bms_cb_clients[bms_cb_client_index++] = bms_cb_setup_eeprom_24c02(adap, 0x56);
    return 0;
}

static int __init bms_cb_i2c_init(void)
{
    /* Initial bms_cb_slients array. */
    memset(bms_cb_clients, 0x0, BMS_CB_I2C_CLIENT_NUM);

    bms_cb_setup_devices_i801();
    mdelay(200);
    bms_cb_setup_devices_ismt();

    return 0;
}


static void __exit bms_cb_i2c_exit(void){
    int i;

    for (i=(bms_cb_client_index-1); i>=0; i--) {
        if (bms_cb_clients[i]) {
            i2c_unregister_device(bms_cb_clients[i]);
            bms_cb_clients[i] = NULL;
        }
    }

    bms_cb_client_index = 0;

}


module_init(bms_cb_i2c_init);
module_exit(bms_cb_i2c_exit);


MODULE_DESCRIPTION("mitac_ly1200_32x_cb_i2c driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

