# Changing the virtual device

You can control the hw sku and default factory configuration for the VS image 
by modifying the content of the file default_sku in this directory. 

The format of default_sku is a single line:

```
<hw_key> <default_preset>
```

## Allowable values for hw_key

| hw_key | Device |
| ------ | ------ |
| Force10-S6000 | Dell Force10 S6000|
| brcm_gearbox_vs | Similar to Force10-S6000, but implements a virtual BRCM81724 Gearbox Phy |

## Allowable values for default_preset

These include "t1", "l2", and "empty". See the file 
sonic-buildimage/src/sonic-config-engine/config_samples.py for details on how 
each default_preset value is interpreted.

# Changing the hwsku of an existing VS switch

To change the default hwsku for a VS image that has already been built and installed, follow these steps:

- Edit /usr/share/sonic/device/x86_64-kvm_x86_64-r0/default_sku. For details, see the section below (Device Specific Documentation)
- Edit /etc/sonic/config_db.json, and change the "hwsku" key in DEVICE_METADATA:localhost to match the hw_key used in default_sku. Example:

  "DEVICE_METADATA": {
    "localhost": {
        ...
        "hwsku": "brcm_gearbox_vs",
        ...
    }
  },
  ...
- Reboot the switch
- Use "show platform summary" to verify, and follow any steps specific to the platform, as needed, such as those described below for the brcm_gearbox_vs hwsku.

# Device Specific Documentation

For general info on building, see https://github.com/Azure/sonic-buildimage/blob/master/README.md

## Force-10-S6000

This is the default VS for SONiC. To enable, set contents of default_sku to:

```
Force10-S6000 t1
```

To build:

```
make init
make configure PLATFORM=vs
make target/sonic-vs.img.gz
```

## brcm_gearbox_vs

This sku simulates a device with a Broadcom BRCM81724 gearbox PHY. To enable,
set default_sku to:
 

```
brcm_gearbox_vs t1
```

To build (same as Force-10-S6000):

```
make init
make configure PLATFORM=vs
make target/sonic-vs.img.gz
```

To verify, install and bring up SONiC. There will be a new gbsyncd docker 
which is designed to respond to configuration directed towards the gearbox phy
"switch". swss will create that gearbox switch on startup after detecting the
gearbox is present (this is done by a short lived gearsyncd that runs in the
swss docker). 

The commands "show gearbox interfaces status" and "show gearbox phys status" can be 
used to verify the virtual gearbox phy has been created. See https://github.com/Azure/sonic-utilities/blob/master/doc/Command-Reference.md#gearbox for details.
