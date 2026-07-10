/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include "io_expander.h"
#include "inv_mux.h"

static struct mux_obj_s *mux_head_p = NULL;


/* ========== MUX object functions ==========
 */
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
    mdelay(MUX_RST_WAIT_MS);
    if (self->_pull_high(self) < 0) {
        SWPS_ERR("%s: _pull_high fail!\n", __func__);
        return -1;
    }
    mdelay(MUX_RST_WAIT_MS);
    return 0;
}


int
common_reset_mux_all(struct mux_obj_s *self){
    SWPS_ERR("%s: not ready!\n", __func__);
    return -1;
}


int
init_gpio_4_force(struct mux_obj_s *self){
    return 0;
}


int
init_gpio_4_normal(struct mux_obj_s *self){

    int err = 0;

    if (!gpio_is_valid(self->gpio_num)) {
        SWPS_ERR("%s: GIPO:%d isn't valid\n", __func__, self->gpio_num);
        return -1;
    }
    err = gpio_request(self->gpio_num, MUX_GPIO_LABEL);
    if (err < 0) {
        SWPS_ERR("%s: gpio_request fail <err>:%d <gpio>:%d\n",
                 __func__, err, self->gpio_num);
        return -1;
    }
    SWPS_DEBUG("%s: gpio_request:%d ok.\n", __func__, self->gpio_num);
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
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Rangeley force mode");
            goto ok_setup_muxctl_cb;

        case MUX_RST_GPIO_FORCE_HEDERA:
            self->gpio_num   = gpio;
            self->_pull_low  = hedera_force_pull_low;
            self->_pull_high = hedera_force_pull_high;
            self->_init      = init_gpio_4_force;
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Hedera force mode");
            goto ok_setup_muxctl_cb;

        case MUX_RST_GPIO_48_PAC9548:
        case MUX_RST_GPIO_69_PAC9548:
        case MUX_RST_GPIO_249_PCA9548:
        case MUX_RST_GPIO_505_PCA9548:
            self->gpio_num   = gpio;
            self->_pull_low  = normal_gpio_pull_low;
            self->_pull_high = normal_gpio_pull_high;
            self->_init      = init_gpio_4_normal;
            self->reset      = pca9548_reset_mux_all;
            memset(mod_dsc, 0, 32);
            snprintf(mod_dsc, 31, "Normal mode <gpio>:%d", (int)gpio);
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
clean_mux_gpio(void){

    if (!mux_head_p) {
        SWPS_DEBUG("%s: mux_head_p is NULL\n", __func__);
        return;
    }
    if (gpio_is_valid(mux_head_p->gpio_num)) {
        gpio_free(mux_head_p->gpio_num);
    }
    kfree(mux_head_p);
    mux_head_p = NULL;
    SWPS_DEBUG("%s: done.\n", __func__);
}


int
reset_mux_gpio(void){

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


int
init_mux_gpio(unsigned gpio){

    /* Create MUX control object */
    if (mux_head_p) {
        SWPS_DEBUG("%s: mux_head_p is not NULL!\n", __func__);
        clean_mux_gpio();
    }
    /* Currently, it is using single muxctl architecture.
     * In the future, it may use the multi-muxctl if HW add new features.
     * (Ex: Port power-status control)
     */
    mux_head_p = kzalloc(sizeof(struct mux_obj_s), GFP_KERNEL);
    if (!mux_head_p) {
        SWPS_ERR("%s: kzalloc fail!\n", __func__);
        return -1;
    }
    /* Initial MUX controller */
    if (_setup_muxctl_cb(mux_head_p, gpio) < 0){
        SWPS_ERR("%s: _setup_muxctl_cb fail!\n", __func__);
        return -1;
    }
    if (mux_head_p->_init(mux_head_p) < 0) {
        SWPS_ERR("%s: init() fail\n", __func__);
        goto err_init_mux_gpio;
    }
    /* Setup default value */
    if (mux_head_p->_pull_high(mux_head_p) < 0) {
        SWPS_ERR("%s: setup default fail!\n", __func__);
        goto err_init_mux_gpio;
    }
    return 0;

err_init_mux_gpio:
    clean_mux_gpio();
    return -1;
}





