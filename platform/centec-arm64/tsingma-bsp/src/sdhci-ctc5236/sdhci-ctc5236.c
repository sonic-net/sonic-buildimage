/*
 * sdhci-ctc5236.c Support for SDHCI on Centec TsingMa SoC's
 *
 * Author: Wangyb <wangyb@centecnetworks.com>
 *
 * Copyright 2005-2020, Centec Networks (Suzhou) Co., Ltd.
 *
 */

#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>
#include "sdhci-pltfm.h"
#include "../pinctrl-ctc/pinctrl-ctc.h"
#include "../include/sysctl.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <asm-generic/delay.h>
#include <linux/delay.h>
#include <linux/raid/pq.h>
#include <linux/sizes.h>
#include <linux/dma-mapping.h>

#define REG_OFFSET_ADDR		0x500
#define MSHC_CTRL_R		0x8
#define AT_CTRL_R		0x40
#define SW_TUNE_EN		0x10
#define SD_CLK_EN_MASK	0x00000001
#define AT_STAT_R		0x44
#define MAX_TUNING_LOOP		0x80
#define MIN_TUNING_LOOP		0x0
#define TUNE_CTRL_STEP		1
#define EMMC_CTRL_R		0x2c

#define SDHCI_REFCLK_150M		150000000

static struct regmap *regmap_base;
static u32 version;
#define CTC_REV_TM_1_0 0x0
#define CTC_REV_TM_1_1 0x1

#define BOUNDARY_OK(addr, len) \
	((addr | (SZ_128M - 1)) == ((addr + len - 1) | (SZ_128M - 1)))

static u16 sdhci_ctc5236_readw(struct sdhci_host *host, int reg)
{
	if (unlikely(reg == SDHCI_HOST_VERSION)) {
		return SDHCI_SPEC_300;
	}

	return readw(host->ioaddr + reg);
}

static u32 sdhci_ctc5236_readl(struct sdhci_host *host, int reg)
{
	u32 ret;
	ret = readl(host->ioaddr + reg);

	return ret;
}

static void ctc5236_mmc_init_card(struct sdhci_host *host)
{
	int CLK_CTRL_R_value = 0x0;
	int AT_CTRL_R_value = 0x0;
	int MSHC_CTRL_R_value = 0x0;

	/* Disable command conflict check for 150M */
	sdhci_writel(host, MSHC_CTRL_R_value, REG_OFFSET_ADDR + MSHC_CTRL_R);
	dev_dbg(mmc_dev(host->mmc), "MSHC_CTRL_R is %x\n",
		sdhci_readl(host, REG_OFFSET_ADDR + MSHC_CTRL_R));

	/* Disable auto-tuning function */
	CLK_CTRL_R_value = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	CLK_CTRL_R_value &= (~SDHCI_CLOCK_CARD_EN);
	sdhci_writew(host, CLK_CTRL_R_value, SDHCI_CLOCK_CONTROL);
	AT_CTRL_R_value = sdhci_readl(host, REG_OFFSET_ADDR + AT_CTRL_R);
	AT_CTRL_R_value &= (~SD_CLK_EN_MASK);
	sdhci_writel(host, AT_CTRL_R_value, REG_OFFSET_ADDR + AT_CTRL_R);
	dev_dbg(mmc_dev(host->mmc), "AT_CTRL_R is %x\n",
		sdhci_readl(host, REG_OFFSET_ADDR + AT_CTRL_R));
}

void sdhci_ctc5236_reset(struct sdhci_host *host, u8 mask)
{
	unsigned long timeout;

	sdhci_writeb(host, mask, SDHCI_SOFTWARE_RESET);

	/* Wait max 100 ms */
	timeout = 100;

	/* hw clears the bit when it's done */
	while (sdhci_readb(host, SDHCI_SOFTWARE_RESET) & mask) {
		if (timeout == 0) {
			pr_err("%s: Reset 0x%x never completed.\n",
			       mmc_hostname(host->mmc), (int)mask);
			return;
		}
		timeout--;
		mdelay(1);
	}

	if (mask & SDHCI_RESET_ALL) {
		host->clock = 0;
		ctc5236_mmc_init_card(host);
	}

}

static void ctc5236_select_90degree_phase(struct sdhci_host *host)
{
	u32 val = 0;

	regmap_read(regmap_base, offsetof(struct SysCtl_regs, SysMshCfg), &val);
	if (val & SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_PHASE_SEL_MASK) {
		val &= (~SYS_MSH_CFG_W0_MSH_INTF_C_CLK_TX_PHASE_SEL_MASK);
		regmap_write(regmap_base,
			     offsetof(struct SysCtl_regs, SysMshCfg), val);
		printk("select ctc 90 degree phase\n");
	}
}

void ctc_sdhci_set_clock(struct sdhci_host *host, unsigned int clock)
{
	int val = 0;

	if (clock == SDHCI_REFCLK_150M) {
		regmap_read(regmap_base,
			    offsetof(struct SysCtl_regs, SysClkPeriCfg), &val);
		if ((val & 0xc) == 0xc) {
			val =
			    val &
			    (~SYS_CLK_PERI_CFG_W0_CFG_DIV_MSH_REF_CNT_MASK);
			val |=
			    ((0x8 &
			      SYS_CLK_PERI_CFG_W0_CFG_DIV_MSH_REF_CNT_MASK)) <<
			    0;
			regmap_write(regmap_base,
				     offsetof(struct SysCtl_regs,
					      SysClkPeriCfg), val);
			printk("SDHCI reference clock change 150M\n");
		}
	}

	if (version == CTC_REV_TM_1_1) {
		if (host->mmc->ios.timing == MMC_TIMING_MMC_DDR52) {
			ctc5236_select_90degree_phase(host);
		}
	}

	sdhci_set_clock(host, clock);
}

/*
 * If DMA addr spans 128MB boundary, we split the DMA transfer into two
 * so that each DMA transfer doesn't exceed the boundary.
 */
static void sdhci_ctc5236_adma_write_desc(struct sdhci_host *host, void **desc,
					  dma_addr_t addr, int len,
					  unsigned int cmd)
{
	int tmplen, offset;

	if (likely(!len || BOUNDARY_OK(addr, len))) {
		sdhci_adma_write_desc(host, desc, addr, len, cmd);
		return;
	}

	offset = addr & (SZ_128M - 1);
	tmplen = SZ_128M - offset;
	sdhci_adma_write_desc(host, desc, addr, tmplen, cmd);

	addr += tmplen;
	len -= tmplen;
	sdhci_adma_write_desc(host, desc, addr, len, cmd);
}

static int sdhci_ctc5236_prepare_tuning(struct sdhci_host *host,
					int CENTER_PH_CODE)
{
	int CLK_CTRL_R_value = 0x0;
	int HOST_CTRL2_R_value = 0x0;
	int AT_CTRL_R_value = 0x0;

	/* Turn-off Sampling Clock */
	CLK_CTRL_R_value = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	CLK_CTRL_R_value &= (~SDHCI_CLOCK_CARD_EN);
	sdhci_writew(host, CLK_CTRL_R_value, SDHCI_CLOCK_CONTROL);

	/* Reset Tuning Engine */
	HOST_CTRL2_R_value = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	HOST_CTRL2_R_value &= (~SDHCI_CTRL_TUNED_CLK);
	sdhci_writew(host, HOST_CTRL2_R_value, SDHCI_HOST_CONTROL2);

	/* Set up registers for CMD21 operation */
	if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_8)
		sdhci_writew(host, SDHCI_MAKE_BLKSZ(7, 128), SDHCI_BLOCK_SIZE);
	else if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_4)
		sdhci_writew(host, SDHCI_MAKE_BLKSZ(7, 64), SDHCI_BLOCK_SIZE);

	sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

	/* Enable Software tuning */
	AT_CTRL_R_value = sdhci_readl(host, REG_OFFSET_ADDR + AT_CTRL_R);
	AT_CTRL_R_value |= SW_TUNE_EN;
	sdhci_writel(host, AT_CTRL_R_value, REG_OFFSET_ADDR + AT_CTRL_R);

	/* Set tuning_cclk_sel to 0 */
	sdhci_writel(host, CENTER_PH_CODE, REG_OFFSET_ADDR + AT_STAT_R);

	/* Turn-on Sampling Clock */
	CLK_CTRL_R_value |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, CLK_CTRL_R_value, SDHCI_CLOCK_CONTROL);

	return 0;
}

static int sdhci_ctc5236_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	int min, max, avg, ret;
	int val = 0;

	/* Disable Rx bypass when tuning */
	regmap_read(regmap_base, offsetof(struct SysCtl_regs, SysMshCfg), &val);
	val &= (~SYS_MSH_CFG_W0_MSH_INTF_RX_DLL_MASTER_BYPASS_MASK);
	regmap_write(regmap_base, offsetof(struct SysCtl_regs, SysMshCfg), val);

	sdhci_ctc5236_prepare_tuning(host, 0);

	/* find the mininum delay first which can pass tuning */
	min = MIN_TUNING_LOOP;
	sdhci_writel(host, min, REG_OFFSET_ADDR + AT_STAT_R);
	while (min < MAX_TUNING_LOOP) {
		if (!mmc_send_tuning(host->mmc, opcode, NULL))
			break;
		host->ops->reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
		min += TUNE_CTRL_STEP;
		sdhci_writel(host, min, REG_OFFSET_ADDR + AT_STAT_R);
	}

	/* find the maxinum delay which can not pass tuning */
	max = min + TUNE_CTRL_STEP;
	sdhci_writel(host, max, REG_OFFSET_ADDR + AT_STAT_R);
	while (max < MAX_TUNING_LOOP) {
		if (mmc_send_tuning(host->mmc, opcode, NULL)) {
			max -= TUNE_CTRL_STEP;
			break;
		}
		host->ops->reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
		max += TUNE_CTRL_STEP;
		sdhci_writel(host, max, REG_OFFSET_ADDR + AT_STAT_R);
	}

	/* use average delay to get the best timing */
	avg = (min + max) / 2;
	sdhci_writel(host, avg, REG_OFFSET_ADDR + AT_STAT_R);
	ret = mmc_send_tuning(host->mmc, opcode, NULL);
	host->ops->reset(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA);

	dev_info(mmc_dev(host->mmc),
		 "Tuning %s at 0x%x ret %d, min is 0x%x, max is 0x%x\n",
		 ret ? "failed" : "passed", avg, ret, min, max);

	return ret;
}

static void sdhci_ctc5236_hw_reset(struct sdhci_host *host)
{
	sdhci_writel(host, 0x0, REG_OFFSET_ADDR + EMMC_CTRL_R);
	udelay(10);
	sdhci_writel(host, 0xc, REG_OFFSET_ADDR + EMMC_CTRL_R);
	udelay(300);
	dev_info(mmc_dev(host->mmc), "Hardware reset\n");
}

static const struct sdhci_ops sdhci_ctc5236_ops = {
	.read_w = sdhci_ctc5236_readw,
	.read_l = sdhci_ctc5236_readl,
	.set_clock = ctc_sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_ctc5236_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
	.get_max_clock = sdhci_pltfm_clk_get_max_clock,
	.platform_execute_tuning = sdhci_ctc5236_execute_tuning,
	.adma_write_desc = sdhci_ctc5236_adma_write_desc,
	.hw_reset = sdhci_ctc5236_hw_reset,
};

static struct sdhci_pltfm_data sdhci_ctc5236_pdata = {
	.ops = &sdhci_ctc5236_ops,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN | SDHCI_QUIRK2_BROKEN_HS200,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
};

static int sdhci_ctc5236_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;
	struct clk *clk;
	int ret, val;
	u32 extra;

	host = sdhci_pltfm_init(pdev, &sdhci_ctc5236_pdata, 0);
	if (IS_ERR(host))
		return PTR_ERR(host);

	/*
	 * extra adma table cnt for cross 128M boundary handling.
	 */
	extra = DIV_ROUND_UP_ULL(dma_get_required_mask(&pdev->dev), SZ_128M);
	if (extra > SDHCI_MAX_SEGS)
		extra = SDHCI_MAX_SEGS;
	host->adma_table_cnt += extra;

	regmap_base =
	    syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "ctc,sysctrl");
	if (IS_ERR(regmap_base))
		return PTR_ERR(regmap_base);

	val = 0x3400027;
	regmap_write(regmap_base, offsetof(struct SysCtl_regs, SysMshCfg), val);

	regmap_read(regmap_base, offsetof(struct SysCtl_regs, SysCtlSysRev),
		    &val);

	version = (val == 0x1) ? CTC_REV_TM_1_1 : CTC_REV_TM_1_0;

	mmc_of_parse_voltage(host->mmc, &host->ocr_mask);

	ret = mmc_of_parse(host->mmc);
	if (ret)
		goto err_sdhci_add;

	clk = devm_clk_get(&pdev->dev, "mmc_clk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Peripheral clk not found\n");
		return PTR_ERR(clk);
	}
	pltfm_host = sdhci_priv(host);
	pltfm_host->clk = clk;
	clk_prepare_enable(clk);

	if (version == CTC_REV_TM_1_0) {
		if (host->mmc->caps & MMC_CAP_1_8V_DDR) {
			host->mmc->caps &= ~MMC_CAP_1_8V_DDR;
			printk("%s, not support DDR Mode\n", __func__);
		}
	}

	if (host->mmc->caps2 & MMC_CAP2_HS200_1_8V_SDR) {
		host->mmc->caps2 &= ~MMC_CAP2_HS200_1_8V_SDR;
		printk("%s, not support Hs200 Mode\n", __func__);
	}

	ret = sdhci_add_host(host);
	if (ret)
		goto err_sdhci_add;
	return 0;

err_sdhci_add:
	sdhci_pltfm_free(pdev);
	return ret;
}

static const struct of_device_id sdhci_ctc5236_of_match[] = {
	{.compatible = "centec,ctc5236-sdhci"},
	{},
};

MODULE_DEVICE_TABLE(of, sdhci_ctc5236_of_match);

static struct platform_driver sdhci_ctc5236_driver = {
	.driver = {
		   .name = "sdhci-ctc5236",
		   .pm = &sdhci_pltfm_pmops,
		   .of_match_table = of_match_ptr(sdhci_ctc5236_of_match),
		   },
	.probe = sdhci_ctc5236_probe,
	.remove = sdhci_pltfm_unregister,
};

module_platform_driver(sdhci_ctc5236_driver);

MODULE_DESCRIPTION("SDHCI driver for Centec TsingMa SoCs");
MODULE_AUTHOR("Wangyb <wangyb@centecnetworks.com>");
MODULE_LICENSE("GPL v2");
