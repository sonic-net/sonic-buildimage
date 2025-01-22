#include <asm/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include "io_expander.h"
#include "net_mux.h"

/* For build single module using (Ex: ONL platform) */
#include <linux/module.h>

static struct mux_obj_s *mux_head_p = NULL;

/* ========== MUX object functions ==========
 */
static int
_setup_i2c_value(struct mux_obj_s *self, int offset, int value){

    return i2c_smbus_write_byte_data(self->i2c_client_p, offset, value);
}


static int
_setup_i2c_client(struct mux_obj_s *self, int chan_id, int addr){

    struct i2c_adapter *adap  = NULL;
    char *emsg = "ERR";

    adap = i2c_get_adapter(chan_id);
    if (!adap){
        emsg = "can't get adapter";
        goto err_setup_i2c_client;
    }
    self->i2c_client_p = kzalloc(sizeof(*self->i2c_client_p), GFP_KERNEL);
    if (!self->i2c_client_p){
        emsg = "can't kzalloc client";
        goto err_setup_i2c_client;
    }
    self->i2c_client_p->adapter = adap;
    self->i2c_client_p->addr = addr;
    return 0;

err_setup_i2c_client:
    SWPS_ERR("%s: %s\n", __func__, emsg);
    return ERR_MUX_UNEXCPT;
}


int
_common_force_pull_gpio(int mem_addr,
                        int input,
                        int bit_offset){

    unsigned int val  = 0;
    unsigned int targ = 0;

    /* Get current value */
    val = inl(mem_addr);
    if (val == 0) {
        SWPS_ERR("%s: inl:%d fail!\n", __func__, val);
        return -1;
    }
    /* Count target value */
    switch (input) {
        case 0: /* Pull Low */
            targ = (val & (~(1 << bit_offset)));
            break;
        case 1: /* Pull high */
            targ = (val | (1 << bit_offset));
            break;
        default:
            SWPS_ERR("%s: input state:%d incorrect!\n",
                    __func__, input);
            return -1;
    }
    /* Setup gpio */
    outl(targ, mem_addr);
    if (targ != inl(mem_addr)){
        SWPS_ERR("%s: outl:%d fail!\n", __func__, targ);
        return -1;
    }
    SWPS_DEBUG("%s: done.\n", __func__);
    return 0;
}


int
rangeley_force_pull_high(struct mux_obj_s *self){
    SWPS_ERR("%s: not ready!\n", __func__);
    return -1;
}


int
rangeley_force_pull_low(struct mux_obj_s *self){
    SWPS_ERR("%s: not ready!\n", __func__);
    return -1;
}


int
hedera_force_pull_high(struct mux_obj_s *self){
    return _common_force_pull_gpio(MUX_RST_MEM_ADDR_HEDERA, 1, 5);
}


int
hedera_force_pull_low(struct mux_obj_s *self){
    return _common_force_pull_gpio(MUX_RST_MEM_ADDR_HEDERA, 0, 5);
}


int
normal_gpio_pull_high(struct mux_obj_s *self){
    return gpio_direction_output(self->gpio_num, 1);
}


int
normal_gpio_pull_low(struct mux_obj_s *self){
    return gpio_direction_output(self->gpio_num, 0);
}


int
cpld_rst_all_4_pull_low(struct mux_obj_s *self){

    char *emsg = "ERR";
    int   err  = ERR_MUX_UNEXCPT;

    switch(self->gpio_num) {
        case MUX_RST_CPLD_C0_A77_70_74_RST_ALL:
            goto setlow_cpld_rst_all_4_c0_a77_70_74_rst_all;

        default:
            break;
    }
    emsg = "Undefined case";
    goto err_cpld_rst_all_4_pull_low;

setlow_cpld_rst_all_4_c0_a77_70_74_rst_all:
    err = _setup_i2c_value(self, 0x70, 0x0);
    if (err < 0) {
        emsg = "setup 0x70 fail";
        goto err_cpld_rst_all_4_pull_low;
    }
    err = _setup_i2c_value(self, 0x74, 0x01);
    if (err < 0) {
        emsg = "setup 0x74 fail";
        goto err_cpld_rst_all_4_pull_low;
    }
    return 0;

err_cpld_rst_all_4_pull_low:
    SWPS_INFO("%s: %s <type>:%d <err>:%d\n",
            __func__, emsg, self->gpio_num, err);
    return ERR_MUX_UNEXCPT;
}


int
cpld_rst_all_4_pull_high(struct mux_obj_s *self){

    char *emsg = "ERR";
    int   err  = ERR_MUX_UNEXCPT;

    switch(self->gpio_num) {
        case MUX_RST_CPLD_C0_A77_70_74_RST_ALL:
            goto sethigh_cpld_rst_all_4_c0_a77_70_74_rst_all;

        default:
            break;
    }
    emsg = "Undefined case";
    goto err_cpld_rst_all_4_pull_high;

sethigh_cpld_rst_all_4_c0_a77_70_74_rst_all:
    err = _setup_i2c_value(self, 0x70, 0xfe);
    if (err < 0) {
        emsg = "setup 0x70 fail";
        goto err_cpld_rst_all_4_pull_high;
    }
    err = _setup_i2c_value(self, 0x74, 0x03);
    if (err < 0) {
        emsg = "setup 0x74 fail";
        goto err_cpld_rst_all_4_pull_high;
    }
    return 0;

err_cpld_rst_all_4_pull_high:
    SWPS_INFO("%s: %s <type>:%d <err>:%d\n",
            __func__, emsg, self->gpio_num, err);
    return ERR_MUX_UNEXCPT;
}


int
pca9548_reset_mux_all(struct mux_obj_s *self){
    /* [Note] Power-on reset (PCA9548A-NXP)
     *     When power is applied to VDD, an internal Power-On Reset (POR)
     *     holds the PCA9548A in a reset condition until VDD has reached
     *     VPOR. At this point, the reset condition is released and the
     *     PCA9548A register and I2C-bus state machine are initialized to
     *     their default states (all zeroes) causing all the channels to
     *     be deselected. Thereafter, VDD must be lowered below 0.2 V for
     *     at least 5 us in order to reset the device.
     */
    if (self->_pull_low(self) < 0) {
        SWPS_ERR("%s: _pull_low fail!\n", __func__);
        return -1;
    }
    mdelay(MUX_RST_WAIT_MS_PCA9548);
    if (self->_pull_high(self) < 0) {
        SWPS_ERR("%s: _pull_high fail!\n", __func__);
        return -1;
    }
    mdelay(MUX_RST_WAIT_MS_PCA9548);
    return 0;
}


int
cpld_reset_mux_all(struct mux_obj_s *self){

    char *emsg = "ERR";
    int   err  = ERR_MUX_UNEXCPT;

    switch(self->gpio_num) {
        case MUX_RST_CPLD_C0_A77_70_74_RST_ALL:
            goto reset_cpld_rst_all_4_c0_a77_70_74_rst_all;

        default:
            break;
    }
    emsg = "Undefined case";
    goto err_cpld_reset_mux_all;
    
reset_cpld_rst_all_4_c0_a77_70_74_rst_all:
    if (self->_pull_low(self) < 0) {
        emsg = "_pull_low fail";
        goto err_cpld_reset_mux_all;
    }
    mdelay(MUX_RST_WAIT_MS_CPLD);
    return 0;
    
err_cpld_reset_mux_all:
    SWPS_INFO("%s: %s <type>:%d <err>:%d\n",
            __func__, emsg, self->gpio_num, err);
    return ERR_MUX_UNEXCPT;
}


int
common_reset_mux_all(struct mux_obj_s *self){
    SWPS_ERR("%s: not ready!\n", __func__);
    return -1;
}


int
init_gpio_4_force(struct mux_obj_s *self){

    if (self->_pull_high(self) < 0) {
        SWPS_ERR("%s: setup default fail!\n", __func__);
        return -1;
    }
    return 0;
}


int
init_gpio_4_normal(struct mux_obj_s *self){

    int   err  = 0;
    char *emsg = "ERR";

    if (!gpio_is_valid(self->gpio_num)) {
        emsg = "GPIO invalid";
        goto err_init_gpio_4_normal;
    }
    err = gpio_request(self->gpio_num, MUX_GPIO_LABEL);
    if (err < 0) {
        emsg = "gpio_request fail";
        goto err_init_gpio_4_normal;
    }
    err = self->_pull_high(self);
    if (err < 0) {
        emsg = "setup default fail";
        goto err_init_gpio_4_normal;
    }
    SWPS_DEBUG("%s: gpio_request:%d ok.\n", __func__, self->gpio_num);
    return 0;

err_init_gpio_4_normal:
    SWPS_ERR("%s: %s <gpio>:%d <err>:%d\n",
             __func__, emsg, self->gpio_num, err);
    return -1;
}


int
init_cpld_4_rst_all(struct mux_obj_s *self){

    char *emsg = "ERR";
    int err  = ERR_MUX_UNEXCPT;
    int chan = ERR_MUX_UNEXCPT;
    int addr = ERR_MUX_UNEXCPT;

    switch(self->gpio_num) {
        case MUX_RST_CPLD_C0_A77_70_74_RST_ALL:
            goto init_cpld_i2c_c0_a77_70_74_rst_all;

        default:
            break;
    }
    emsg = "Undefined case";
    goto err_init_cpld_4_rst_all;

init_cpld_i2c_c0_a77_70_74_rst_all:
    chan = 0;
    addr = 0x77;
    err = _setup_i2c_client(self, chan, addr);
    if (err < 0) {
        emsg = "_setup_i2c_client fail";
        goto err_init_cpld_4_rst_all;
    }
    err = self->_pull_high(self);
    if (err < 0) {
        emsg = "setup default value fail";
        goto err_init_cpld_4_rst_all;
    }
    SWPS_DEBUG("%s: init_cpld_i2c_c0_a77_70_74_rst_all ok", __func__);
    return 0;

err_init_cpld_4_rst_all:
    SWPS_INFO("%s: %s <type>:%d <err>:%d\n",
             __func__, emsg, self->gpio_num, err);
    return ERR_MUX_UNEXCPT;
}


int
clean_gpio_4_common(struct mux_obj_s *self){

    if (!self) return 0;
    if (!gpio_is_valid(self->gpio_num)) return 0;
    self->_pull_high(self);
    gpio_free(mux_head_p->gpio_num);
    return 0;
}


int
clean_cpld_4_rst_all(struct mux_obj_s *self){

    if (!self) return 0;
    self->_pull_high(self);
    if (self->i2c_client_p) {
        i2c_put_adapter(self->i2c_client_p->adapter);
        kfree(self->i2c_client_p);
    }
    return 0;
}


static int
_setup_muxctl_cb(struct mux_obj_s *self,
                 unsigned gpio){

    char mod_dsc[32] = "ERR";

    switch (gpio) {
        case MUX_RST_GPIO_FORCE_RANGELEY:
            self->gpio_num   = gpio;
            self->_pull_low  = rangeley_force_pull_low;
            self->_pull_high = rangeley_force_pull_high;
            self->_init      = init_gpio_4_force;
            self->_clean     = clean_gpio_4_common;
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Rangeley force mode");
            goto ok_setup_muxctl_cb;

        case MUX_RST_GPIO_FORCE_HEDERA:
            self->gpio_num   = gpio;
            self->_pull_low  = hedera_force_pull_low;
            self->_pull_high = hedera_force_pull_high;
            self->_init      = init_gpio_4_force;
            self->_clean     = clean_gpio_4_common;
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Hedera force mode");
            goto ok_setup_muxctl_cb;

        case MUX_RST_GPIO_48_PCA9548:
        case MUX_RST_GPIO_69_PCA9548:
        case MUX_RST_GPIO_249_PCA9548:
        case MUX_RST_GPIO_500_PCA9548:
        case MUX_RST_GPIO_505_PCA9548:
            self->gpio_num   = gpio;
            self->_pull_low  = normal_gpio_pull_low;
            self->_pull_high = normal_gpio_pull_high;
            self->_init      = init_gpio_4_normal;
            self->_clean     = clean_gpio_4_common;
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Normal mode <gpio>:%d", (int)gpio);
            goto ok_setup_muxctl_cb;

        case MUX_RST_CPLD_C0_A77_70_74_RST_ALL:
            self->gpio_num   = gpio;
            self->_pull_low  = cpld_rst_all_4_pull_low;
            self->_pull_high = cpld_rst_all_4_pull_high;
            self->_init      = init_cpld_4_rst_all;
            self->_clean     = clean_cpld_4_rst_all;
            self->reset      = cpld_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "CPLD mode <type>:%d", (int)gpio);
            goto ok_setup_muxctl_cb;

        default:
            break;
    }
    SWPS_ERR("%s: Unexpected GPIO:%d\n", __func__, gpio);
    return -1;

ok_setup_muxctl_cb:
    SWPS_INFO("muxctl: %s.\n", mod_dsc);
    return 0;
}


/* ========== MUX public functions ==========
 */
void
clean_mux_objs(void){

    struct mux_obj_s *curr_p = mux_head_p;
    struct mux_obj_s *next_p = NULL;

    if (!curr_p) {
        SWPS_DEBUG("%s: mux_head_p is NULL\n", __func__);
        return;
    }
    while (curr_p) {
        next_p = curr_p->next;
        curr_p->_clean(curr_p);
        kfree(curr_p);
        curr_p = next_p;
    }
    SWPS_DEBUG("%s: done.\n", __func__);
}
EXPORT_SYMBOL(clean_mux_objs);


int
reset_mux_objs(void){

    if (!mux_head_p) {
        SWPS_ERR("%s: MUX ctl object doesn't exist!\n", __func__);
        return -1;
    }
    if (mux_head_p->reset(mux_head_p) < 0){
        SWPS_ERR("%s: reset fail!\n", __func__);
        return -1;
    }
    return 0;
}
EXPORT_SYMBOL(reset_mux_objs);


struct mux_obj_s *
_create_mux_obj(unsigned gpio){

    char *emsg = "ERR";
    struct mux_obj_s *obj_p = NULL;

    obj_p = kzalloc(sizeof(struct mux_obj_s), GFP_KERNEL);
    if (!obj_p) {
        emsg = "kzalloc fail!";
        goto err_create_mux_obj_1;
    }
    if (_setup_muxctl_cb(obj_p, gpio) < 0){
        emsg = "_setup_muxctl_cb fail!";
        goto err_create_mux_obj_2;
    }
    if (obj_p->_init(obj_p) < 0) {
        emsg = "_init() fail!";
        goto err_create_mux_obj_2;
    }
    SWPS_DEBUG("%s: created MUX object <id>:%d\n", __func__, gpio);
    return obj_p;

err_create_mux_obj_2:
    kfree(obj_p);
err_create_mux_obj_1:
    SWPS_ERR("%s: %s <type>:%d\n", __func__, emsg, gpio);
    return NULL;
}


int
init_mux_objs(unsigned gpio){

    struct mux_obj_s *curr_p = NULL;
    char  *emsg = "ERR";

    /* Create MUX control object */
    if (mux_head_p) {
        SWPS_DEBUG("%s: mux_head_p is not NULL!\n", __func__);
        clean_mux_objs();
    }
    /* Currently, it is using single muxctl architecture.
     * In the future, it may use the multi-muxctl.
     * (Ex: Aurora 610's advance I2C control)
     */
    curr_p = _create_mux_obj(gpio);
    if (!curr_p) {
        emsg = "_create_mux_obj fail";
        goto err_init_mux_objs;
    }
    curr_p->next = NULL;
    mux_head_p = curr_p;
    SWPS_DEBUG("%s: all done. <type>:%d\n", __func__, gpio);
    return 0;

err_init_mux_objs:
    clean_mux_objs();
    SWPS_ERR("%s: %s\n", __func__, emsg);
    return -1;
}
EXPORT_SYMBOL(init_mux_objs);


/* For single ko module
 * => You need to declare MODULE_LICENSE If you want to build single module along.
 * => Ex: For ONL platform
 */
MODULE_LICENSE("GPL");




