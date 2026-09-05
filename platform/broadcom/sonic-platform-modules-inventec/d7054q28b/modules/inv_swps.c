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

static int port_major;
static int ioexp_total;
static int port_total;
static struct class *swp_class_p = NULL;
static struct inv_platform_s *platform_p = NULL;
static struct inv_ioexp_layout_s *ioexp_layout = NULL;
static struct inv_port_layout_s *port_layout = NULL;

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
                                           (const void *)__swp_match);
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

/* ========== IO Expander attribute: from expander ==========
 */
static DEVICE_ATTR(present,         S_IRUGO,         show_attr_present,         NULL);
static DEVICE_ATTR(tx_fault,        S_IRUGO,         show_attr_tx_fault,        NULL);
static DEVICE_ATTR(rxlos,           S_IRUGO,         show_attr_rxlos,           NULL);
static DEVICE_ATTR(tx_disable,      S_IRUGO|S_IWUSR, show_attr_tx_disable,      store_attr_tx_disable);
static DEVICE_ATTR(reset,           S_IRUGO|S_IWUSR, show_attr_reset,           store_attr_reset);
static DEVICE_ATTR(lpmod,           S_IRUGO|S_IWUSR, show_attr_lpmod,           store_attr_lpmod);
static DEVICE_ATTR(modsel,          S_IRUGO|S_IWUSR, show_attr_modsel,          store_attr_modsel);

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
            kfree(transvr_obj_p);
        }
        dev_num = MKDEV(port_major, minor_curr);
        device_unregister(device_p);
        device_destroy(swp_class_p, dev_num);
    }
    SWPS_DEBUG("%s: done.\n", __func__);
}


static int
get_platform_type(void){

    char log_msg[64] = "ERROR";

    platform_p = kzalloc(sizeof(struct inv_platform_s), GFP_KERNEL);
    if (!platform_p){
        snprintf(log_msg, sizeof(log_msg), "kzalloc fail");
        goto err_get_platform_type_1;
    }
    platform_p->id = PLATFORM_SETTINGS;
    memset(platform_p->name, 0, sizeof(platform_p->name));
	snprintf(platform_p->name, (sizeof(platform_p->name) - 1),
			"%s", platform_map.name);
	snprintf(log_msg, sizeof(log_msg),
			"User setup platform: %d (%s)",
			platform_p->id, platform_p->name);
    SWPS_DEBUG("%s: %s, <conf>:%d\n", __func__, log_msg, PLATFORM_SETTINGS);
    return 0;

err_get_platform_type_1:
    SWPS_ERR("%s: %s <conf>:%d\n", __func__, log_msg, PLATFORM_SETTINGS);
    return -1;
}


static int
get_layout_info(void){
	ioexp_layout  = cypress_ga2_ioexp_layout;
    port_layout   = cypress_ga2_port_layout;
    ioexp_total   = ARRAY_SIZE(cypress_ga2_ioexp_layout);
    port_total    = ARRAY_SIZE(cypress_ga2_port_layout);

    SWPS_INFO("Start to initial platform: %d (%s)\n",
              platform_p->id, platform_p->name);
    return 0;
}

/* ========== Functions for register something ==========
 */

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
register_ioexp_attr(struct device *device_p,
                  struct transvr_obj_s *transvr_obj){

    char *err_msg = "ERR";

    switch (transvr_obj->ioexp_obj_p->ioexp_type){
        case IOEXP_TYPE_CYPRESS_NABC:
            if (register_ioexp_attr_sfp_2(device_p) < 0){
                err_msg = "register_ioexp_attr_sfp_2 fail";
                goto err_reg_ioexp_attr;
            }
            break;
        case IOEXP_TYPE_CYPRESS_7ABC:
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

    dev_t port_devt = 0;
    int dev_total = port_total + 1; /* char_dev for module control */

    if (alloc_chrdev_region(&port_devt, 0, dev_total, SWP_CLS_NAME) < 0){
        SWPS_WARN("Allocate PORT MAJOR failure! \n");
        goto err_register_swp_module_3;
    }
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
    if (init_ioexp_objs() < 0){
        goto err_init_portobj;
    }
    SWPS_INFO("Inventec switch-port module V.%s initial success.\n", SWP_VERSION);
    return 0;


err_init_portobj:
    clean_ioexp_objs();
err_init_ioexp:
    class_unregister(swp_class_p);
    class_destroy(swp_class_p);
    unregister_chrdev_region(MKDEV(port_major, 0), port_total);
err_init_out:
    SWPS_ERR("Inventec switch-port module V.%s initial failure.\n", SWP_VERSION);
    return -1;
}


static void __exit
swp_module_exit(void){
    clean_port_obj();
    clean_ioexp_objs();
    class_unregister(swp_class_p);
    class_destroy(swp_class_p);
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






