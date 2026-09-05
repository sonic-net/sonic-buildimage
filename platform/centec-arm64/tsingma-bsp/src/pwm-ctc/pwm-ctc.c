/*
 * Centec PWM driver
 *
 * Author: wangyb <wangyb@centecnetworks.com>
 *
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "../pinctrl-ctc/pinctrl-ctc.h"
#include "../pinctrl-ctc/core.h"
#include "../include/sysctl.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/pinctrl/consumer.h>

#define CTC_NUM_PWM			4
#define CTC_CR_PWM  		0x0
#define CTC_DUTY_PWM 		0x4

#define CTC_MAX_PERIOD_PWM	0xFFFFFF
#define CTC_MAX_DUTY_PWM  	0xFFFFFF

#define CTC_PWM_ENABLE 		0x80000000

#define CTC_PERIOD_TACH		0x0
#define CTC_DUTY_TACH		0x4

struct ctc_pwm_chip {
	void __iomem *base;
	struct regmap *regmap_base;
	struct pwm_chip chip;
};

static inline struct ctc_pwm_chip *to_ctc_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct ctc_pwm_chip, chip);
}

static inline void ctc_pwm_writel(struct ctc_pwm_chip *chip, unsigned int num,
				  unsigned long offset, unsigned long val)
{
	regmap_write(chip->regmap_base,
		     offsetof(struct SysCtl_regs,
			      SysPwmCtl) + offset + num * 0x8, val);
}

static inline u32 ctc_pwm_readl(struct ctc_pwm_chip *chip, unsigned int num,
				unsigned long offset)
{
	u32 val;
	regmap_read(chip->regmap_base,
		    offsetof(struct SysCtl_regs,
			     SysPwmCtl) + offset + num * 0x8, &val);
	return val;
}

static inline u32 ctc_tach_readl(struct ctc_pwm_chip *chip, unsigned int num,
				 unsigned long offset)
{
	u32 val;
	regmap_read(chip->regmap_base,
		    offsetof(struct SysCtl_regs,
			     SysTachLog) + offset + num * 0x8, &val);
	return val;
}

static int ctc_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			  int duty_ns, int period_ns)
{
	struct ctc_pwm_chip *pc = to_ctc_pwm_chip(chip);
	u32 cur_value;
	u32 duty_cycle = 0;
	u32 period_cycle = 0;

	duty_cycle = duty_ns / 1000;
	period_cycle = period_ns / 1000;

	if (duty_cycle < 0 || period_cycle < 1)
		return -1;

	if (duty_cycle == 0) {
		duty_cycle = 1;
	} else if (duty_cycle == period_cycle) {
		duty_cycle = duty_cycle - 1;
	}

	/* duty cycle */
	duty_cycle = duty_cycle & CTC_MAX_DUTY_PWM;
	ctc_pwm_writel(pc, pwm->hwpwm, CTC_DUTY_PWM, duty_cycle);

	/* period cycle */
	period_cycle = period_cycle & CTC_MAX_PERIOD_PWM;
	cur_value = ctc_pwm_readl(pc, pwm->hwpwm, CTC_CR_PWM);
	cur_value &= ~(CTC_MAX_PERIOD_PWM);
	cur_value |= period_cycle << 0;
	ctc_pwm_writel(pc, pwm->hwpwm, CTC_CR_PWM, cur_value);

	return 0;
}

static int ctc_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ctc_pwm_chip *pc = to_ctc_pwm_chip(chip);
	u32 cur_value;

	cur_value = ctc_pwm_readl(pc, pwm->hwpwm, CTC_CR_PWM);
	cur_value |= 1 << 31;
	ctc_pwm_writel(pc, pwm->hwpwm, CTC_CR_PWM, cur_value);
	return 0;
}

static void ctc_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ctc_pwm_chip *pc = to_ctc_pwm_chip(chip);
	u32 cur_value;

	cur_value = ctc_pwm_readl(pc, pwm->hwpwm, CTC_CR_PWM);
	cur_value &= ~(1 << 31);

	ctc_pwm_writel(pc, pwm->hwpwm, CTC_CR_PWM, cur_value);
}

static int ctc_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			  const struct pwm_state *state)
{
	int err;

	if (state->polarity != PWM_POLARITY_NORMAL)
		return -EINVAL;

	if (!state->enabled)
	{
		if(pwm->state.enabled)
			ctc_pwm_disable(chip,pwm);
		return 0;
	}

	err = ctc_pwm_config(pwm->chip, pwm, state->duty_cycle, state->period);
	if (err)
		return err;
	
	if (!pwm->state.enabled)
		err = ctc_pwm_enable(chip, pwm);
	
	return err;
}


static int ctc_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			      struct pwm_state *state)
{
	struct ctc_pwm_chip *pc = to_ctc_pwm_chip(chip);
	u32 cur_value;
	u32 cur_value_2;

	cur_value = ctc_pwm_readl(pc, pwm->hwpwm, CTC_CR_PWM);

	if (cur_value & CTC_PWM_ENABLE)
		state->enabled = true;
	else
		state->enabled = false;

	state->polarity = PWM_POLARITY_NORMAL;

	state->period = (cur_value & (~CTC_PWM_ENABLE)) * 1000;	// in nanoseconds

	cur_value_2 = ctc_pwm_readl(pc, pwm->hwpwm, CTC_DUTY_PWM);

	state->duty_cycle = cur_value_2 * 1000;	// in nanoseconds

	return 0;
}

static int ctc_pwm_capture(struct pwm_chip *chip, struct pwm_device *pwm,
			   struct pwm_capture *result, unsigned long timeout)
{
	struct ctc_pwm_chip *pc = to_ctc_pwm_chip(chip);
	u32 period_tach;
	u32 duty_tach;

	period_tach = ctc_tach_readl(pc, pwm->hwpwm, CTC_PERIOD_TACH) / 4;
	result->period = period_tach * 1000;	// in nanoseconds

	duty_tach = ctc_tach_readl(pc, pwm->hwpwm, CTC_DUTY_TACH) / 4;
	result->duty_cycle = duty_tach * 1000;	// in nanoseconds

	return 0;
}

static const struct pwm_ops ctc_pwm_ops = {
	.apply = ctc_pwm_apply,
	//.enable = ctc_pwm_enable,
	//.disable = ctc_pwm_disable,
	.get_state = ctc_pwm_get_state,
	.capture = ctc_pwm_capture,
	.owner = THIS_MODULE,
};

static int ctc_pwm_probe(struct platform_device *pdev)
{
	struct pinctrl_dev *pctldev;
	struct ctc_pwm_chip *pc;
	struct pinctrl_state *state;

	int ret;
	int i;
	u32 cur_value;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	pc->regmap_base = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							  "ctc,sysctrl");
	pctldev = devm_kzalloc(&pdev->dev, sizeof(*pctldev), GFP_KERNEL);
	if (!pctldev) {
		return -1;
	}
	pctldev->p = pinctrl_get(&pdev->dev);
	state = pinctrl_lookup_state(pctldev->p, PINCTRL_STATE_DEFAULT);
	pinctrl_select_state(pctldev->p, state);
	printk("Select PWM Function\n");

	for (i = 0; i < CTC_NUM_PWM; i++) {
		cur_value = ctc_pwm_readl(pc, i, CTC_CR_PWM);
		cur_value |= 1 << 31;
		ctc_pwm_writel(pc, i, CTC_CR_PWM, cur_value);
	}

	platform_set_drvdata(pdev, pc);
	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &ctc_pwm_ops;
	pc->chip.base = -1;
	pc->chip.npwm = CTC_NUM_PWM;

	ret = pwmchip_add(&pc->chip);
	if (ret < 0)
		return -1;

	return 0;
}

static void ctc_pwm_remove(struct platform_device *pdev)
{
	struct ctc_pwm_chip *pc = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < CTC_NUM_PWM; i++)
		pwm_disable(&pc->chip.pwms[i]);

	pwmchip_remove(&pc->chip);
}

static const struct of_device_id ctc_pwm_of_match[] = {
	{.compatible = "centec-pwm"},
	{}
};

MODULE_DEVICE_TABLE(of, ctc_pwm_of_match);

static struct platform_driver ctc_pwm_driver = {
	.driver = {
		   .name = "ctc-pwm",
		   .of_match_table = ctc_pwm_of_match,
		   },
	.probe = ctc_pwm_probe,
	.remove_new = ctc_pwm_remove,
};

module_platform_driver(ctc_pwm_driver);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:centec-pwm");
