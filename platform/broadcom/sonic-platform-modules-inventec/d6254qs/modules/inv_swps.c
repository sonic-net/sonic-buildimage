/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/dmi.h>
#include "inv_swps.h"

static int ctl_major;
static int port_major;
static int ioexp_total;
static int port_total;
static int auto_config;
static int flag_i2c_reset;
static int flag_mod_state;
static unsigned gpio_rest_mux;
static struct class *swp_class_p = NULL;
static struct inv_platform_s *platform_p = NULL;
static struct inv_ioexp_layout_s *ioexp_layout = NULL;
static struct inv_port_layout_s *port_layout = NULL;

static void swp_polling_worker(struct work_struct *work);
static DECLARE_DELAYED_WORK(swp_polling, swp_polling_worker);

static int reset_i2c_topology(void);


static int
__swp_match(struct device *dev,
#ifdef SWPS_KERN_VER_AF_3_10

            const void *data){
#else
            void *data){
#endif

    char *name = (char *)data;
    if (strcmp(dev_name(dev), name) == 0)
        return 1;
    return 0;
}


struct device *
get_swpdev_by_name(char *name){
    struct device *dev = class_find_device(swp_class_p,
                                           NULL,
                                           name,
                                           __swp_match);
    return dev;
}


static int
sscanf_2_int(const char *buf) {

    int   result  = -EBFONT;
    char *hex_tag = "0x";

    if (strcspn(buf, hex_tag) == 0) {
        if (sscanf(buf,"%x",&result)) {
            return result;
        }
    } else {
        if (sscanf(buf,"%d",&result)) {
            return result;
        }
        if(sscanf(buf,"-%d",&result)) {
            return -result;
        }
        if (sscanf(buf,"%x",&result)) {
            return result;
        }
    }
    return -EBFONT;
}


static int
sscanf_2_binary(const char *buf) {

    int result = sscanf_2_int(buf);

    if (result < 0){
        return -EBFONT;
    }
    switch (result) {
        case 0:
        case 1:
            return result;
        default:
            break;
    }
    return -EBFONT;
}


static int
_get_polling_period(void) {

    int retval = 0;

    if (SWP_POLLING_PERIOD == 0) {
        return 0;
    }
    retval = ((SWP_POLLING_PERIOD * HZ) / 1000);
    if (retval == 0) {
        return 1;
    }
    return retval;
}


static struct transvr_obj_s *
_get_transvr_obj(char *dev_name) {

    struct device *dev_p = NULL;
    struct transvr_obj_s *transvr_obj_p = NULL;

    dev_p = get_swpdev_by_name(dev_name);
    if (!dev_p){
        return NULL;
    }
    transvr_obj_p = dev_get_drvdata(dev_p);
    if (!transvr_obj_p){
        return NULL;
    }
    return transvr_obj_p;
}


static void
unlock_tobj_all(void) {

    struct transvr_obj_s *tobj_p;
    char port_name[32];
    int  port_id = 0;
    int  minor_curr = 0;

    for (minor_curr=0; minor_curr<port_total; minor_curr++) {
        port_id = port_layout[minor_curr].port_id;
        memset(port_name, 0, sizeof(port_name));
        snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
        tobj_p = _get_transvr_obj(port_name);
        if (!tobj_p) {
            continue;
        }
        unlock_transvr_obj(tobj_p);
    }
}


static int
lock_tobj_all(void) {

    struct transvr_obj_s *tobj_p;
    char port_name[32] = "ERR";
    int  port_id    = 0;
    int  minor_curr = 0;

    for (minor_curr=0; minor_curr<port_total; minor_curr++) {
        port_id = port_layout[minor_curr].port_id;
        memset(port_name, 0, sizeof(port_name));
        snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
        tobj_p = _get_transvr_obj(port_name);
        if (!tobj_p) {
            SWPS_DEBUG("%s: get %s tobj fail!\n", __func__, port_name);
            goto err_lock_tobj_all;
        }
        lock_transvr_obj(tobj_p);
    }
    return 0;

err_lock_tobj_all:
    unlock_tobj_all();
    return -1;
}


static int
_update_auto_config_2_trnasvr(void) {
    
    struct transvr_obj_s *tobj_p;
    char port_name[32] = "ERR";
    int port_id = 0;
    int curr    = 0;
    int retval  = 0;
        
    for (curr=0; curr<port_total; curr++) {
        port_id = port_layout[curr].port_id;
        memset(port_name, 0, sizeof(port_name));
        snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
        tobj_p = _get_transvr_obj(port_name);
        if (!tobj_p) {
            retval = -1;
            continue;
        }
        lock_transvr_obj(tobj_p);
        tobj_p->auto_config = auto_config;
        unlock_transvr_obj(tobj_p);
        SWPS_DEBUG("%s: Set %s auto_config=%d\n",
                   __func__, tobj_p->swp_name, auto_config);
    }
    return retval;
}


/* ========== R/W Functions module control attribute ==========
 */
static ssize_t
show_attr_platform(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    return snprintf(buf_p, 32, "%s\n", platform_p->name);
}


static ssize_t
show_attr_version(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    return snprintf(buf_p, 8, "%s\n", SWP_VERSION);
}


static ssize_t
show_attr_status(struct device *dev_p,
                 struct device_attribute *attr_p,
                 char *buf_p){

    return snprintf(buf_p, 8, "%d\n", flag_mod_state);
}


static ssize_t
show_attr_auto_config(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    return snprintf(buf_p, 8, "%d\n", auto_config);
}


static int
_check_reset_pwd(const char *buf_p,
                 size_t count) {

    int  in_max = 64;
    int  in_len = (int)count;
    char in_val[64] = "ERR";
    char *emsg = "ERR";

    if (in_len >= in_max) {
        emsg = "input too much";
        goto err_check_reset_pwd;
    }
    if (!sscanf(buf_p,"%s",in_val)) {
        emsg = "format incorrect";
        goto err_check_reset_pwd;
    }
    if (strcmp(in_val, SWP_RESET_PWD) != 0) {
        emsg = "password incorrect";
        goto err_check_reset_pwd;
    }
    return 0;

err_check_reset_pwd:
    SWPS_ERR("%s: %s\n", __func__, emsg);
    return -1;
}


static ssize_t
store_attr_reset_i2c(struct device *dev_p,
                     struct device_attribute *attr_p,
                     const char *buf_p,
                     size_t count){

    if (_check_reset_pwd(buf_p, count) < 0) {
        return -EBFONT;
    }
    /* Polling mode */
    if (SWP_POLLING_ENABLE) {
        SWPS_INFO("%s: reset I2C <mode>:polling\n", __func__);
        flag_i2c_reset = 1;
        return count;
    }
    /* Direct mode */
    SWPS_INFO("%s: reset I2C go. <mode>:direct\n", __func__);
    if (reset_i2c_topology() < 0) {
        SWPS_ERR("%s: reset fail!\n", __func__);
        return -EIO;
    }
    SWPS_INFO("%s: reset I2C ok. <mode>:direct\n", __func__);
    return count;
}


static ssize_t
store_attr_reset_swps(struct device *dev_p,
                      struct device_attribute *attr_p,
                      const char *buf_p,
                      size_t count){

    struct transvr_obj_s *tobj_p;
    char port_name[32] = "ERR";
    int  port_id    = 0;
    int  minor_curr = 0;

    if (_check_reset_pwd(buf_p, count) < 0) {
        return -EBFONT;
    }
    for (minor_curr=0; minor_curr<port_total; minor_curr++) {
        port_id = port_layout[minor_curr].port_id;
        memset(port_name, 0, sizeof(port_name));
        snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
        tobj_p = _get_transvr_obj(port_name);
        if (!tobj_p) {
            continue;
        }
        lock_transvr_obj(tobj_p);
        tobj_p->state = STATE_TRANSVR_DISCONNECTED;
        unlock_transvr_obj(tobj_p);
        SWPS_INFO("%s: reset:%s\n", __func__, tobj_p->swp_name);
    }
    return count;
}


static ssize_t
store_attr_auto_config(struct device *dev_p,
                       struct device_attribute *attr_p,
                       const char *buf_p,
                       size_t count){

    int input_val = sscanf_2_int(buf_p);
    
    if (input_val < 0){
        return -EBFONT;
    }
    if ((input_val != 0) && (input_val != 1)) {
        return -EBFONT;
    }
    auto_config = input_val;
    _update_auto_config_2_trnasvr();
    return count;
}


/* ========== Show functions: For transceiver attribute ==========
 */
static ssize_t
_show_transvr_hex_attr(struct transvr_obj_s* tobj_p,
                       int (*get_func)(struct transvr_obj_s* tobj_p),
                       char *buf_p) {
    size_t len;
    int result;

    lock_transvr_obj(tobj_p);
    result = get_func(tobj_p);
    unlock_transvr_obj(tobj_p);
    if (result < 0){
        len = snprintf(buf_p, 8, "%d\n", result);
    } else {
        len = snprintf(buf_p, 8, "0x%02x\n", result);
    }
    return len;
}


static ssize_t
_show_transvr_int_attr(struct transvr_obj_s* tobj_p,
                       int (*get_func)(struct transvr_obj_s* tobj_p),
                       char *buf_p) {
    size_t len;

    lock_transvr_obj(tobj_p);
    len = snprintf(buf_p, 16, "%d\n", get_func(tobj_p));
    unlock_transvr_obj(tobj_p);
    return len;
}


static ssize_t
_show_transvr_str_attr(struct transvr_obj_s* tobj_p,
                       int (*get_func)(struct transvr_obj_s* tobj_p, char* buf),
                       char *buf_p) {
    size_t len;

    lock_transvr_obj(tobj_p);
    len = get_func(tobj_p, buf_p);
    unlock_transvr_obj(tobj_p);
    return len;
}


static ssize_t
show_attr_id(struct device *dev_p,
             struct device_attribute *attr_p,
             char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_id,
                                  buf_p);
}


static ssize_t
show_attr_ext_id(struct device *dev_p,
                 struct device_attribute *attr_p,
                 char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_ext_id,
                                  buf_p);
}


static ssize_t
show_attr_connector(struct device *dev_p,
                    struct device_attribute *attr_p,
                    char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_connector,
                                  buf_p);
}


static ssize_t
show_attr_vendor_name(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_vendor_name,
                                  buf_p);
}


static ssize_t
show_attr_vendor_pn(struct device *dev_p,
                    struct device_attribute *attr_p,
                    char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_vendor_pn,
                                  buf_p);
}


static ssize_t
show_attr_vendor_rev(struct device *dev_p,
                     struct device_attribute *attr_p,
                     char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_vendor_rev,
                                  buf_p);
}


static ssize_t
show_attr_vendor_sn(struct device *dev_p,
                    struct device_attribute *attr_p,
                    char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_vendor_sn,
                                  buf_p);
}


static ssize_t
show_attr_power_cls(struct device *dev_p,
                    struct device_attribute *attr_p,
                    char *buf_p){
    size_t len;
    int result;
    struct transvr_obj_s *tobj_p;

    tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    lock_transvr_obj(tobj_p);
    result = tobj_p->get_power_cls(tobj_p);
    unlock_transvr_obj(tobj_p);
    if (result < 0){
        len = snprintf(buf_p, 16, "%d\n", result);
    } else {
        len = snprintf(buf_p, 16, "Power Class %d\n", result);
    }
    return len;
}


static ssize_t
show_attr_br(struct device *dev_p,
             struct device_attribute *attr_p,
             char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_br,
                                  buf_p);
}


static ssize_t
show_attr_len_sm(struct device *dev_p,
                 struct device_attribute *attr_p,
                 char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_sm,
                                  buf_p);
}


static ssize_t
show_attr_len_smf(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_smf,
                                  buf_p);
}


static ssize_t
show_attr_len_om1(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_om1,
                                  buf_p);
}


static ssize_t
show_attr_len_om2(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_om2,
                                  buf_p);
}


static ssize_t
show_attr_len_om3(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_om3,
                                  buf_p);
}


static ssize_t
show_attr_len_om4(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_len_om4,
                                  buf_p);
}


static ssize_t
show_attr_comp_rev(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_comp_rev,
                                  buf_p);
}


static ssize_t
show_attr_comp_eth(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_comp_eth_1,
                                  buf_p);
}


static ssize_t
show_attr_comp_eth_10(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_comp_eth_10,
                                  buf_p);
}


static ssize_t
show_attr_comp_eth_10_40(struct device *dev_p,
                         struct device_attribute *attr_p,
                         char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_comp_eth_10_40,
                                  buf_p);
}


static ssize_t
show_attr_comp_extend(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_comp_extend,
                                  buf_p);
}


static ssize_t
show_attr_rate_id(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_rate_id,
                                  buf_p);
}


static ssize_t
show_attr_temperature(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_curr_temp,
                                  buf_p);
}


static ssize_t
show_attr_voltage(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_curr_vol,
                                  buf_p);
}


static ssize_t
show_attr_tx_bias(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_tx_bias,
                                  buf_p);
}


static ssize_t
show_attr_tx_power(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_tx_power,
                                  buf_p);
}


static ssize_t
show_attr_tx_eq(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_tx_eq,
                                  buf_p);
}


static ssize_t
show_attr_rx_power(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_rx_power,
                                  buf_p);
}


static ssize_t
show_attr_rx_am(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_rx_am,
                                  buf_p);
}


static ssize_t
show_attr_rx_em(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_rx_em,
                                  buf_p);
}


static ssize_t
show_attr_wavelength(struct device *dev_p,
                     struct device_attribute *attr_p,
                     char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_wavelength,
                                  buf_p);
}


static ssize_t
show_attr_extphy_offset(struct device *dev_p,
                        struct device_attribute *attr_p,
                        char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_extphy_offset,
                                  buf_p);
}


static ssize_t
show_attr_extphy_reg(struct device *dev_p,
                     struct device_attribute *attr_p,
                     char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_extphy_reg,
                                  buf_p);
}


static ssize_t
show_attr_info(struct device *dev_p,
               struct device_attribute *attr_p,
               char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_info,
                                  buf_p);
}


static ssize_t
show_attr_if_type(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_if_type,
                                  buf_p);
}


static ssize_t
show_attr_if_speed(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_if_speed,
                                  buf_p);
}


static ssize_t
show_attr_if_lane(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_if_lane,
                                  buf_p);
}


static ssize_t
show_attr_cdr(struct device *dev_p,
              struct device_attribute *attr_p,
              char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_hex_attr(tobj_p,
                                  tobj_p->get_cdr,
                                  buf_p);
}


static ssize_t
show_attr_soft_rs0(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_soft_rs0,
                                  buf_p);
}


static ssize_t
show_attr_soft_rs1(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_int_attr(tobj_p,
                                  tobj_p->get_soft_rs1,
                                  buf_p);
}


static ssize_t
show_attr_soft_rx_los(struct device *dev_p,
                      struct device_attribute *attr_p,
                      char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_soft_rx_los,
                                  buf_p);
}


static ssize_t
show_attr_soft_tx_disable(struct device *dev_p,
                          struct device_attribute *attr_p,
                          char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_soft_tx_disable,
                                  buf_p);
}


static ssize_t
show_attr_soft_tx_fault(struct device *dev_p,
                        struct device_attribute *attr_p,
                        char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_soft_tx_fault,
                                  buf_p);
}


static ssize_t
show_attr_auto_tx_disable(struct device *dev_p,
                          struct device_attribute *attr_p,
                          char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if(!tobj_p){
        return -ENODEV;
    }
    return _show_transvr_str_attr(tobj_p,
                                  tobj_p->get_auto_tx_disable,
                                  buf_p);
}


/* ========== Store functions: transceiver (R/W) attribute ==========
 */
static ssize_t
_store_transvr_int_attr(struct transvr_obj_s* tobj_p,
                        int (*set_func)(struct transvr_obj_s *tobj_p, int input_val),
                        const char *buf_p,
                        size_t count) {
    int input, err;

    input = sscanf_2_int(buf_p);
    if (input < 0){
        return -EBFONT;
    }
    lock_transvr_obj(tobj_p);
    err = set_func(tobj_p, input);
    unlock_transvr_obj(tobj_p);
    if (err < 0){
        return err;
    }
    return count;
}


static ssize_t
_store_transvr_byte_hex_attr(struct transvr_obj_s* tobj_p,
                             int (*set_func)(struct transvr_obj_s *tobj_p, int input_val),
                             const char *buf_p,
                             size_t count) {
    int input, err;

    input = sscanf_2_int(buf_p);
    if ((input < 0) || (input > 0xff)){
        return -EBFONT;
    }
    lock_transvr_obj(tobj_p);
    err = set_func(tobj_p, input);
    unlock_transvr_obj(tobj_p);
    if (err < 0){
        return err;
    }
    return count;
}


static ssize_t
_store_transvr_binary_attr(struct transvr_obj_s* tobj_p,
                           int (*set_func)(struct transvr_obj_s *tobj_p, int input_val),
                           const char *buf_p,
                           size_t count) {
    int input, err;

    input = sscanf_2_binary(buf_p);
    if (input < 0){
        return -EBFONT;
    }
    lock_transvr_obj(tobj_p);
    err = set_func(tobj_p, input);
    unlock_transvr_obj(tobj_p);
    if (err < 0){
        return err;
    }
    return count;
}


static ssize_t
store_attr_cdr(struct device *dev_p,
               struct device_attribute *attr_p,
               const char *buf_p,
               size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_byte_hex_attr(tobj_p,
                                        tobj_p->set_cdr,
                                        buf_p,
                                        count);
}


static ssize_t
store_attr_soft_rs0(struct device *dev_p,
                   struct device_attribute *attr_p,
                   const char *buf_p,
                   size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_binary_attr(tobj_p,
                                      tobj_p->set_soft_rs0,
                                      buf_p,
                                      count);
}


static ssize_t
store_attr_soft_rs1(struct device *dev_p,
                   struct device_attribute *attr_p,
                   const char *buf_p,
                   size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_binary_attr(tobj_p,
                                      tobj_p->set_soft_rs1,
                                      buf_p,
                                      count);
}


static ssize_t
store_attr_soft_tx_disable(struct device *dev_p,
                           struct device_attribute *attr_p,
                           const char *buf_p,
                           size_t count) {

    int check = sscanf_2_int(buf_p);
    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    if ((check < 0) || (check > 0xf)){
        return -EBFONT;
    }
    return _store_transvr_byte_hex_attr(tobj_p,
                                        tobj_p->set_soft_tx_disable,
                                        buf_p,
                                        count);
}


static ssize_t
store_attr_auto_tx_disable(struct device *dev_p,
                           struct device_attribute *attr_p,
                           const char *buf_p,
                           size_t count) {

    int err   = -EPERM;
    int input = sscanf_2_int(buf_p);
    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    if ((input < 0) || (input > 0xf)){
        if (input != VAL_TRANSVR_FUNCTION_DISABLE) {
            return -EBFONT;
        }
    }
    lock_transvr_obj(tobj_p);
    err = tobj_p->set_auto_tx_disable(tobj_p, input);
    unlock_transvr_obj(tobj_p);
    if (err < 0){
        return err;
    }
    return count;
}


static ssize_t
store_attr_tx_eq(struct device *dev_p,
                 struct device_attribute *attr_p,
                 const char *buf_p,
                 size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_int_attr(tobj_p,
                                   tobj_p->set_tx_eq,
                                   buf_p,
                                   count);
}


static ssize_t
store_attr_rx_am(struct device *dev_p,
                 struct device_attribute *attr_p,
                 const char *buf_p,
                 size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_int_attr(tobj_p,
                                   tobj_p->set_rx_am,
                                   buf_p,
                                   count);
}


static ssize_t
store_attr_rx_em(struct device *dev_p,
                 struct device_attribute *attr_p,
                 const char *buf_p,
                 size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_int_attr(tobj_p,
                                   tobj_p->set_rx_em,
                                   buf_p,
                                   count);
}


static ssize_t
store_attr_extphy_offset(struct device *dev_p,
                         struct device_attribute *attr_p,
                         const char *buf_p,
                         size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_int_attr(tobj_p,
                                   tobj_p->set_extphy_offset,
                                   buf_p,
                                   count);
}


static ssize_t
store_attr_extphy_reg(struct device *dev_p,
                      struct device_attribute *attr_p,
                      const char *buf_p,
                      size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _store_transvr_int_attr(tobj_p,
                                   tobj_p->set_extphy_reg,
                                   buf_p,
                                   count);
}

/* ========== Show functions: For I/O Expander attribute ==========
 */
static ssize_t
_show_ioexp_binary_attr(struct transvr_obj_s *tobj_p,
                        int (*get_func)(struct ioexp_obj_s *ioexp_p, int voffset),
                        char *buf_p) {
    size_t len;
    struct ioexp_obj_s *ioexp_p = tobj_p->ioexp_obj_p;

    if (!ioexp_p) {
        SWPS_ERR(" %s: data corruption! <port>:%s\n", __func__, tobj_p->swp_name);
        return -ENODATA;
    }
    mutex_lock(&ioexp_p->lock);
    len = snprintf(buf_p, 8, "%d\n", get_func(ioexp_p, tobj_p->ioexp_virt_offset));
    mutex_unlock(&ioexp_p->lock);
    return len;
}


static ssize_t
show_attr_present(struct device *dev_p,
                  struct device_attribute *attr_p,
                  char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_present,
                                   buf_p);
}


static ssize_t
show_attr_tx_fault(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_tx_fault,
                                   buf_p);
}


static ssize_t
show_attr_rxlos(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_rxlos,
                                   buf_p);
}


static ssize_t
show_attr_tx_disable(struct device *dev_p,
                     struct device_attribute *attr_p,
                     char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_tx_disable,
                                   buf_p);
}


static ssize_t
show_attr_reset(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_reset,
                                   buf_p);
}


static ssize_t
show_attr_lpmod(struct device *dev_p,
                struct device_attribute *attr_p,
                char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_lpmod,
                                   buf_p);
}


static ssize_t
show_attr_modsel(struct device *dev_p,
                 struct device_attribute *attr_p,
                 char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_modsel,
                                   buf_p);
}


static ssize_t
show_attr_hard_rs0(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_hard_rs0,
                                   buf_p);
}


static ssize_t
show_attr_hard_rs1(struct device *dev_p,
                   struct device_attribute *attr_p,
                   char *buf_p){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p){
        return -ENODEV;
    }
    return _show_ioexp_binary_attr(tobj_p,
                                   tobj_p->ioexp_obj_p->get_hard_rs1,
                                   buf_p);
}


/* ========== Store functions: For I/O Expander (R/W) attribute ==========
 */
static ssize_t
_store_ioexp_binary_attr(struct transvr_obj_s *tobj_p,
                         int (*set_func)(struct ioexp_obj_s *ioexp_p,
                                         int virt_offset, int input_val),
                         const char *buf_p,
                         size_t count) {

    int input, err;
    struct ioexp_obj_s *ioexp_p = tobj_p->ioexp_obj_p;

    if (!ioexp_p) {
        SWPS_ERR("%s: data corruption! <port>:%s\n",
                 __func__, tobj_p->swp_name);
        return -ENODATA;
    }
    input = sscanf_2_binary(buf_p);
    if (input < 0) {
        return -EBFONT;
    }
    mutex_lock(&ioexp_p->lock);
    err = set_func(ioexp_p, tobj_p->ioexp_virt_offset, input);
    mutex_unlock(&ioexp_p->lock);
    if (err < 0){
        return err;
    }
    return count;
}

static ssize_t
store_attr_tx_disable(struct device *dev_p,
                      struct device_attribute *attr_p,
                      const char *buf_p,
                      size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_tx_disable,
                                    buf_p,
                                    count);
}


static ssize_t
store_attr_reset(struct device *dev_p,
                 struct device_attribute *attr_p,
                 const char *buf_p,
                 size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_reset,
                                    buf_p,
                                    count);
}


static ssize_t
store_attr_lpmod(struct device *dev_p,
                 struct device_attribute *attr_p,
                 const char *buf_p,
                 size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_lpmod,
                                    buf_p,
                                    count);
}


static ssize_t
store_attr_modsel(struct device *dev_p,
                  struct device_attribute *attr_p,
                  const char *buf_p,
                  size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_modsel,
                                    buf_p,
                                    count);
}


static ssize_t
store_attr_hard_rs0(struct device *dev_p,
                    struct device_attribute *attr_p,
                    const char *buf_p,
                    size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_hard_rs0,
                                    buf_p,
                                    count);
}


static ssize_t
store_attr_hard_rs1(struct device *dev_p,
                    struct device_attribute *attr_p,
                    const char *buf_p,
                    size_t count){

    struct transvr_obj_s *tobj_p = dev_get_drvdata(dev_p);
    if (!tobj_p) {
        return -ENODEV;
    }
    return _store_ioexp_binary_attr(tobj_p,
                                    tobj_p->ioexp_obj_p->set_hard_rs1,
                                    buf_p,
                                    count);
}


/* ========== SWPS attribute: For module control ==========
 */
static DEVICE_ATTR(platform,        S_IRUGO,         show_attr_platform,        NULL);
static DEVICE_ATTR(version,         S_IRUGO,         show_attr_version,         NULL);
static DEVICE_ATTR(status,          S_IRUGO,         show_attr_status,          NULL);
static DEVICE_ATTR(reset_i2c,       S_IWUSR,         NULL,                      store_attr_reset_i2c);
static DEVICE_ATTR(reset_swps,      S_IWUSR,         NULL,                      store_attr_reset_swps);
static DEVICE_ATTR(auto_config,     S_IRUGO|S_IWUSR, show_attr_auto_config,     store_attr_auto_config);

/* ========== Transceiver attribute: from eeprom ==========
 */
static DEVICE_ATTR(id,              S_IRUGO,         show_attr_id,              NULL);
static DEVICE_ATTR(ext_id,          S_IRUGO,         show_attr_ext_id,          NULL);
static DEVICE_ATTR(connector,       S_IRUGO,         show_attr_connector,       NULL);
static DEVICE_ATTR(vendor_name,     S_IRUGO,         show_attr_vendor_name,     NULL);
static DEVICE_ATTR(vendor_pn,       S_IRUGO,         show_attr_vendor_pn,       NULL);
static DEVICE_ATTR(vendor_rev,      S_IRUGO,         show_attr_vendor_rev,      NULL);
static DEVICE_ATTR(vendor_sn,       S_IRUGO,         show_attr_vendor_sn,       NULL);
static DEVICE_ATTR(power_cls,       S_IRUGO,         show_attr_power_cls,       NULL);
static DEVICE_ATTR(br,              S_IRUGO,         show_attr_br,              NULL);
static DEVICE_ATTR(len_sm,          S_IRUGO,         show_attr_len_sm,          NULL);
static DEVICE_ATTR(len_smf,         S_IRUGO,         show_attr_len_smf,         NULL);
static DEVICE_ATTR(len_om1,         S_IRUGO,         show_attr_len_om1,         NULL);
static DEVICE_ATTR(len_om2,         S_IRUGO,         show_attr_len_om2,         NULL);
static DEVICE_ATTR(len_om3,         S_IRUGO,         show_attr_len_om3,         NULL);
static DEVICE_ATTR(len_om4,         S_IRUGO,         show_attr_len_om4,         NULL);
static DEVICE_ATTR(comp_rev,        S_IRUGO,         show_attr_comp_rev,        NULL);
static DEVICE_ATTR(comp_eth,        S_IRUGO,         show_attr_comp_eth,        NULL);
static DEVICE_ATTR(comp_eth_10,     S_IRUGO,         show_attr_comp_eth_10,     NULL);
static DEVICE_ATTR(comp_eth_10_40,  S_IRUGO,         show_attr_comp_eth_10_40,  NULL);
static DEVICE_ATTR(comp_extend,     S_IRUGO,         show_attr_comp_extend,     NULL);
static DEVICE_ATTR(rate_id,         S_IRUGO,         show_attr_rate_id,         NULL);
static DEVICE_ATTR(temperature,     S_IRUGO,         show_attr_temperature,     NULL);
static DEVICE_ATTR(voltage,         S_IRUGO,         show_attr_voltage,         NULL);
static DEVICE_ATTR(tx_bias,         S_IRUGO,         show_attr_tx_bias,         NULL);
static DEVICE_ATTR(tx_power,        S_IRUGO,         show_attr_tx_power,        NULL);
static DEVICE_ATTR(rx_power,        S_IRUGO,         show_attr_rx_power,        NULL);
static DEVICE_ATTR(info,            S_IRUGO,         show_attr_info,            NULL);
static DEVICE_ATTR(if_type,         S_IRUGO,         show_attr_if_type,         NULL);
static DEVICE_ATTR(if_speed,        S_IRUGO,         show_attr_if_speed,        NULL);
static DEVICE_ATTR(if_lane,         S_IRUGO,         show_attr_if_lane,         NULL);
static DEVICE_ATTR(soft_rx_los,     S_IRUGO,         show_attr_soft_rx_los,     NULL);
static DEVICE_ATTR(soft_tx_fault,   S_IRUGO,         show_attr_soft_tx_fault,   NULL);
static DEVICE_ATTR(wavelength,      S_IRUGO,         show_attr_wavelength,      NULL);
static DEVICE_ATTR(tx_eq,           S_IRUGO|S_IWUSR, show_attr_tx_eq,           store_attr_tx_eq);
static DEVICE_ATTR(rx_am,           S_IRUGO|S_IWUSR, show_attr_rx_am,           store_attr_rx_am);
static DEVICE_ATTR(rx_em,           S_IRUGO|S_IWUSR, show_attr_rx_em,           store_attr_rx_em);
static DEVICE_ATTR(cdr,             S_IRUGO|S_IWUSR, show_attr_cdr,             store_attr_cdr);
static DEVICE_ATTR(soft_rs0,        S_IRUGO|S_IWUSR, show_attr_soft_rs0,        store_attr_soft_rs0);
static DEVICE_ATTR(soft_rs1,        S_IRUGO|S_IWUSR, show_attr_soft_rs1,        store_attr_soft_rs1);
static DEVICE_ATTR(soft_tx_disable, S_IRUGO|S_IWUSR, show_attr_soft_tx_disable, store_attr_soft_tx_disable);
static DEVICE_ATTR(auto_tx_disable, S_IRUGO|S_IWUSR, show_attr_auto_tx_disable, store_attr_auto_tx_disable);
static DEVICE_ATTR(extphy_offset,   S_IRUGO|S_IWUSR, show_attr_extphy_offset,   store_attr_extphy_offset);
static DEVICE_ATTR(extphy_reg,      S_IRUGO|S_IWUSR, show_attr_extphy_reg,      store_attr_extphy_reg);

/* ========== IO Expander attribute: from expander ==========
 */
static DEVICE_ATTR(present,         S_IRUGO,         show_attr_present,         NULL);
static DEVICE_ATTR(tx_fault,        S_IRUGO,         show_attr_tx_fault,        NULL);
static DEVICE_ATTR(rxlos,           S_IRUGO,         show_attr_rxlos,           NULL);
static DEVICE_ATTR(tx_disable,      S_IRUGO|S_IWUSR, show_attr_tx_disable,      store_attr_tx_disable);
static DEVICE_ATTR(reset,           S_IRUGO|S_IWUSR, show_attr_reset,           store_attr_reset);
static DEVICE_ATTR(lpmod,           S_IRUGO|S_IWUSR, show_attr_lpmod,           store_attr_lpmod);
static DEVICE_ATTR(modsel,          S_IRUGO|S_IWUSR, show_attr_modsel,          store_attr_modsel);
static DEVICE_ATTR(hard_rs0,        S_IRUGO|S_IWUSR, show_attr_hard_rs0,        store_attr_hard_rs0);
static DEVICE_ATTR(hard_rs1,        S_IRUGO|S_IWUSR, show_attr_hard_rs1,        store_attr_hard_rs1);

/* ========== Functions for module handling ==========
 */
static void
clean_port_obj(void){

    dev_t dev_num;
    char dev_name[32];
    struct device *device_p;
    struct transvr_obj_s *transvr_obj_p;
    int minor_curr, port_id;

    for (minor_curr=0; minor_curr<port_total; minor_curr++){
        port_id = port_layout[minor_curr].port_id;
        memset(dev_name, 0, sizeof(dev_name));
        snprintf(dev_name, sizeof(dev_name), "%s%d", SWP_DEV_PORT, port_id);
        device_p = get_swpdev_by_name(dev_name);
        if (!device_p){
            continue;
        }
        transvr_obj_p = dev_get_drvdata(device_p);
        if (transvr_obj_p){
            kfree(transvr_obj_p->i2c_client_p);
            kfree(transvr_obj_p->vendor_name);
            kfree(transvr_obj_p->vendor_pn);
            kfree(transvr_obj_p->vendor_rev);
            kfree(transvr_obj_p->vendor_sn);
            kfree(transvr_obj_p->worker_p);
            kfree(transvr_obj_p);
        }
        dev_num = MKDEV(port_major, minor_curr);
        device_unregister(device_p);
        device_destroy(swp_class_p, dev_num);
    }
    SWPS_DEBUG("%s: done.\n", __func__);
}


static void
clean_swps_common(void){

    dev_t dev_num;
    struct device *device_p;

    device_p = get_swpdev_by_name(SWP_DEV_MODCTL);
    if (device_p){
        dev_num = MKDEV(ctl_major, 1);
        device_unregister(device_p);
        device_destroy(swp_class_p, dev_num);
    }
    cancel_delayed_work_sync(&swp_polling);
    SWPS_DEBUG("%s: done.\n", __func__);
}


static int
get_platform_type(void){

    int i;
    int pf_total = ARRAY_SIZE(platform_map);
    char log_msg[64] = "ERROR";

    platform_p = kzalloc(sizeof(struct inv_platform_s), GFP_KERNEL);
    if (!platform_p){
        snprintf(log_msg, sizeof(log_msg), "kzalloc fail");
        goto err_get_platform_type_1;
    }
    memset(platform_p->name, 0, sizeof(platform_p->name));

    switch (PLATFORM_SETTINGS) {
        case PLATFORM_TYPE_AUTO:
            snprintf(platform_p->name, (sizeof(platform_p->name) - 1),
                    "%s", dmi_get_system_info(DMI_BOARD_NAME));
            for (i=0; i<pf_total; i++) {
                if (strcmp(platform_p->name, platform_map[i].name) == 0) {
                    platform_p->id = platform_map[i].id;
                    snprintf(log_msg, sizeof(log_msg),
                            "Auto detect platform: %d (%s)",
                            platform_p->id, platform_p->name);
                    goto ok_get_platform_type_1;
                }
            }
            snprintf(log_msg, sizeof(log_msg),
                    "Auto detect fail! detect platform: %s",
                    platform_p->name);
            goto err_get_platform_type_2;

        case PLATFORM_TYPE_MAGNOLIA:
        case PLATFORM_TYPE_MAGNOLIA_FNC:
        case PLATFORM_TYPE_REDWOOD:
        case PLATFORM_TYPE_REDWOOD_FSL:
        case PLATFORM_TYPE_HUDSON32I_GA:
        case PLATFORM_TYPE_SPRUCE:
        case PLATFORM_TYPE_CYPRESS_GA1:
        case PLATFORM_TYPE_CYPRESS_GA2:
        case PLATFORM_TYPE_CYPRESS_BAI:
        case PLATFORM_TYPE_TAHOE:
        case PLATFORM_TYPE_SEQUOIA_GA:
        case PLATFORM_TYPE_LAVENDER_GA:
        case PLATFORM_TYPE_LAVENDER_ONL:
            platform_p->id = PLATFORM_SETTINGS;
            for (i=0; i<pf_total; i++) {
                if (PLATFORM_SETTINGS == platform_map[i].id) {
                    snprintf(platform_p->name, (sizeof(platform_p->name) - 1),
                            "%s", platform_map[i].name);
                    snprintf(log_msg, sizeof(log_msg),
                            "User setup platform: %d (%s)",
                            platform_p->id, platform_p->name);
                    goto ok_get_platform_type_1;
                }
            }
            snprintf(log_msg, sizeof(log_msg),
                    "Internal error, can not map id:%d",
                    PLATFORM_SETTINGS);
            goto err_get_platform_type_2;

        default:
            break;
    }
    snprintf(log_msg, sizeof(log_msg),
            "PLATFORM_SETTINGS:%d undefined", PLATFORM_SETTINGS);
    goto err_get_platform_type_2;

ok_get_platform_type_1:
    SWPS_DEBUG("%s: %s, <conf>:%d\n", __func__, log_msg, PLATFORM_SETTINGS);
    return 0;

err_get_platform_type_2:
    kfree(platform_p);
err_get_platform_type_1:
    SWPS_ERR("%s: %s <conf>:%d\n", __func__, log_msg, PLATFORM_SETTINGS);
    return -1;
}


static int
get_layout_info(void){

    switch (platform_p->id) {
#ifdef SWPS_MAGNOLIA
        case PLATFORM_TYPE_MAGNOLIA:
        case PLATFORM_TYPE_MAGNOLIA_FNC:
            gpio_rest_mux = magnolia_gpio_rest_mux;
            ioexp_layout  = magnolia_ioexp_layout;
            port_layout   = magnolia_port_layout;
            ioexp_total   = ARRAY_SIZE(magnolia_ioexp_layout);
            port_total    = ARRAY_SIZE(magnolia_port_layout);
            break;
#endif
#ifdef SWPS_REDWOOD
        case PLATFORM_TYPE_REDWOOD:
            gpio_rest_mux = redwood_gpio_rest_mux;
            ioexp_layout  = redwood_ioexp_layout;
            port_layout   = redwood_port_layout;
            ioexp_total   = ARRAY_SIZE(redwood_ioexp_layout);
            port_total    = ARRAY_SIZE(redwood_port_layout);
            break;
#endif
#ifdef SWPS_HUDSON32I_GA
        case PLATFORM_TYPE_HUDSON32I_GA:
            gpio_rest_mux = hudsin32iga_gpio_rest_mux;
            ioexp_layout  = hudson32iga_ioexp_layout;
            port_layout   = hudson32iga_port_layout;
            ioexp_total   = ARRAY_SIZE(hudson32iga_ioexp_layout);
            port_total    = ARRAY_SIZE(hudson32iga_port_layout);
            break;
#endif
#ifdef SWPS_SPRUCE
        case PLATFORM_TYPE_SPRUCE:
            gpio_rest_mux = spruce_gpio_rest_mux;
            ioexp_layout  = spruce_ioexp_layout;
            port_layout   = spruce_port_layout;
            ioexp_total   = ARRAY_SIZE(spruce_ioexp_layout);
            port_total    = ARRAY_SIZE(spruce_port_layout);
            break;
#endif
#ifdef SWPS_CYPRESS_GA1
        case PLATFORM_TYPE_CYPRESS_GA1:
            gpio_rest_mux = cypress_ga1_gpio_rest_mux;
            ioexp_layout  = cypress_ga1_ioexp_layout;
            port_layout   = cypress_ga1_port_layout;
            ioexp_total   = ARRAY_SIZE(cypress_ga1_ioexp_layout);
            port_total    = ARRAY_SIZE(cypress_ga1_port_layout);
            break;
#endif
#ifdef SWPS_CYPRESS_GA2
        case PLATFORM_TYPE_CYPRESS_GA2:
            gpio_rest_mux = cypress_ga2_gpio_rest_mux;
            ioexp_layout  = cypress_ga2_ioexp_layout;
            port_layout   = cypress_ga2_port_layout;
            ioexp_total   = ARRAY_SIZE(cypress_ga2_ioexp_layout);
            port_total    = ARRAY_SIZE(cypress_ga2_port_layout);
            break;
#endif
#ifdef SWPS_CYPRESS_BAI
        case PLATFORM_TYPE_CYPRESS_BAI:
            gpio_rest_mux = cypress_b_gpio_rest_mux;
            ioexp_layout  = cypress_b_ioexp_layout;
            port_layout   = cypress_b_port_layout;
            ioexp_total   = ARRAY_SIZE(cypress_b_ioexp_layout);
            port_total    = ARRAY_SIZE(cypress_b_port_layout);
            break;
#endif
#ifdef SWPS_REDWOOD_FSL
        case PLATFORM_TYPE_REDWOOD_FSL:
            gpio_rest_mux = redwood_fsl_gpio_rest_mux;
            ioexp_layout  = redwood_fsl_ioexp_layout;
            port_layout   = redwood_fsl_port_layout;
            ioexp_total   = ARRAY_SIZE(redwood_fsl_ioexp_layout);
            port_total    = ARRAY_SIZE(redwood_fsl_port_layout);
            break;
#endif
#ifdef SWPS_TAHOE
        case PLATFORM_TYPE_TAHOE:
            gpio_rest_mux = tahoe_gpio_rest_mux;
            ioexp_layout  = tahoe_ioexp_layout;
            port_layout   = tahoe_port_layout;
            ioexp_total   = ARRAY_SIZE(tahoe_ioexp_layout);
            port_total    = ARRAY_SIZE(tahoe_port_layout);
            break;
#endif
#ifdef SWPS_SEQUOIA
        case PLATFORM_TYPE_SEQUOIA_GA:
            gpio_rest_mux = sequoia_gpio_rest_mux;
            ioexp_layout  = sequoia_ioexp_layout;
            port_layout   = sequoia_port_layout;
            ioexp_total   = ARRAY_SIZE(sequoia_ioexp_layout);
            port_total    = ARRAY_SIZE(sequoia_port_layout);
            break;
#endif
#ifdef SWPS_LAVENDER
        case PLATFORM_TYPE_LAVENDER_GA:
        case PLATFORM_TYPE_LAVENDER_ONL:
            gpio_rest_mux = lavender_gpio_rest_mux;
            ioexp_layout  = lavender_ioexp_layout;
            port_layout   = lavender_port_layout;
            ioexp_total   = ARRAY_SIZE(lavender_ioexp_layout);
            port_total    = ARRAY_SIZE(lavender_port_layout);
            break;
#endif
        default:
            SWPS_ERR(" Invalid platform: %d (%s)\n",
                    platform_p->id, platform_p->name);
            return -1;
    }
    SWPS_INFO("Start to initial platform: %d (%s)\n",
              platform_p->id, platform_p->name);
    return 0;
}


/* ========== Functions for objects operations ==========
 */
static int
__detect_issues_port(int minor_num) {

    struct transvr_obj_s *tobj_p;
    int  port_id = port_layout[minor_num].port_id;
    char port_name[32] = "ERR";
    char *i2c_emsg = "detected bad transceiver/cable";

    memset(port_name, 0, sizeof(port_name));
    snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
    tobj_p = _get_transvr_obj(port_name);
    if (!tobj_p) {
        SWPS_INFO("%s: tobj_p is NULL <minor>:%d\n", __func__, minor_num);
        return -1;
    }
    if (resync_channel_tier_2(tobj_p) < 0) {
        if (check_channel_tier_1() < 0) {
            alarm_msg_2_user(tobj_p, i2c_emsg);
            return -2;;
        }
    }
    return 0;
}


static int
_detect_issues_port(void) {
    /* OK  : retrun -1;
     * Fail: return fail at which minor number (0~N)
     */
    char *emsg = "ERR";
    int  minor = 0;
    int  minor_2st = 1;

    /* Force moving the initial channel pointer
     * Filter out case of fail at minor-0 port
     */
    while (minor_2st < port_total) {
        minor = minor_2st;
        if (__detect_issues_port(minor_2st) < 0) {
            emsg = "detect minor_2st fail";
            goto err_p_detect_issues_port;
        }
        minor_2st += 8;
    }
    /* Scan all port */
    for (minor=0; minor<port_total; minor++) {
        if (__detect_issues_port(minor) < 0) {
            emsg = "is I2C issues port";
            goto err_p_detect_issues_port;
        }
    }
    return -1;

err_p_detect_issues_port:
    SWPS_DEBUG("%s: minor:%d %s\n", __func__, minor, emsg);
    return minor;
}


static int
_isolate_issues_port(int minor_num) {

    struct transvr_obj_s *tobj_p;
    char port_name[32];
    int  port_id = port_layout[minor_num].port_id;

    memset(port_name, 0, sizeof(port_name));
    snprintf(port_name, sizeof(port_name), "%s%d", SWP_DEV_PORT, port_id);
    tobj_p = _get_transvr_obj(port_name);
    if (!tobj_p) {
        return -1;
    }
    return isolate_transvr_obj(tobj_p);
}


static int
_reset_i2c_topology_tier_1(void) {

    if (reset_mux_gpio() < 0) {
        SWPS_ERR("%s: reset MUX GPIO fail!\n", __func__);
        return -1;
    }
    SWPS_DEBUG("%s: reset_mux_gpio OK.\n", __func__);
    if (resync_channel_tier_1() < 0) {
        SWPS_ERR("%s: resync tier-1 channel fail!\n", __func__);
        return -1;
    }
    SWPS_DEBUG("%s: resync_channel_tier_1 OK.\n", __func__);
    return 0;
}


static int
reset_i2c_topology(void) {

    int run_count = 0;
    int minor_err = 0;
    char *emsg = "ERR";

    /* Get all lock */
    if (lock_ioexp_all() < 0) {
        emsg = "lock_ioexp_all fail";
        goto err_reset_i2c_topology_1;
    }
    if (lock_tobj_all() < 0) {
        emsg = "lock_tobj_all fail";
        goto err_reset_i2c_topology_2;
    }
    SWPS_DEBUG("%s: get Lock OK.\n", __func__);
    /* Reset tier-1 topology */
    if (_reset_i2c_topology_tier_1() < 0) {
        emsg = "initial task fail";
        goto err_reset_i2c_topology_3;
    }
    SWPS_DEBUG("%s: reset tier-1 OK.\n", __func__);
    /* Reset tier-2 topology */
    for (run_count=0; run_count<port_total; run_count++) {
        SWPS_DEBUG("%s: reset tier-2 No.%d-run go.\n", __func__, run_count);
        minor_err = _detect_issues_port();
        /* Case: no issues port */
        if (minor_err < 0) {
            break;
        }
        /* Case: has issues port */
        if (_isolate_issues_port(minor_err) < 0) {
            emsg = "isolate fail";
            goto err_reset_i2c_topology_3;
        }
        if (_reset_i2c_topology_tier_1() < 0) {
            emsg = "reset tier-1 fail";
            goto err_reset_i2c_topology_3;
        }
    }
    unlock_tobj_all();
    unlock_ioexp_all();
    flag_mod_state = SWP_STATE_NORMAL;
    SWPS_INFO("%s: done\n", __func__);
    return 0;

err_reset_i2c_topology_3:
    unlock_tobj_all();
err_reset_i2c_topology_2:
    unlock_ioexp_all();
err_reset_i2c_topology_1:
    flag_mod_state = SWP_STATE_I2C_DIE;
    SWPS_INFO("%s: %s <minor>:%d\n", __func__, emsg, minor_err);
    return -1;
}


static int
check_transvr_obj_one(char *dev_name){
    /* [Return]
     *    0 : Doesn't need to take care
     *   -1 : Single error
     *   -2 : Critical error (I2C topology die)
     *   -9 : Internal error
     */
    struct transvr_obj_s *tobj_p = NULL;
    int retval = -9;

    tobj_p = _get_transvr_obj(dev_name);
    if (!tobj_p) {
        SWPS_ERR("%s: %s _get_transvr_obj fail\n",
                __func__, dev_name);
        return -9;
    }
    /* Check transceiver current status */
    lock_transvr_obj(tobj_p);
    retval = tobj_p->check(tobj_p);
    unlock_transvr_obj(tobj_p);
    switch (retval) {
        case 0:
        case ERR_TRANSVR_UNPLUGGED:
        case ERR_TRNASVR_BE_ISOLATED:
        case ERR_TRANSVR_TASK_BUSY:
            return 0;

        case ERR_TRANSVR_I2C_CRASH:
        default:
            break;
    }
    /* Identify abnormal case */
    if (check_channel_tier_1() < 0) {
        SWPS_DEBUG("%s: %s critical error <err>:%d\n",
                   __func__, dev_name, retval);
        return -2;
    }
    SWPS_DEBUG("%s: %s single error <err>:%d\n",
               __func__, dev_name, retval);
    return -1;
}


static int
check_transvr_objs(void){

    char dev_name[32];
    int port_id, err_code;
    int minor_curr = 0;

    for (minor_curr=0; minor_curr<port_total; minor_curr++) {
        /* Generate device name */
        port_id = port_layout[minor_curr].port_id;
        memset(dev_name, 0, sizeof(dev_name));
        snprintf(dev_name, sizeof(dev_name), "%s%d", SWP_DEV_PORT, port_id);
        /* Handle current status */
        err_code = check_transvr_obj_one(dev_name);
        switch (err_code) {
            case  0:
            case -1:
                break;

            case -2:
                SWPS_DEBUG("%s: %s reset I2C GO.\n",
                           __func__, dev_name);
                if (reset_i2c_topology() < 0) {
                    goto err_check_transvr_objs;
                }
                SWPS_DEBUG("%s: %s reset I2C OK.\n",
                           __func__, dev_name);
                break;

            case -9:
            default:
                SWPS_DEBUG("%s: %s internal error <err>:%d\n",
                        __func__, dev_name, err_code);
                break;
        }
    }
    return 0;

err_check_transvr_objs:
    SWPS_ERR("%s: %s reset_i2c_topology fail.\n",
               __func__, dev_name);
    return -1;
}


static void
swp_polling_worker(struct work_struct *work){

    /* Reset I2C */
    if (flag_i2c_reset) {
        goto polling_reset_i2c;
    }
    /* Check IOEXP */
    if (check_ioexp_objs() < 0) {
        goto polling_reset_i2c;
    }
    /* Check transceiver */
    if (check_transvr_objs() < 0) {
        SWPS_DEBUG("%s: check_transvr_objs fail.\n", __func__);
        flag_i2c_reset = 1;
    }
    goto polling_schedule_round;

polling_reset_i2c:
    SWPS_DEBUG("%s: reset_i2c_topology start.\n", __func__);
    if (reset_i2c_topology() < 0) {
        SWPS_ERR("%s: reset i2c fail!\n", __func__);
        flag_i2c_reset = 1;
    } else {
        SWPS_DEBUG("%s: reset_i2c_topology OK.\n", __func__);
        flag_i2c_reset = 0;
    }
polling_schedule_round:
    schedule_delayed_work(&swp_polling, _get_polling_period());
}


/* ========== Functions for register something ==========
 */
static int
register_transvr_common_attr(struct device *device_p){

    char *err_attr = NULL;

    if (device_create_file(device_p, &dev_attr_id) < 0) {
          err_attr = "dev_attr_id";
           goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_ext_id) < 0) {
          err_attr = "dev_attr_ext_id";
           goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_connector) < 0) {
          err_attr = "dev_attr_connector";
           goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_vendor_name) < 0) {
        err_attr = "dev_attr_vendor_name";
         goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_vendor_pn) < 0) {
        err_attr = "dev_attr_vendor_pn";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_vendor_rev) < 0) {
        err_attr = "dev_attr_vendor_rev";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_vendor_sn) < 0) {
        err_attr = "dev_attr_vendor_sn";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_br) < 0) {
        err_attr = "dev_attr_br";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_smf) < 0) {
        err_attr = "dev_attr_len_smf";
         goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_om1) < 0) {
         err_attr = "dev_attr_len_om1";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_om2) < 0) {
         err_attr = "dev_attr_len_om2";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_om3) < 0) {
         err_attr = "dev_attr_len_om3";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_om4) < 0) {
         err_attr = "dev_attr_len_om4";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_comp_extend) < 0) {
        err_attr = "dev_attr_comp_extend";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_comp_eth) < 0) {
        err_attr = "dev_attr_comp_eth";
         goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_comp_rev) < 0) {
        err_attr = "dev_attr_comp_rev";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_info) < 0) {
        err_attr = "dev_attr_info";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_if_type) < 0) {
        err_attr = "dev_attr_if_type";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_if_speed) < 0) {
        err_attr = "dev_attr_if_speed";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_if_lane) < 0) {
        err_attr = "dev_attr_if_lane";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_temperature) < 0) {
        err_attr = "dev_attr_temperature";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_voltage) < 0) {
        err_attr = "dev_attr_voltage";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_tx_bias) < 0) {
        err_attr = "dev_attr_tx_bias";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_tx_power) < 0) {
        err_attr = "dev_attr_tx_power";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_rx_power) < 0) {
        err_attr = "dev_attr_rx_power";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_tx_eq) < 0) {
        err_attr = "dev_attr_tx_eq";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_rx_em) < 0) {
        err_attr = "dev_attr_rx_em";
        goto err_transvr_comm_attr;
    }
    if (device_create_file(device_p, &dev_attr_wavelength) < 0) {
        err_attr = "dev_attr_wavelength";
        goto err_transvr_comm_attr;
    }
    return 0;

err_transvr_comm_attr:
    SWPS_ERR("%s: %s\n", __func__, err_attr);
    return -1;
}

static int
register_transvr_sfp_attr(struct device *device_p){

    char *err_attr = NULL;

    if (register_transvr_common_attr(device_p) < 0) {
        err_attr = "register_transvr_common_attr";
        goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_comp_eth_10) < 0) {
        err_attr = "dev_attr_comp_eth_10";
         goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_len_sm) < 0) {
        err_attr = "dev_attr_len_sm";
         goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_rate_id) < 0) {
        err_attr = "dev_attr_rate_id";
        goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_soft_rs0) < 0) {
        err_attr = "dev_attr_soft_rs0";
        goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_soft_rs1) < 0) {
        err_attr = "dev_attr_soft_rs1";
        goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_extphy_offset) < 0) {
        err_attr = "dev_attr_extphy_offset";
        goto err_transvr_sfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_extphy_reg) < 0) {
        err_attr = "dev_attr_extphy_reg";
        goto err_transvr_sfp_attr;
    }
    return 0;

err_transvr_sfp_attr:
    SWPS_ERR("%s: %s\n", __func__, err_attr);
    return -1;
}


static int
register_transvr_qsfp_attr(struct device *device_p){

    char *err_attr = NULL;

    if (register_transvr_common_attr(device_p) < 0) {
        err_attr = "register_transvr_common_attr";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_comp_eth_10_40) < 0) {
        err_attr = "dev_attr_comp_eth_10_40";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_power_cls) < 0) {
        err_attr = "dev_attr_power_cls";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_soft_rx_los) < 0) {
        err_attr = "soft_rx_los";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_soft_tx_disable) < 0) {
        err_attr = "soft_tx_disable";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_auto_tx_disable) < 0) {
        err_attr = "auto_tx_disable";
        goto err_transvr_qsfp_attr;
    }
    if (device_create_file(device_p, &dev_attr_soft_tx_fault) < 0) {
        err_attr = "soft_tx_fault";
        goto err_transvr_qsfp_attr;
    }
    return 0;

err_transvr_qsfp_attr:
    SWPS_ERR("%s: %s\n", __func__, err_attr);
    return -1;
}


static int
register_transvr_qsfp28_attr(struct device *device_p){

    char *err_attr = NULL;

    if (register_transvr_qsfp_attr(device_p) < 0){
        err_attr = "register_transvr_qsfp_attr";
        goto err_transvr_qsfp28_attr;
    }
    if (device_create_file(device_p, &dev_attr_cdr) < 0) {
        err_attr = "dev_attr_cdr";
        goto err_transvr_qsfp28_attr;
    }
    if (device_create_file(device_p, &dev_attr_rx_am) < 0) {
        err_attr = "dev_attr_rx_am";
        goto err_transvr_qsfp28_attr;
    }
    return 0;

err_transvr_qsfp28_attr:
    SWPS_ERR("%s: %s\n", __func__, err_attr);
    return -1;
}


static int
register_transvr_attr(struct device *device_p,
                      struct transvr_obj_s *transvr_obj){

    switch (transvr_obj->layout){
        case TRANSVR_TYPE_SFP:
            if (register_transvr_sfp_attr(device_p) < 0){
                goto err_reg_tvr_attr;
            }
            break;
        case TRANSVR_TYPE_QSFP:
        case TRANSVR_TYPE_QSFP_PLUS:
            if (register_transvr_qsfp_attr(device_p) < 0){
                goto err_reg_tvr_attr;
            }
            break;
        case TRANSVR_TYPE_QSFP_28:
            if (register_transvr_qsfp28_attr(device_p) < 0){
                goto err_reg_tvr_attr;
            }
            break;
        default:
            goto err_reg_tvr_attr;
    }
    return 0;

err_reg_tvr_attr:
    SWPS_ERR("%s: fail! type=%d \n", __func__, transvr_obj->type);
    return -1;
}


static int
register_ioexp_attr_sfp_1(struct device *device_p){
    /* Support machine type:
     * - SFP : Magnolia
     */
    char *err_attr = NULL;

    if (device_create_file(device_p, &dev_attr_present) < 0) {
           err_attr = "dev_attr_present";
        goto err_ioexp_sfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_tx_fault) < 0) {
        err_attr = "dev_attr_tx_fault";
        goto err_ioexp_sfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_rxlos) < 0) {
        err_attr = "dev_attr_rxlos";
        goto err_ioexp_sfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_tx_disable) < 0) {
        err_attr = "dev_attr_tx_disable";
         goto err_ioexp_sfp1_attr;
    }
    return 0;

err_ioexp_sfp1_attr:
    SWPS_ERR("Add device attribute:%s failure! \n",err_attr);
    return -1;
}


static int
register_ioexp_attr_sfp_2(struct device *device_p){
    /* Support machine type:
     * - SFP28 : Cypress
     */
    char *err_attr = NULL;

    if (register_ioexp_attr_sfp_1(device_p) < 0){
        goto err_ioexp_sfp2_attr;
    }
    if (device_create_file(device_p, &dev_attr_hard_rs0) < 0) {
        err_attr = "dev_attr_hard_rs0";
         goto err_ioexp_sfp2_attr;
    }
    if (device_create_file(device_p, &dev_attr_hard_rs1) < 0) {
        err_attr = "dev_attr_hard_rs1";
         goto err_ioexp_sfp2_attr;
    }
    return 0;

err_ioexp_sfp2_attr:
    SWPS_ERR("Add device attribute:%s failure! \n",err_attr);
    return -1;
}


static int
register_ioexp_attr_qsfp_1(struct device *device_p){
    /* Support machine type:
     * - QSFP  : Magnolia, Redwood, Hudson32i
     * - QSFP+ : Magnolia, Redwood, Hudson32i
     * - QSFP28: Redwood
     */
    char *err_attr = NULL;

    if (device_create_file(device_p, &dev_attr_present) < 0) {
           err_attr = "dev_attr_present";
        goto err_ioexp_qsfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_reset) < 0) {
           err_attr = "dev_attr_reset";
        goto err_ioexp_qsfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_lpmod) < 0) {
        err_attr = "dev_attr_lpmod";
        goto err_ioexp_qsfp1_attr;
    }
    if (device_create_file(device_p, &dev_attr_modsel) < 0) {
           err_attr = "dev_attr_modsel";
        goto err_ioexp_qsfp1_attr;
    }
    return 0;

err_ioexp_qsfp1_attr:
    SWPS_ERR("Add device attribute:%s failure! \n",err_attr);
    return -1;
}


static int
register_modctl_attr(struct device *device_p){

    char *err_msg = NULL;

    if (device_create_file(device_p, &dev_attr_platform) < 0) {
        err_msg = "dev_attr_platform";
        goto err_reg_modctl_attr;
    }
    if (device_create_file(device_p, &dev_attr_version) < 0) {
        err_msg = "dev_attr_version";
        goto err_reg_modctl_attr;
    }
    if (device_create_file(device_p, &dev_attr_status) < 0) {
        err_msg = "dev_attr_status";
        goto err_reg_modctl_attr;
    }
    if (device_create_file(device_p, &dev_attr_reset_i2c) < 0) {
        err_msg = "dev_attr_reset_i2c";
        goto err_reg_modctl_attr;
    }
    if (device_create_file(device_p, &dev_attr_reset_swps) < 0) {
        err_msg = "dev_attr_reset_swps";
        goto err_reg_modctl_attr;
    }
    if (device_create_file(device_p, &dev_attr_auto_config) < 0) {
        err_msg = "dev_attr_auto_config";
        goto err_reg_modctl_attr;
    }
    return 0;

err_reg_modctl_attr:
    SWPS_ERR("%s: %s\n", __func__, err_msg);
    return -1;
}


static int
register_ioexp_attr(struct device *device_p,
                  struct transvr_obj_s *transvr_obj){

    char *err_msg = "ERR";

    switch (transvr_obj->ioexp_obj_p->ioexp_type){
        case IOEXP_TYPE_MAGINOLIA_NAB:
        case IOEXP_TYPE_MAGINOLIA_4AB:
            if (register_ioexp_attr_sfp_1(device_p) < 0){
                err_msg = "register_ioexp_attr_sfp_1 fail";
                goto err_reg_ioexp_attr;
            }
            break;
        case IOEXP_TYPE_CYPRESS_NABC:
            if (register_ioexp_attr_sfp_2(device_p) < 0){
                err_msg = "register_ioexp_attr_sfp_2 fail";
                goto err_reg_ioexp_attr;
            }
            break;
        case IOEXP_TYPE_MAGINOLIA_7AB:
        case IOEXP_TYPE_SPRUCE_7AB:
        case IOEXP_TYPE_CYPRESS_7ABC:
        case IOEXP_TYPE_REDWOOD_P01P08:
        case IOEXP_TYPE_REDWOOD_P09P16:
        case IOEXP_TYPE_HUDSON32IGA_P01P08:
        case IOEXP_TYPE_HUDSON32IGA_P09P16:
        case IOEXP_TYPE_TAHOE_5A:
        case IOEXP_TYPE_TAHOE_6ABC:
        case IOEXP_TYPE_SEQUOIA_NABC:
        case IOEXP_TYPE_LAVENDER_P65:
            if (register_ioexp_attr_qsfp_1(device_p) < 0){
                err_msg = "register_ioexp_attr_qsfp_1 fail";
                goto err_reg_ioexp_attr;
            }
            break;

        default:
            err_msg = "Unknow type";
            goto err_reg_ioexp_attr;
    }
    return 0;

err_reg_ioexp_attr:
    SWPS_ERR("%s: %s <type>:%d \n",
            __func__, err_msg, transvr_obj->ioexp_obj_p->ioexp_type);
    return -1;
}


static int
register_modctl_device(void) {

    struct device *device_p = NULL;
    int minor_comm = 0; /* Default minor number for common device */
    dev_t dev_num  = MKDEV(ctl_major, minor_comm);
    char *err_msg  = "ERROR";

    device_p = device_create(swp_class_p,     /* struct class *cls     */
                             NULL,            /* struct device *parent */
                             dev_num,         /* dev_t devt            */
                             NULL,            /* void *private_data    */
                             SWP_DEV_MODCTL); /* const char *fmt       */
    if (IS_ERR(device_p)){
        err_msg = "device_create fail";
        goto err_register_modctl_device_1;
    }
    if (register_modctl_attr(device_p) < 0) {
        err_msg = "register_modctl_attr fail";
        goto err_register_modctl_device_2;
    }
    return 0;

err_register_modctl_device_2:
    device_unregister(device_p);
    device_destroy(swp_class_p, dev_num);
err_register_modctl_device_1:
    SWPS_ERR("%s: %s\n", __func__, err_msg);
    return -1;
}


static int
register_port_device(char *dev_name,
                     dev_t dev_num,
                     struct transvr_obj_s *transvr_obj){

    struct device *device_p = NULL;
    device_p = device_create(swp_class_p,   /* struct class *cls     */
                             NULL,          /* struct device *parent */
                             dev_num,       /* dev_t devt            */
                             transvr_obj,   /* void *private_data    */
                             dev_name);     /* const char *fmt       */
    if (IS_ERR(device_p)){
        goto err_regswp_create_dev;
    }
    if (register_transvr_attr(device_p, transvr_obj) < 0){
        goto err_regswp_reg_attr;
    }
    if (register_ioexp_attr(device_p, transvr_obj) < 0){
           goto err_regswp_reg_attr;
    }
    return 0;

err_regswp_reg_attr:
    device_unregister(device_p);
    device_destroy(swp_class_p, dev_num);
err_regswp_create_dev:
    SWPS_ERR("%s fail! <port>:%s\n", __func__, dev_name);
    return -1;
}


static int
register_swp_module(void){

    dev_t ctl_devt  = 0;
    dev_t port_devt = 0;
    int dev_total = port_total + 1; /* char_dev for module control */

    /* Register device number */
    if (alloc_chrdev_region(&ctl_devt, 0, 1, SWP_DEV_MODCTL) < 0){
        SWPS_WARN("Allocate CTL MAJOR failure! \n");
        goto err_register_swp_module_1;
    }
    if (alloc_chrdev_region(&port_devt, 0, dev_total, SWP_CLS_NAME) < 0){
        SWPS_WARN("Allocate PORT MAJOR failure! \n");
        goto err_register_swp_module_2;
    }
    ctl_major  = MAJOR(ctl_devt);
    port_major = MAJOR(port_devt);

    /* Create class object */
    swp_class_p = class_create(THIS_MODULE, SWP_CLS_NAME);
    if (IS_ERR(swp_class_p)) {
        SWPS_ERR("Create class failure! \n");
        goto err_register_swp_module_3;
    }
    return 0;

err_register_swp_module_3:
    unregister_chrdev_region(MKDEV(port_major, 0), port_total);
err_register_swp_module_2:
    unregister_chrdev_region(MKDEV(ctl_major, 0), 1);
err_register_swp_module_1:
    return -1;
}


/* ========== Module initial relate ==========
 */
static int
create_ioexp_objs(void) {

    int i, run_mod;

    /* Clean IOEXP object */
    clean_ioexp_objs();
    /* Get running mode */
    run_mod = IOEXP_MODE_DIRECT;
    if (SWP_POLLING_ENABLE){
        run_mod = IOEXP_MODE_POLLING;
    }
    /* Create IOEXP object */
    for(i=0; i<ioexp_total; i++){
        if (create_ioexp_obj(ioexp_layout[i].ioexp_id,
                             ioexp_layout[i].ioexp_type,
                             (ioexp_layout[i].addr),
                             run_mod) < 0) {
            goto err_initioexp_create_obj_1;
        }
    }
    return 0;

err_initioexp_create_obj_1:
    clean_ioexp_objs();
    return -1;
}


static int
create_port_objs(void) {

    int port_id, chan_id, ioexp_id, ioexp_virt_offset;
    int transvr_type, chipset_type, run_mod, i, j;
    int minor_curr = 0;
    int ok_count   = 0;
    int devlen_max = 31; // 32 - 1
    char dev_name[32] = "ERROR";
    char err_msg[64]  = "ERROR";
    struct transvr_obj_s* transvr_obj_p = NULL;
    struct ioexp_obj_s *ioexp_obj_p = NULL;
    struct device *dev_p = NULL;

    for (minor_curr=0; minor_curr<port_total; minor_curr++) {
        /* Get info from  port_layout[] */
        port_id           = port_layout[minor_curr].port_id;
        chan_id           = port_layout[minor_curr].chan_id;
        ioexp_id          = port_layout[minor_curr].ioexp_id;
        ioexp_virt_offset = port_layout[minor_curr].ioexp_offset;
        transvr_type      = port_layout[minor_curr].transvr_type;
        chipset_type      = port_layout[minor_curr].chipset_type;
        /* Get running mode */
        run_mod = TRANSVR_MODE_DIRECT;
        if (SWP_POLLING_ENABLE){
            run_mod = TRANSVR_MODE_POLLING;
        }
        /* Prepare device name */
        if (strlen(SWP_DEV_PORT) > devlen_max) {
            snprintf(err_msg, sizeof(err_msg),
                    "SWP_DEV_PORT too long!");
            goto err_initport_create_tranobj;
        }
        memset(dev_name, 0, sizeof(dev_name));
        snprintf(dev_name, devlen_max, "%s%d", SWP_DEV_PORT, port_id);
        /* Create transceiver object */
        ioexp_obj_p = get_ioexp_obj(ioexp_id);
        if (!ioexp_obj_p){
            snprintf(err_msg, sizeof(err_msg),
                    "IOEXP object:%d not exist", ioexp_id);
            goto err_initport_create_tranobj;
        }
        transvr_obj_p = create_transvr_obj(dev_name, chan_id, ioexp_obj_p,
                                           ioexp_virt_offset, transvr_type,
                                           chipset_type, run_mod);
        if (!transvr_obj_p){
            snprintf(err_msg, sizeof(err_msg),
                    "Create transceiver object fail <id>:%s", dev_name);
            goto err_initport_create_tranobj;
        }
        /* Setup Lane_ID mapping */
        i = ARRAY_SIZE(port_layout[minor_curr].lane_id);
        j = ARRAY_SIZE(transvr_obj_p->lane_id);
        if (i != j) {
            snprintf(err_msg, sizeof(err_msg),
                    "Lane_id size inconsistent %d/%d", i, j);
            goto err_initport_reg_device;
        }
        memcpy(transvr_obj_p->lane_id, port_layout[minor_curr].lane_id, i*sizeof(int));
        /* Create and register device object */
        if (register_port_device(dev_name, MKDEV(port_major, minor_curr), transvr_obj_p) < 0){
            snprintf(err_msg, sizeof(err_msg),
                    "register_port_device fail");
            goto err_initport_reg_device;
        }
        /* Setup device_ptr of transvr_obj */
        dev_p = get_swpdev_by_name(dev_name);
        if (!dev_p){
            snprintf(err_msg, sizeof(err_msg),
                    "get_swpdev_by_name fail");
            goto err_initport_reg_device;
        }
        transvr_obj_p->transvr_dev_p = dev_p;
        /* Success */
        ok_count++;
    }
    SWPS_INFO("%s: initialed %d port-dev",__func__, ok_count);
    return 0;

err_initport_reg_device:
    kfree(transvr_obj_p);
err_initport_create_tranobj:
    clean_port_obj();
    SWPS_ERR("%s: %s", __func__, err_msg);
    SWPS_ERR("Dump: <port_id>:%d <chan_id>:%d <ioexp_id>:%d <voffset>:%d <tvr_type>:%d <run_mod>:%d\n",
            port_id, chan_id, ioexp_id, ioexp_virt_offset, transvr_type, run_mod);
    return -1;
}


static int
init_dev_topology(void){

    int err;
    char *emsg = "ERR";
    flag_mod_state = SWP_STATE_NORMAL;

    err = init_ioexp_objs();
    switch(err){
        case 0:  /* Normal */
            SWPS_DEBUG("%s: normal case\n", __func__);
            break;

        case -1: /* topology error */
            SWPS_DEBUG("%s: detect tier-1 topology initial failure <err>:%d\n",
                       __func__, err);
            /* Reset and isolate */
            err = reset_i2c_topology();
            if (err < 0) {
                emsg = "reset i2c topology fail";
                goto err_init_dev_topology;
            }
            /* Re-initial again */
            err = init_ioexp_objs();
            if (err < 0) {
                emsg = "re-init ioexp objects fail";
                goto err_init_dev_topology;
            }
            break;

        case -2: /* Internal error */
            SWPS_DEBUG("%s: internal error case\n", __func__);
            err  = -2;
            emsg = "internal error";
            goto err_init_dev_topology;

        default:
            SWPS_DEBUG("%s: undefined error case\n", __func__);
            emsg = "undefined error case";
            goto err_init_dev_topology;
    }
    SWPS_DEBUG("%s: initial I2C topology success\n", __func__);
    return 0;

err_init_dev_topology:
    SWPS_ERR("%s: %s <err>:%d\n", __func__, emsg, err);
    return -1;
}


static int
init_polling_task(void){

    if (SWP_POLLING_ENABLE){
        schedule_delayed_work(&swp_polling, _get_polling_period());
    }
    return 0;
}


static int
init_swps_common(void){

    char *err_msg = "ERR";
    
    auto_config = 0;
    if ((SWP_AUTOCONFIG_ENABLE) && (SWP_POLLING_ENABLE)){
        auto_config = 1;
    }
    if (register_modctl_device() < 0) {
        err_msg = "register_modctl_device fail";
        goto err_init_swps_common_1;
    }
    if (_update_auto_config_2_trnasvr() < 0) {
        err_msg = "_update_auto_config_2_trnasvr fail";
        goto err_init_swps_common_1;
    }
    if (init_polling_task() < 0){
        err_msg = "init_polling_task fail";
        goto err_init_swps_common_1;
    }
    return 0;

err_init_swps_common_1:
    clean_swps_common();
    SWPS_ERR("%s: %s\n", __func__, err_msg);
    return -1;
}


static int __init
swp_module_init(void){

    if (get_platform_type() < 0){
        goto err_init_out;
    }
    if (get_layout_info() < 0){
        goto err_init_out;
    }
    if (register_swp_module() < 0){
        goto err_init_out;
    }
    if (create_ioexp_objs() < 0){
        goto err_init_ioexp;
    }
    if (create_port_objs() < 0){
        goto err_init_portobj;
    }
    if (init_mux_gpio(gpio_rest_mux) < 0){
        goto err_init_mux;
    }
    if (init_dev_topology() < 0){
        goto err_init_topology;
    }
    if (init_swps_common() < 0){
        goto err_init_topology;
    }
    SWPS_INFO("Inventec switch-port module V.%s initial success.\n", SWP_VERSION);
    return 0;


err_init_topology:
    clean_mux_gpio();
err_init_mux:
    clean_port_obj();
err_init_portobj:
    clean_ioexp_objs();
err_init_ioexp:
    class_unregister(swp_class_p);
    class_destroy(swp_class_p);
    unregister_chrdev_region(MKDEV(ctl_major, 0), 1);
    unregister_chrdev_region(MKDEV(port_major, 0), port_total);
err_init_out:
    SWPS_ERR("Inventec switch-port module V.%s initial failure.\n", SWP_VERSION);
    return -1;
}


static void __exit
swp_module_exit(void){

    clean_swps_common();
    clean_port_obj();
    clean_ioexp_objs();
    clean_mux_gpio();
    class_unregister(swp_class_p);
    class_destroy(swp_class_p);
    unregister_chrdev_region(MKDEV(ctl_major, 0), 1);
    unregister_chrdev_region(MKDEV(port_major, 0), port_total);
    SWPS_INFO("Remove Inventec switch-port module success.\n");
}


/*  Module information  */
MODULE_AUTHOR(SWP_AUTHOR);
MODULE_DESCRIPTION(SWP_DESC);
MODULE_VERSION(SWP_VERSION);
MODULE_LICENSE(SWP_LICENSE);

module_init(swp_module_init);
module_exit(swp_module_exit);









