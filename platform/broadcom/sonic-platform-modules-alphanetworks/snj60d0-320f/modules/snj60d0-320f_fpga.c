/*
 * A hwmon driver for the alphanetworks_snj60d0_320f_fpga
 *
 * Copyright (C) 2020 Alphanetworks Technology Corporation.
 * Robin Chen <Robin_chen@Alphanetworks.com>
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * see <http://www.gnu.org/licenses/>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>


#define DRIVER_NAME        "snj60d0_fpga"
#define PSU1_STATUS_REG    0x2
#define PSU2_STATUS_REG    0x3
#define FAN_PWM_REG        0x18

#define PSU_PRESENT_BIT    0x10
#define PSU_POWER_BIT      0x20
#define FAN_DIRECTION_BIT  0x1
#define FAN_PRESENT_BIT    0x2
#define FPGA_REVISION_BIT  0xF

#define FPGA_REVISION_REG  0x0
#define SYS_LED_REG        0x2D
#define SYS2_LED_REG       0x2E
#define FAN12_LED_REG      0x30
#define FAN34_LED_REG      0x31
#define FAN56_LED_REG      0x32

#define FAN1_STATUS_REG    0x5
#define FAN2_STATUS_REG    0x6
#define FAN3_STATUS_REG    0x7
#define FAN4_STATUS_REG    0x8
#define FAN5_STATUS_REG    0x9
#define FAN6_STATUS_REG    0xA

#define SYS_RESET1_REG          0x1C
#define SYS_RESET2_REG          0x1D
#define SYS_RESET3_REG          0x1E
//#define SWI_CTRL_REG     0x4

#define SYS_LOCATOR_LED_BITS    0x07
#define SYS_PWR_LED_BITS        0x38
#define PORT_LED_DISABLE_BITS   0x40
#define SYS_STATUS_LED_BITS     0x07
#define SYS_FAN_LED_BITS        0x38
#define FAN135_LED_BITS         0x07
#define FAN246_LED_BITS         0x38
#define REST_BUTTON_BITS        0x0

#define SFP_TX_FAULT_REG        0x25
#define SFP_TX_FAULT_MASK_REG   0x26
#define SFP_TX_DISABLE_REG      0x27
#define SFP_PRESENT_REG         0x28
#define SFP_PRESENT_MASK_REG    0x29
#define SFP_RX_LOSS_REG         0x2A
#define SFP_RX_LOSS_MASK_REG    0x2B
//#define SWI_CTRL_REG            0x34




static ssize_t psu_show_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fan_pwm_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t fpga_version_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t fan_show_status(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fan_show_status_reg(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t sys_sfp_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sys_sfp_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static ssize_t sys_led_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t sys_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static LIST_HEAD(fpga_client_list);
static struct mutex	 list_lock;

struct fpga_client_node {
    struct i2c_client *client;
    struct list_head   list;
};

/* Addresses scanned for alphanetworks_snj60d0_320f_fpga */
static const unsigned short normal_i2c[] = { 0x5E, I2C_CLIENT_END };

struct alphanetworks_snj60d0_320f_pwr_fpga_data {
  struct device      *hwmon_dev;
  struct mutex        update_lock;
  char model_name[9]; /* Model name, read from eeprom */
};


enum sysfs_fpga_attributes {
  PSU1_PRESENT,
  PSU2_PRESENT,
  PSU1_POWER_GOOD,
  PSU2_POWER_GOOD,
  FAN_PWM,
  FAN1_PRESENT=0x05,
  FAN2_PRESENT,
  FAN3_PRESENT,
  FAN4_PRESENT,
  FAN5_PRESENT,
  FAN6_PRESENT,
  FAN1_FRONT_SPEED_RPM=0x0b,
  FAN1_REAR_SPEED_RPM,
  FAN2_FRONT_SPEED_RPM,
  FAN2_REAR_SPEED_RPM,
  FAN3_FRONT_SPEED_RPM,
  FAN3_REAR_SPEED_RPM,
  FAN4_FRONT_SPEED_RPM,
  FAN4_REAR_SPEED_RPM,
  FAN5_FRONT_SPEED_RPM,
  FAN5_REAR_SPEED_RPM,
  FAN6_FRONT_SPEED_RPM,
  FAN6_REAR_SPEED_RPM,
  FAN1_FAULT,
  FAN2_FAULT,
  FAN3_FAULT,
  FAN4_FAULT,
  FAN5_FAULT,
  FAN6_FAULT,
  SYS_STATUS,
  SYS_PWR,
  SYS_FAN,
  SYS_LOCATOR,
  SFP_TX_FAULT,
  SFP_TX_FAULT_MASK,
  SFP_TX_DISABLE,
  SFP_PRESENT,
  SFP_PRESENT_MASK,
  SFP_RX_LOSS,
  SFP_RX_LOSS_MASK,
  FAN1_LED,
  FAN2_LED,
  FAN3_LED,
  FAN4_LED,
  FAN5_LED,
  FAN6_LED,
  SYS_RESET1,
  SYS_RESET2,
  SYS_RESET3,
  PORT_LED_DISABLE,
  FAN1_DIRECTION,
  FAN2_DIRECTION,
  FAN3_DIRECTION,
  FAN4_DIRECTION,
  FAN5_DIRECTION,
  FAN6_DIRECTION,
  FPGA_REVISION,
};






static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO, psu_show_status, NULL, PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO, psu_show_status, NULL, PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_power_good, S_IRUGO, psu_show_status, NULL, PSU1_POWER_GOOD);
static SENSOR_DEVICE_ATTR(psu2_power_good, S_IRUGO, psu_show_status, NULL, PSU2_POWER_GOOD);
static SENSOR_DEVICE_ATTR(fan_pwm, (0660), fan_pwm_show, set_fan_pwm, FAN_PWM);
static SENSOR_DEVICE_ATTR(fpga_revision, (0660), fpga_version_show, NULL, FPGA_REVISION);
static SENSOR_DEVICE_ATTR(fan1_present, S_IRUGO, fan_show_status_reg, NULL, FAN1_PRESENT);
static SENSOR_DEVICE_ATTR(fan2_present, S_IRUGO, fan_show_status_reg, NULL, FAN2_PRESENT);
static SENSOR_DEVICE_ATTR(fan3_present, S_IRUGO, fan_show_status_reg, NULL, FAN3_PRESENT);
static SENSOR_DEVICE_ATTR(fan4_present, S_IRUGO, fan_show_status_reg, NULL, FAN4_PRESENT);
static SENSOR_DEVICE_ATTR(fan5_present, S_IRUGO, fan_show_status_reg, NULL, FAN5_PRESENT);
static SENSOR_DEVICE_ATTR(fan6_present, S_IRUGO, fan_show_status_reg, NULL, FAN6_PRESENT);
static SENSOR_DEVICE_ATTR(fan1_direction, S_IRUGO, fan_show_status_reg, NULL, FAN1_DIRECTION);
static SENSOR_DEVICE_ATTR(fan2_direction, S_IRUGO, fan_show_status_reg, NULL, FAN2_DIRECTION);
static SENSOR_DEVICE_ATTR(fan3_direction, S_IRUGO, fan_show_status_reg, NULL, FAN3_DIRECTION);
static SENSOR_DEVICE_ATTR(fan4_direction, S_IRUGO, fan_show_status_reg, NULL, FAN4_DIRECTION);
static SENSOR_DEVICE_ATTR(fan5_direction, S_IRUGO, fan_show_status_reg, NULL, FAN5_DIRECTION);
static SENSOR_DEVICE_ATTR(fan6_direction, S_IRUGO, fan_show_status_reg, NULL, FAN6_DIRECTION);
static SENSOR_DEVICE_ATTR(fan1_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN1_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN2_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN3_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN4_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN5_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_front_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN6_FRONT_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan1_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN1_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN2_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN3_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN4_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN5_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_rear_speed_rpm, S_IRUGO, fan_show_status, NULL, FAN6_REAR_SPEED_RPM);

static SENSOR_DEVICE_ATTR(fan1_fault, S_IRUGO, fan_show_status, NULL, FAN1_FAULT); static SENSOR_DEVICE_ATTR(fan11_fault, S_IRUGO, fan_show_status, NULL, FAN1_FAULT);
static SENSOR_DEVICE_ATTR(fan2_fault, S_IRUGO, fan_show_status, NULL, FAN2_FAULT); static SENSOR_DEVICE_ATTR(fan12_fault, S_IRUGO, fan_show_status, NULL, FAN2_FAULT);
static SENSOR_DEVICE_ATTR(fan3_fault, S_IRUGO, fan_show_status, NULL, FAN3_FAULT); static SENSOR_DEVICE_ATTR(fan13_fault, S_IRUGO, fan_show_status, NULL, FAN3_FAULT);
static SENSOR_DEVICE_ATTR(fan4_fault, S_IRUGO, fan_show_status, NULL, FAN4_FAULT); static SENSOR_DEVICE_ATTR(fan14_fault, S_IRUGO, fan_show_status, NULL, FAN4_FAULT);
static SENSOR_DEVICE_ATTR(fan5_fault, S_IRUGO, fan_show_status, NULL, FAN5_FAULT); static SENSOR_DEVICE_ATTR(fan15_fault, S_IRUGO, fan_show_status, NULL, FAN5_FAULT);
static SENSOR_DEVICE_ATTR(fan6_fault, S_IRUGO, fan_show_status, NULL, FAN6_FAULT); static SENSOR_DEVICE_ATTR(fan16_fault, S_IRUGO, fan_show_status, NULL, FAN6_FAULT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, fan_show_status, NULL, FAN1_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan11_input, S_IRUGO, fan_show_status, NULL, FAN1_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, fan_show_status, NULL, FAN2_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan12_input, S_IRUGO, fan_show_status, NULL, FAN2_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO, fan_show_status, NULL, FAN3_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan13_input, S_IRUGO, fan_show_status, NULL, FAN3_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO, fan_show_status, NULL, FAN4_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan14_input, S_IRUGO, fan_show_status, NULL, FAN4_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO, fan_show_status, NULL, FAN5_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan15_input, S_IRUGO, fan_show_status, NULL, FAN5_REAR_SPEED_RPM);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO, fan_show_status, NULL, FAN6_FRONT_SPEED_RPM); static SENSOR_DEVICE_ATTR(fan16_input, S_IRUGO, fan_show_status, NULL, FAN6_REAR_SPEED_RPM);

static SENSOR_DEVICE_ATTR(sys_status,  (0600), sys_led_read, sys_led_write, SYS_STATUS);
static SENSOR_DEVICE_ATTR(port_led_disable,     (0660), sys_led_read, sys_led_write, PORT_LED_DISABLE);
static SENSOR_DEVICE_ATTR(sys_pwr,     (0660), sys_led_read, sys_led_write, SYS_PWR);
static SENSOR_DEVICE_ATTR(sys_locator, (0660), sys_led_read, sys_led_write, SYS_LOCATOR);
static SENSOR_DEVICE_ATTR(fan1_led,    (0660), sys_led_read, sys_led_write, FAN1_LED);
static SENSOR_DEVICE_ATTR(fan2_led,    (0660), sys_led_read, sys_led_write, FAN2_LED);
static SENSOR_DEVICE_ATTR(fan3_led,    (0660), sys_led_read, sys_led_write, FAN3_LED);
static SENSOR_DEVICE_ATTR(fan4_led,    (0660), sys_led_read, sys_led_write, FAN4_LED);
static SENSOR_DEVICE_ATTR(fan5_led,    (0660), sys_led_read, sys_led_write, FAN5_LED);
static SENSOR_DEVICE_ATTR(fan6_led,    (0660), sys_led_read, sys_led_write, FAN6_LED);
static SENSOR_DEVICE_ATTR(sys_reset1,  (0660), sys_led_read, sys_led_write, SYS_RESET1);
static SENSOR_DEVICE_ATTR(sys_reset2,  (0660), sys_led_read, sys_led_write, SYS_RESET2);
static SENSOR_DEVICE_ATTR(sys_reset3,  (0660), sys_led_read, sys_led_write, SYS_RESET3);

static SENSOR_DEVICE_ATTR(sfp_tx_fault,        (0660), sys_sfp_read, sys_sfp_write, SFP_TX_FAULT);
static SENSOR_DEVICE_ATTR(sfp_tx_fault_mask,   (0660), sys_sfp_read, sys_sfp_write, SFP_TX_FAULT_MASK);
static SENSOR_DEVICE_ATTR(sfp_tx_disable,      (0660), sys_sfp_read, sys_sfp_write, SFP_TX_DISABLE);
static SENSOR_DEVICE_ATTR(sfp_present,         (0660), sys_sfp_read, sys_sfp_write, SFP_PRESENT);
static SENSOR_DEVICE_ATTR(sfp_present_mask,    (0660), sys_sfp_read, sys_sfp_write, SFP_PRESENT_MASK);
static SENSOR_DEVICE_ATTR(sfp_rx_loss,         (0660), sys_sfp_read, sys_sfp_write, SFP_RX_LOSS);
static SENSOR_DEVICE_ATTR(sfp_rx_loss_mask,    (0660), sys_sfp_read, sys_sfp_write, SFP_RX_LOSS_MASK);


static struct attribute *alphanetworks_snj60d0_320f_fpga_attributes[] = {
  &sensor_dev_attr_psu1_present.dev_attr.attr,
  &sensor_dev_attr_psu2_present.dev_attr.attr,
  &sensor_dev_attr_psu1_power_good.dev_attr.attr,
  &sensor_dev_attr_psu2_power_good.dev_attr.attr,
  &sensor_dev_attr_fan_pwm.dev_attr.attr,
  &sensor_dev_attr_fan1_present.dev_attr.attr,
  &sensor_dev_attr_fan2_present.dev_attr.attr,
  &sensor_dev_attr_fan3_present.dev_attr.attr,
  &sensor_dev_attr_fan4_present.dev_attr.attr,
  &sensor_dev_attr_fan5_present.dev_attr.attr,
  &sensor_dev_attr_fan6_present.dev_attr.attr,
  &sensor_dev_attr_fan1_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan2_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan3_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan4_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan5_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan6_front_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan1_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan2_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan3_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan4_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan5_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan6_rear_speed_rpm.dev_attr.attr,
  &sensor_dev_attr_fan1_fault.dev_attr.attr,  &sensor_dev_attr_fan11_fault.dev_attr.attr,
  &sensor_dev_attr_fan2_fault.dev_attr.attr,  &sensor_dev_attr_fan12_fault.dev_attr.attr,
  &sensor_dev_attr_fan3_fault.dev_attr.attr,  &sensor_dev_attr_fan13_fault.dev_attr.attr,
  &sensor_dev_attr_fan4_fault.dev_attr.attr,  &sensor_dev_attr_fan14_fault.dev_attr.attr,
  &sensor_dev_attr_fan5_fault.dev_attr.attr,  &sensor_dev_attr_fan15_fault.dev_attr.attr,
  &sensor_dev_attr_fan6_fault.dev_attr.attr,  &sensor_dev_attr_fan16_fault.dev_attr.attr,
  &sensor_dev_attr_fan1_input.dev_attr.attr,  &sensor_dev_attr_fan11_input.dev_attr.attr,
  &sensor_dev_attr_fan2_input.dev_attr.attr,  &sensor_dev_attr_fan12_input.dev_attr.attr,
  &sensor_dev_attr_fan3_input.dev_attr.attr,  &sensor_dev_attr_fan13_input.dev_attr.attr,
  &sensor_dev_attr_fan4_input.dev_attr.attr,  &sensor_dev_attr_fan14_input.dev_attr.attr,
  &sensor_dev_attr_fan5_input.dev_attr.attr,  &sensor_dev_attr_fan15_input.dev_attr.attr,
  &sensor_dev_attr_fan6_input.dev_attr.attr,  &sensor_dev_attr_fan16_input.dev_attr.attr,
  &sensor_dev_attr_sys_status.dev_attr.attr,
  &sensor_dev_attr_sys_pwr.dev_attr.attr,
  &sensor_dev_attr_fan1_led.dev_attr.attr,
  &sensor_dev_attr_fan2_led.dev_attr.attr,
  &sensor_dev_attr_fan3_led.dev_attr.attr,
  &sensor_dev_attr_fan4_led.dev_attr.attr,
  &sensor_dev_attr_fan5_led.dev_attr.attr,
  &sensor_dev_attr_fan6_led.dev_attr.attr,
  &sensor_dev_attr_sys_reset1.dev_attr.attr,
  &sensor_dev_attr_sys_reset2.dev_attr.attr,
  &sensor_dev_attr_sys_reset3.dev_attr.attr,
  &sensor_dev_attr_sfp_tx_fault.dev_attr.attr,
  &sensor_dev_attr_sfp_tx_fault_mask.dev_attr.attr,
  &sensor_dev_attr_sfp_tx_disable.dev_attr.attr,
  &sensor_dev_attr_sfp_present.dev_attr.attr,
  &sensor_dev_attr_sfp_present_mask.dev_attr.attr,
  &sensor_dev_attr_sfp_rx_loss.dev_attr.attr,
  &sensor_dev_attr_sfp_rx_loss_mask.dev_attr.attr,
  &sensor_dev_attr_sys_locator.dev_attr.attr,
  &sensor_dev_attr_port_led_disable.dev_attr.attr,
  &sensor_dev_attr_fan1_direction.dev_attr.attr,
  &sensor_dev_attr_fan2_direction.dev_attr.attr,
  &sensor_dev_attr_fan3_direction.dev_attr.attr,
  &sensor_dev_attr_fan4_direction.dev_attr.attr,
  &sensor_dev_attr_fan5_direction.dev_attr.attr,
  &sensor_dev_attr_fan6_direction.dev_attr.attr,
  &sensor_dev_attr_fpga_revision.dev_attr.attr,
  NULL
};

static const struct attribute_group alphanetworks_snj60d0_320f_fpga_group = {
  .attrs = alphanetworks_snj60d0_320f_fpga_attributes,
};


static ssize_t psu_show_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    switch(sda->index) {
        case PSU1_PRESENT:
        case PSU1_POWER_GOOD:
          command = PSU1_STATUS_REG;
          break;
        case PSU2_PRESENT:
        case PSU2_POWER_GOOD:
          command = PSU2_STATUS_REG;
          break;
    }

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    switch(sda->index) {
        case PSU1_PRESENT:
        case PSU2_PRESENT:
          res = (val & PSU_PRESENT_BIT ? 0 : 1 );  
          break;
        case PSU1_POWER_GOOD:
        case PSU2_POWER_GOOD:
          res = (val & PSU_POWER_BIT ? 1 : 0 );
          break;
    }

    return sprintf(buf, "%d\n", res);
}

static ssize_t fan_pwm_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

    val = i2c_smbus_read_byte_data(client, FAN_PWM_REG);

    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d", val);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int error, value;

    error = kstrtoint(buf, 10, &value);
    if (error)
      return error;

    if (value < 0 || value > 0xFF)
      return -EINVAL;

    i2c_smbus_write_byte_data(client, FAN_PWM_REG, value);

    return count;
}

static ssize_t fpga_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    struct i2c_client *client = to_i2c_client(dev);

    val = i2c_smbus_read_byte_data(client, FPGA_REVISION_REG);

    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    return sprintf(buf, "%d\n", (val & FPGA_REVISION_BIT));
}

static ssize_t fan_show_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev);
    //    struct as7712_32x_fan_data *data = as7712_32x_fan_update_device(dev);
    ssize_t ret = 0;
    int val, val2;

    switch (sda->index) {
        /* case FAN_DUTY_CYCLE_PERCENTAGE: */
        /* { */
        /*   u32 duty_cycle = reg_val_to_duty_cycle(data->reg_val[FAN_DUTY_CYCLE_PERCENTAGE]); */
        /*   ret = sprintf(buf, "%u\n", duty_cycle); */
        /*   break; */
        /* } */
        case FAN1_FRONT_SPEED_RPM:
        case FAN2_FRONT_SPEED_RPM:
        case FAN3_FRONT_SPEED_RPM:
        case FAN4_FRONT_SPEED_RPM:
        case FAN5_FRONT_SPEED_RPM:
        case FAN6_FRONT_SPEED_RPM:
        case FAN1_REAR_SPEED_RPM:
        case FAN2_REAR_SPEED_RPM:
        case FAN3_REAR_SPEED_RPM:
        case FAN4_REAR_SPEED_RPM:
        case FAN5_REAR_SPEED_RPM:
        case FAN6_REAR_SPEED_RPM:
          val = i2c_smbus_read_byte_data(client, sda->index);
          ret = sprintf(buf, "%d\n", val * 150);
          break;
        case FAN1_FAULT:
        case FAN2_FAULT:
        case FAN3_FAULT:
        case FAN4_FAULT:
        case FAN5_FAULT:
        case FAN6_FAULT:
          val  = i2c_smbus_read_byte_data(client, (sda->index - FAN1_FAULT)*2 + FAN1_FRONT_SPEED_RPM);
          val2 = i2c_smbus_read_byte_data(client, (sda->index - FAN1_FAULT)*2 + FAN1_REAR_SPEED_RPM);
          ret = sprintf(buf, "%d\n", (val|val2) ? 0 : 1);
          break;
        default:
          break;
    }

  return ret;
}

static ssize_t fan_show_status_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev);
    ssize_t ret = 0;
    int val;
    u8 fan_status_offset = 0;

    switch (sda->index) {

        case FAN1_PRESENT:
        case FAN1_DIRECTION:
            fan_status_offset = FAN1_STATUS_REG;
            break;
        case FAN2_PRESENT:
        case FAN2_DIRECTION:
            fan_status_offset = FAN2_STATUS_REG;
            break;
        case FAN3_PRESENT:
        case FAN3_DIRECTION:
            fan_status_offset = FAN3_STATUS_REG;
            break;
        case FAN4_PRESENT:
        case FAN4_DIRECTION:
            fan_status_offset = FAN4_STATUS_REG;
            break;
        case FAN5_PRESENT:
        case FAN5_DIRECTION:
            fan_status_offset = FAN5_STATUS_REG;
            break;
        case FAN6_PRESENT:
        case FAN6_DIRECTION:
            fan_status_offset = FAN6_STATUS_REG;
            break;
        default:
            break;
    }

    switch (sda->index) {

        case FAN1_PRESENT:
        case FAN2_PRESENT:
        case FAN3_PRESENT:
        case FAN4_PRESENT:
        case FAN5_PRESENT:
        case FAN6_PRESENT:
            val = i2c_smbus_read_byte_data(client, fan_status_offset);
            /* Debug Msg
            printk(KERN_ERR "%s: Present: fan_status_offset: %d, Value: %d \n", __FUNCTION__, fan_status_offset, val);
            */
            ret = sprintf(buf, "%d\n", (val & FAN_PRESENT_BIT) ? 1 : 0);
            break;
        case FAN1_DIRECTION:
        case FAN2_DIRECTION:
        case FAN3_DIRECTION:
        case FAN4_DIRECTION:
        case FAN5_DIRECTION:
        case FAN6_DIRECTION:
            val = i2c_smbus_read_byte_data(client, fan_status_offset);
            /* Debug Msg
            printk(KERN_ERR "%s: Direction: fan_status_offset: %d, Value: %d \n", __FUNCTION__, fan_status_offset, val);
            */
            ret = sprintf(buf, "%d\n", (val & FAN_DIRECTION_BIT) ? 1 : 0);
            break;
        default:
            break;
    }
    
    return ret;
}

static ssize_t sys_led_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    switch(sda->index) {
        case SYS_LOCATOR:
        case SYS_PWR:			
        case PORT_LED_DISABLE:
            command = SYS_LED_REG;
            break;
        case SYS_STATUS: 
        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
        case FAN5_LED:
        case FAN6_LED:
          command = SYS2_LED_REG;
          break;
        case SYS_RESET1:
          command = SYS_RESET1_REG;
          break;
        case SYS_RESET2:
          command = SYS_RESET2_REG;
          break;
        case SYS_RESET3:
          command = SYS_RESET3_REG;
          break;
    }

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    switch(sda->index) {
        case SYS_LOCATOR:
          res = (val & SYS_LOCATOR_LED_BITS) >> 0;
          break;
        case SYS_PWR:
          res = (val & SYS_PWR_LED_BITS) >> 3;
          break;
        case PORT_LED_DISABLE:
          res = (val & PORT_LED_DISABLE_BITS) >> 6;
          break;
        case SYS_STATUS:
          res = (val & SYS_STATUS_LED_BITS) >> 0;
          break;
        case FAN1_LED:
        case FAN3_LED:
        case FAN5_LED:
        case FAN2_LED:
        case FAN4_LED:
        case FAN6_LED:
          res = (val & SYS_FAN_LED_BITS) >> 3;
          break;
        case SYS_RESET1:
          res = val;
          break;
        case SYS_RESET2:
          res = val;
          break;
        case SYS_RESET3:
          res = val;
          break;
    }

    return sprintf(buf, "%d\n", res);
}

static ssize_t sys_led_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int error, write, command, read;

    error = kstrtoint(buf, 10, &write);
    if (error)
      return error;


    switch(sda->index) {
        case SYS_LOCATOR:
        case SYS_PWR:
        case PORT_LED_DISABLE:			
		  if(write < 0 || write > 7) 
            return -EINVAL;
		  command = SYS_LED_REG;
		  break;
        case SYS_STATUS:
          if (write < 0 || write > 7) 
            return -EINVAL;
          command = SYS2_LED_REG;
          break;
        case FAN1_LED:
        case FAN2_LED:
        case FAN3_LED:
        case FAN4_LED:
        case FAN5_LED:
        case FAN6_LED:
          if (write < 0 || write > 7)
            return -EINVAL;
          command = SYS2_LED_REG;
          break;
        case SYS_RESET1:
          if (write < 0 || write > 255)
            return -EINVAL;
          command = SYS_RESET1_REG;
          break;
        case SYS_RESET2:
          if (write < 0 || write > 255)
            return -EINVAL;
          command = SYS_RESET2_REG;
          break;
        case SYS_RESET3:
          if (write < 0 || write > 255)
            return -EINVAL;
          command = SYS_RESET3_REG;
          break;
    }

    read = i2c_smbus_read_byte_data(client, command);
    if (read < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, read);
    }

    switch(sda->index) {
        case SYS_LOCATOR:
          read &= ~SYS_LOCATOR_LED_BITS;
          read |= write << 0;
          break;
        case SYS_PWR:
          read &= ~SYS_PWR_LED_BITS;
          read |= write << 3;
          break;
        case PORT_LED_DISABLE:
          read &= ~PORT_LED_DISABLE_BITS;
          read |= write << 6;
          break;
        case SYS_STATUS:
          read &= ~SYS_STATUS_LED_BITS;
          read |= write << 0;
          break;
        case FAN1_LED:
        case FAN3_LED:
        case FAN5_LED:
        case FAN2_LED:
        case FAN4_LED:
        case FAN6_LED:
          read &= ~SYS_FAN_LED_BITS;
          read |= write << 3;
          break;
        case SYS_RESET1:
          read = write;
          break;
        case SYS_RESET2:
          read = write;
          break;
        case SYS_RESET3:
          read = write;
          break;
    }

    i2c_smbus_write_byte_data(client, command, read);

    return count;
}

static ssize_t sys_sfp_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);
    int error, write, command;

    error = kstrtoint(buf, 10, &write);
    if (error)
      return error;

    switch(sda->index) {
        case SFP_TX_FAULT:
            command = SFP_TX_FAULT_MASK_REG;
            break;
        case SFP_TX_DISABLE:
            command = SFP_TX_DISABLE_REG;
            break;
        case SFP_PRESENT:
            command = SFP_PRESENT_MASK_REG;
        case SFP_RX_LOSS:
            command = SFP_RX_LOSS_MASK_REG;
            break;
        default:
            return 0;
    }

    i2c_smbus_write_byte_data(client, command, write);

    return count;

}

static ssize_t sys_sfp_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0, res = 0;
    u8 command;
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(attr);

    switch(sda->index) {
        case SFP_TX_FAULT:
            command = SFP_TX_FAULT_REG;
            break;
        case SFP_TX_DISABLE:
            command = SFP_TX_DISABLE_REG;
            break;
        case SFP_PRESENT:
            command = SFP_PRESENT_REG;
            break;
        case SFP_RX_LOSS:
            command = SFP_RX_LOSS_REG;
            break;
    }

    val = i2c_smbus_read_byte_data(client, command);
    if (val < 0) {
        dev_dbg(&client->dev, "fpga(0x%x) reg(0x1) err %d\n", client->addr, val);
    }

    res = val;

    return sprintf(buf, "%d\n", res);

}


static void alpha_i2c_fpga_add_client(struct i2c_client *client)
{
    struct fpga_client_node *node = kzalloc(sizeof(struct fpga_client_node), GFP_KERNEL);

    if (!node) {
        dev_dbg(&client->dev, "Can't allocate fpga_client_node (0x%x)\n", client->addr);
        return;
    }

    node->client = client;

    mutex_lock(&list_lock);
    list_add(&node->list, &fpga_client_list);
    mutex_unlock(&list_lock);
}

static void alpha_i2c_fpga_remove_client(struct i2c_client *client)
{
    struct list_head		*list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int found = 0;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list)
    {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client == client) {
            found = 1;
            break;
        }
    }

    if (found) {
        list_del(list_node);
        kfree(fpga_node);
    }

    mutex_unlock(&list_lock);
}

static int alpha_i2c_fpga_probe(struct i2c_client *client,
                                 const struct i2c_device_id *dev_id)
{
    int status;
    struct alphanetworks_snj60d0_320f_pwr_fpga_data* data;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct alphanetworks_snj60d0_320f_pwr_fpga_data), GFP_KERNEL);
    if (!data) {
      status = -ENOMEM;
      goto exit;
    }

    status = sysfs_create_group(&client->dev.kobj, &alphanetworks_snj60d0_320f_fpga_group);
    if (status) {
      goto exit;
    }

    dev_info(&client->dev, "chip found\n");
    alpha_i2c_fpga_add_client(client);

    data->hwmon_dev = hwmon_device_register(&client->dev);
    if (IS_ERR(data->hwmon_dev)) {
      status = PTR_ERR(data->hwmon_dev);
      goto exit;
    }

    dev_info(&client->dev, "%s: pwr_fpga '%s'\n",
             dev_name(data->hwmon_dev), client->name);

    return 0;

exit:
    return status;
}

static int alpha_i2c_fpga_remove(struct i2c_client *client)
{
    struct alphanetworks_snj60d0_320f_pwr_fpga_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &alphanetworks_snj60d0_320f_fpga_group);
    alpha_i2c_fpga_remove_client(client);
    kfree(data);

    return 0;
}

static const struct i2c_device_id alpha_i2c_fpga_id[] = {
    { DRIVER_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, alpha_i2c_fpga_id);

static struct i2c_driver alpha_i2c_fpga_driver = {
    .class		= I2C_CLASS_HWMON,
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe		= alpha_i2c_fpga_probe,
    .remove	   	= alpha_i2c_fpga_remove,
    .id_table	= alpha_i2c_fpga_id,
    .address_list = normal_i2c,
};

int alpha_i2c_fpga_read(unsigned short fpga_addr, u8 reg)
{
    struct list_head   *list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int ret = -EPERM;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list)
    {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client->addr == fpga_addr) {
            ret = i2c_smbus_read_byte_data(fpga_node->client, reg);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(alpha_i2c_fpga_read);

int alpha_i2c_fpga_write(unsigned short fpga_addr, u8 reg, u8 value)
{
    struct list_head   *list_node = NULL;
    struct fpga_client_node *fpga_node = NULL;
    int ret = -EIO;

    mutex_lock(&list_lock);

    list_for_each(list_node, &fpga_client_list)
    {
        fpga_node = list_entry(list_node, struct fpga_client_node, list);

        if (fpga_node->client->addr == fpga_addr) {
            ret = i2c_smbus_write_byte_data(fpga_node->client, reg, value);
            break;
        }
    }

    mutex_unlock(&list_lock);

    return ret;
}
EXPORT_SYMBOL(alpha_i2c_fpga_write);

static int __init alpha_i2c_fpga_init(void)
{
    mutex_init(&list_lock);
    return i2c_add_driver(&alpha_i2c_fpga_driver);
}

static void __exit alpha_i2c_fpga_exit(void)
{
    i2c_del_driver(&alpha_i2c_fpga_driver);
}

MODULE_AUTHOR("Alpha-SID6");
MODULE_DESCRIPTION("alpha fpga driver");
MODULE_LICENSE("GPL");

module_init(alpha_i2c_fpga_init);
module_exit(alpha_i2c_fpga_exit);
