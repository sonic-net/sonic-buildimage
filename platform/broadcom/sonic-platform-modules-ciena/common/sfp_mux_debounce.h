#ifndef __SFP_MUX_DEBOUNCE
#define __SFP_MUX_DEBOUNCE

#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif /* CONFIG_OF */
#include <linux/slab.h>
#include <linux/types.h>

/*
 * Here is how we can implement a mux debouncing mechanism outside of
 * the core i2c driver stack. This header file replaces the following
 * commits:
 *
 *   a5524c1448cda082aa5f5b9768cd129fc1d4e898 i2x-mux: Clear "stuck" Oplink SFPs by sending a current address read command when the I2C mux is probed on a reboot.
 *   bda5a1a184e8d57a21cc77265d987e827d9d52d5 Add stuck SFP recovery at runtime, enabling stuck SFPs to be recovered without a reboot.
 *   302a3fcc0cbe96b5f7cdd142ec0be80e5048bb9f I2C: add delay after demux
 *   247ffd6963a288441bedb3ed7f4b11bd5c6dd9f1 I2C: lengthen deselect delay
 *   6f1ce460805a6c74fa6761b1fb6089987c9a2272 Add checks for slave addresses 0x51 and 0x56
 *   537435862be377bc53155b353d2e0d11d8900618 Add a delay between mux selection and i2c bus cycle.
 *   d11a7ca31ef45956fdb24744035f7a193c74c346 Add extra delay time for i2c data line to float high
 *
 * However this is all theory. In practice:
 *
 *   - The debouncing delays are not necessary on the designware
 *     (dernhelm) and cadence (eredan) i2c controllers. On these
 *     platforms, when the mux is de-selected the transfers are
 *     properly stopped.
 */


struct sfp_mux_debounce {
	struct device *mux;
	bool prev_xfer_failed;
	u32 select_delay_usec;
	u32 deselect_delay_usec;
};

static const u32 sfp_mux_tsuSTA = 5; /* setup time for repeat START: 4.5 Âµsec */

static inline u32 sfp_mux_debounce_select(struct sfp_mux_debounce *smd)
{
	u32 delay_usec;

	if (NULL == smd) return 0;

	delay_usec = smd->select_delay_usec;

	/* If the transfer failed from the previous mux position, SCL
	 * may be stuck low. Moving the mux will allow SCL to rise
	 * slowly. But if the controller proceeds too soon with a
	 * START, then it might be interpreted as a repeat START. */
	if ((smd->prev_xfer_failed) && (sfp_mux_tsuSTA > delay_usec))
		delay_usec = sfp_mux_tsuSTA;

	if (!delay_usec) return 0;

	dev_dbg(smd->mux, "%s: usleep_range(%u)\n", __func__, delay_usec);

	usleep_range(delay_usec, delay_usec * 2);

	return delay_usec;
}

static inline u32 sfp_mux_debounce_deselect(struct sfp_mux_debounce *smd)
{
	if ((NULL == smd) || (0 == smd->deselect_delay_usec)) return 0;

	dev_dbg(smd->mux, "%s: usleep_range(%u)\n",
		__func__, smd->deselect_delay_usec);

	usleep_range(smd->deselect_delay_usec,
		     smd->deselect_delay_usec*2);

	return smd->select_delay_usec;
}

static inline int sfp_mux_debounce_probe(struct sfp_mux_debounce *smd)
{
	if (! smd)
		return 0;

	smd->select_delay_usec = 0;
	smd->deselect_delay_usec = 0;

	if (smd && smd->mux) {
		const struct i2c_fpga_mux_info *info = smd->mux->platform_data;
#ifdef CONFIG_OF
		struct device_node *dn = smd->mux->of_node;
		if (dn) {
			u32 delay_msec = 0;

			/*
			 * The device tree parameters are:
			 *
			 *    ciena,select-delay-msec: milliseconds to wait after a
			 *                             mux line has been selected
			 *
			 *    ciena,deselect-delay-msec: milliseconds to wait before a
			 *                               mux line is de-selected
			 */
			if (0 == of_property_read_u32(dn, "ciena,select-delay-msec",
							  &delay_msec)) {
				smd->select_delay_usec = delay_msec*1000;
				dev_info(smd->mux, "%s: select_delay_usec = %u",
					 __func__, smd->select_delay_usec);
			}

			if (0 == of_property_read_u32(dn, "ciena,deselect-delay-msec",
							  &delay_msec)) {
				smd->deselect_delay_usec = delay_msec*1000;
				dev_info(smd->mux, "%s: deselect_delay_usec = %u",
					 __func__, smd->deselect_delay_usec);
			}
		}
#endif
		if (info) {
			smd->select_delay_usec = info->select_delay_usec;
			smd->deselect_delay_usec = info->deselect_delay_usec;

		}
	}

	return 0;
}

static inline void sfp_mux_debounce_remove(struct sfp_mux_debounce *smd)
{
	if (smd) {
		smd->select_delay_usec = 0;
		smd->deselect_delay_usec = 0;
	}
}

#endif /* __SFP_MUX_DEBOUNCE */
