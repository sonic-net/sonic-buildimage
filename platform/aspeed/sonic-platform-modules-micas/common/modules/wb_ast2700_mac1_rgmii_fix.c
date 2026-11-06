// SPDX-License-Identifier: GPL-2.0
/*
 * Board-specific AST2700 MAC1 RGMII delay fix for Micas M2-W6950-128OC.
 *
 * The upstream AST2700 ftgmac100 path rewrites the MAC1 TX delay field when
 * tx/rx-internal-delay-ps are not explicitly provided. This board relies on
 * the firmware-programmed default delay and loses OOB connectivity when the
 * field is cleared. Restore the working MAC1 TX delay value after boot.
 */

#include <linux/bitfield.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>

#define WB_AST2700_MAC01_CLK_DLY_REG        0x390
#define WB_AST2700_MAC1_TX_DLY_MASK         GENMASK(11, 6)
#define WB_AST2700_MAC1_TX_DLY_WORKING_VAL  4

static int __init wb_ast2700_mac1_rgmii_fix_init(void)
{
	struct device_node *scu_np;
	struct regmap *scu_regmap;
	unsigned int regval;
	int ret;

	if (!of_machine_is_compatible("micas,m2-w6950-128oc"))
		return 0;

	scu_np = of_find_compatible_node(NULL, NULL, "aspeed,ast2700-scu1");
	if (!scu_np) {
		pr_err("wb_ast2700_mac1_rgmii_fix: failed to find ast2700-scu1 node\n");
		return -ENODEV;
	}

	scu_regmap = syscon_node_to_regmap(scu_np);
	of_node_put(scu_np);
	if (IS_ERR(scu_regmap)) {
		pr_err("wb_ast2700_mac1_rgmii_fix: failed to get scu regmap: %ld\n",
		       PTR_ERR(scu_regmap));
		return PTR_ERR(scu_regmap);
	}

	ret = regmap_read(scu_regmap, WB_AST2700_MAC01_CLK_DLY_REG, &regval);
	if (ret) {
		pr_err("wb_ast2700_mac1_rgmii_fix: failed to read delay reg: %d\n", ret);
		return ret;
	}

	pr_info("wb_ast2700_mac1_rgmii_fix: MAC01_CLK_DLY before fix: 0x%08x\n",
		regval);

	ret = regmap_update_bits(scu_regmap, WB_AST2700_MAC01_CLK_DLY_REG,
				 WB_AST2700_MAC1_TX_DLY_MASK,
				 FIELD_PREP(WB_AST2700_MAC1_TX_DLY_MASK,
					    WB_AST2700_MAC1_TX_DLY_WORKING_VAL));
	if (ret) {
		pr_err("wb_ast2700_mac1_rgmii_fix: failed to update MAC1 TX delay: %d\n",
		       ret);
		return ret;
	}

	ret = regmap_read(scu_regmap, WB_AST2700_MAC01_CLK_DLY_REG, &regval);
	if (ret) {
		pr_err("wb_ast2700_mac1_rgmii_fix: failed to read back delay reg: %d\n",
		       ret);
		return ret;
	}

	pr_info("wb_ast2700_mac1_rgmii_fix: MAC01_CLK_DLY after fix: 0x%08x\n",
		regval);

	return 0;
}

static void __exit wb_ast2700_mac1_rgmii_fix_exit(void)
{
}

module_init(wb_ast2700_mac1_rgmii_fix_init);
module_exit(wb_ast2700_mac1_rgmii_fix_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OpenAI");
MODULE_DESCRIPTION("Micas M2-W6950-128OC AST2700 MAC1 RGMII delay fix");
