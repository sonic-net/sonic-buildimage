/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#include "ixgbe.h"

#include <linux/ptp_classify.h>
#include <linux/bitfield.h>
#include "ixgbe_ptp_e600.h"

#define IXGBE_E600_CC_SHIFT		31
#define IXGBE_E600_CC_MULT		BIT(IXGBE_E600_CC_SHIFT)
#define IXGBE_E600_FADJ_SHIFT		9
#define IXGBE_E600_FADJ_DENOMINATOR	15625ULL
#define IXGBE_E600_REFRESH_INTERVAL	HZ
#define TSAUXC_DIS_TS_CLEAR		0x40000000

static const struct ptp_pin_desc e600_pin_cfg[] = {
	{ "SDP0", 0, 0, 0 }
};

/**
 * ixgbe_ptp_read_phc_e600 - read PHC value
 * @cc: cyclecounter structure
 *
 * Return: PHC nanosecond time.
 */
#ifdef HAVE_NON_CONST_CYCLECOUNTER
static u64 ixgbe_ptp_read_phc_e600(struct cyclecounter *cc)
#else
static u64 ixgbe_ptp_read_phc_e600(const struct cyclecounter *cc)
#endif
{
	struct ixgbe_adapter *adapter =
		container_of(cc, struct ixgbe_adapter, hw_cc);
	struct ixgbe_hw *hw = &adapter->hw;
	struct timespec64 ts;

	IXGBE_READ_REG(hw, IXGBE_SYSTIMR);
	ts.tv_nsec = IXGBE_READ_REG(hw, IXGBE_SYSTIML);
	ts.tv_sec = IXGBE_READ_REG(hw, IXGBE_SYSTIMH);

	return (u64)timespec64_to_ns(&ts);
}

/**
 * ixgbe_ptp_update_1pps_e600 - update 1PPS start and frequency
 * @adapter: the private adapter struct
 */
static void ixgbe_ptp_update_1pps_e600(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 period;
	u64 start;

	period = (NSEC_PER_SEC << adapter->hw_cc.shift) / adapter->hw_cc.mult;
	IXGBE_WRITE_REG(&adapter->hw, IXGBE_FREQOUT0, period / 2);

	start = IXGBE_READ_REG(hw, IXGBE_TRGTTIML0) +
		IXGBE_READ_REG(hw, IXGBE_TRGTTIMH0) * NSEC_PER_SEC;

	/* Start the timer at the closest multiple of period.
	 * If it happens in the past, round it up.
	 */
	start = DIV_U64_ROUND_CLOSEST(start, period) * period;
	if (start < ixgbe_ptp_read_phc_e600(&adapter->hw_cc))
		start = roundup_u64(start, period);

	start = (start << adapter->hw_cc.shift) / adapter->hw_cc.mult +
		adapter->hw_tc.frac;
	IXGBE_WRITE_REG(hw, IXGBE_TRGTTIML0, start % NSEC_PER_SEC);
	IXGBE_WRITE_REG(hw, IXGBE_TRGTTIMH0, start / NSEC_PER_SEC);
	IXGBE_WRITE_FLUSH(hw);
}

/**
 * ixgbe_ptp_adjfine_e600 - adjust PHC increment rate
 * @ptp: the ptp clock structure
 * @scaled_ppm: scaled parts per million adjustment from base
 *
 * Adjust the frequency of the PTP timecounter by the indicated scaled_ppm
 * from the base frequency.
 *
 * Scaled parts per million is ppm with a 16-bit binary fractional field.
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_adjfine_e600(struct ptp_clock_info *ptp, long scaled_ppm)
{
	struct ixgbe_adapter *adapter = container_of(ptp, struct ixgbe_adapter,
						     ptp_caps);
	unsigned long flags;
	s64 adj;

	adj = (s64)scaled_ppm << IXGBE_E600_FADJ_SHIFT;
	adj = div_s64(adj, IXGBE_E600_FADJ_DENOMINATOR);

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_read(&adapter->hw_tc);
	adapter->hw_cc.mult = IXGBE_E600_CC_MULT + adj;
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	ixgbe_ptp_update_1pps_e600(adapter);

	return 0;
}

#ifndef HAVE_PTP_CLOCK_INFO_ADJFINE
/**
 * ixgbe_ptp_adjfreq_e600 - Adjust the frequency of the clock
 * @info: the driver's PTP info structure
 * @ppb: Parts per billion adjustment from the base
 *
 * Adjust the frequency of the clock by the indicated parts per billion from the
 * base frequency.
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_adjfreq_e600(struct ptp_clock_info *info, s32 ppb)
{
	long scaled_ppm;

	/*
	 * We want to calculate
	 *
	 *    scaled_ppm = ppb * 2^16 / 1000
	 *
	 * which simplifies to
	 *
	 *    scaled_ppm = ppb * 2^13 / 125
	 */
	scaled_ppm = ((long)ppb << 13) / 125;
	return ixgbe_ptp_adjfine_e600(info, scaled_ppm);
}

#endif
/**
 * ixgbe_ptp_gettime_e600 - get PHC time and optional system timestamp
 * @ptp: the ptp clock structure
 * @ts: timespec64 structure to hold the current time value
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_gettime_e600(struct ptp_clock_info *ptp,
				  struct timespec64 *ts)
{
	struct ixgbe_adapter *adapter = container_of(ptp, struct ixgbe_adapter,
						     ptp_caps);
	unsigned long flags;
	u64 ns;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	ns = timecounter_read(&adapter->hw_tc);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
	*ts = ns_to_timespec64(ns);

	return 0;
}

/**
 * ixgbe_ptp_setup_sdp_e600
 * @adapter: private adapter structure
 * @rq: periodic output request
 * @on: true to enable SDP output, false otherwise
 *
 * Enable or disable a clock output signal on SDP0 for E600 hardware.
 *
 * Use the target time feature to align the output signal on the next full
 * second.
 *
 * Return: 0 on success, negative error code otherwise
 */
static int ixgbe_ptp_setup_sdp_e600(struct ixgbe_adapter *adapter,
				    struct ptp_perout_request *rq, bool on)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 period, esdp, tsauxc, tssdp;
	u64 clk, start, phase;
	struct timespec64 ts;
	int err;

#ifdef PTP_PEROUT_PHASE
	if (rq->flags & ~PTP_PEROUT_PHASE)
		return -EOPNOTSUPP;
#else /* !PTP_PEROUT_PHASE */
	if (rq->flags)
		return -EOPNOTSUPP;
#endif /* PTP_PEROUT_PHASE */

	/* Disable the pin first. */
	IXGBE_WRITE_REG(hw, IXGBE_TSAUXC, 0x0);
	IXGBE_WRITE_FLUSH(hw);

	period = rq->period.sec * NSEC_PER_SEC + rq->period.nsec;
	if (!on || !period)
		return 0;

	if (period != NSEC_PER_SEC) {
		e_dev_err("CLK Period != 1s is not supported\n");
		return -EINVAL;
	}

	start = rq->start.sec * NSEC_PER_SEC + rq->start.nsec;

#ifdef PTP_PEROUT_PHASE
	/* If PTP_PEROUT_PHASE is set, rq has phase instead of start time. */
	if (rq->flags & PTP_PEROUT_PHASE)
		phase = start;
	else
#endif /* PTP_PEROUT_PHASE */
		div64_u64_rem(start, period, &phase);

	if (phase % NSEC_PER_SEC != 0) {
		e_dev_err("Start/phase not on a second boundary is not supported\n");
		return -EINVAL;
	}

	/* Convert 1PPS output frequency and start time with cyclecounter
	 * params.
	 */
	period = (NSEC_PER_SEC << adapter->hw_cc.shift) / adapter->hw_cc.mult;
	start = (start << adapter->hw_cc.shift) / adapter->hw_cc.mult +
		adapter->hw_tc.frac;

	/* If we have only phase or start time is in the past, start the timer
	 * at the next multiple of period, maintaining phase at least 0.1 second
	 * from now, so we have time to write it to HW.
	 */
	err = ixgbe_ptp_gettime_e600(&adapter->ptp_caps, &ts);
	if (err)
		return err;
	clk = (u64)timespec64_to_ns(&ts) + NSEC_PER_MSEC * 100;
#ifdef PTP_PEROUT_PHASE
	if (rq->flags & PTP_PEROUT_PHASE || start <= clk)
#else /* !PTP_PEROUT_PHASE */
	if (start <= clk)
#endif /* PTP_PEROUT_PHASE */
		start = roundup_u64(clk, period) + phase;

	/* Write half of the period (50% duty cycle). */
	IXGBE_WRITE_REG(hw, IXGBE_FREQOUT0, period / 2);
	IXGBE_WRITE_REG(hw, IXGBE_TRGTTIML0, start % NSEC_PER_SEC);
	IXGBE_WRITE_REG(hw, IXGBE_TRGTTIMH0, start / NSEC_PER_SEC);

	/* Enable the SDP0 pin as output. */
	esdp = IXGBE_READ_REG(hw, IXGBE_ESDP);
	esdp |= IXGBE_ESDP_SDP0_DIR | IXGBE_ESDP_SDP0_NATIVE;
	IXGBE_WRITE_REG(hw, IXGBE_ESDP, esdp);

	tssdp = IXGBE_TSSDP_TS_SDP0_EN | IXGBE_TSSDP_TS_SDP0_CLK0;
	IXGBE_WRITE_REG(hw, IXGBE_TSSDP, tssdp);

	/* Enable the Clock Out feature on SDP0, and use Target Time 0 to
	 * enable generation of interrupts on the clock change.
	 */
	tsauxc = IXGBE_TSAUXC_EN_CLK | IXGBE_TSAUXC_ST0 | IXGBE_TSAUXC_EN_TT0 |
		 IXGBE_TSAUXC_SDP0_INT | TSAUXC_DIS_TS_CLEAR;
	IXGBE_WRITE_REG(hw, IXGBE_TSAUXC, tsauxc);

	IXGBE_WRITE_FLUSH(hw);

	return 0;
}

/**
 * ixgbe_ptp_verify_pin_e600 - verify if pin supports requested pin function
 * @info: the driver's PTP info structure
 * @pin: pin index
 * @func: assigned function
 * @chan: assigned channel
 *
 * Return: 0 on success, -EOPNOTSUPP when function is not supported.
 */
static int ixgbe_ptp_verify_pin_e600(struct ptp_clock_info *info,
				     unsigned int pin,
				     enum ptp_pin_function func,
				     unsigned int chan)
{
	/* Is assigned function allowed? */
	switch (func) {
	case PTP_PF_PEROUT:
	case PTP_PF_NONE:
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

/**
 * ixgbe_ptp_gpio_enable_e600 - Enable/disable ancillary features of PHC
 * @info: the driver's PTP info structure
 * @rq: the requested feature to change
 * @on: enable/disable flag
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int ixgbe_ptp_gpio_enable_e600(struct ptp_clock_info *info,
				      struct ptp_clock_request *rq, int on)
{
	struct ixgbe_adapter *adapter = container_of(info, struct ixgbe_adapter,
						     ptp_caps);

	switch (rq->type) {
	case PTP_CLK_REQ_PEROUT:
	{
		int err = ixgbe_ptp_setup_sdp_e600(adapter, &rq->perout, on);

		if (!err && on)
			adapter->flags2 |= IXGBE_FLAG2_PTP_PPS_ENABLED;
		else
			adapter->flags2 &= ~IXGBE_FLAG2_PTP_PPS_ENABLED;

		return err;
	}
	default:
		return -EOPNOTSUPP;
	}
}

/**
 * ixgbe_ptp_settime_e600 - set PHC time
 * @ptp: the ptp clock structure
 * @ts: the timespec containing the new time
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_settime_e600(struct ptp_clock_info *ptp,
				  const struct timespec64 *ts)
{
	struct ixgbe_adapter *adapter = container_of(ptp, struct ixgbe_adapter,
						     ptp_caps);
	u64 ns = timespec64_to_ns(ts);
	unsigned long flags;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_init(&adapter->hw_tc, &adapter->hw_cc, ns);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	ixgbe_ptp_update_1pps_e600(adapter);

	return 0;
}

#ifndef HAVE_PTP_CLOCK_INFO_GETTIME64
/**
 * ixgbe_ptp_gettime32_e600 - get PHC time
 * @ptp: the ptp clock structure
 * @ts: timespec64 structure to hold the current time value
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_gettime32_e600(struct ptp_clock_info *ptp,
				    struct timespec *ts)
{
	struct timespec64 ts64;
	int err;

	err = ixgbe_ptp_gettime_e600(ptp, &ts64);
	if (err)
		return err;

	*ts = timespec64_to_timespec(ts64);

	return 0;
}

/**
 * ixgbe_ptp_settime_e600 - set PHC time
 * @ptp: the ptp clock structure
 * @ts: the timespec containing the new time
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_settime32_e600(struct ptp_clock_info *ptp,
				    const struct timespec *ts)
{
	struct timespec64 ts64 = timespec_to_timespec64(*ts);

	return ixgbe_ptp_settime_e600(ptp, &ts64);
}
#endif

/**
 * ixgbe_ptp_adjtime_e600 - adjust time by a specified delta
 * @ptp: the ptp clock structure
 * @delta: offset to adjust the cycle counter by
 *
 * Return: 0 on success.
 */
static int ixgbe_ptp_adjtime_e600(struct ptp_clock_info *ptp, s64 delta)
{
	struct ixgbe_adapter *adapter = container_of(ptp, struct ixgbe_adapter,
						     ptp_caps);
	unsigned long flags;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_adjtime(&adapter->hw_tc, delta);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	ixgbe_ptp_update_1pps_e600(adapter);

	return 0;
}

/**
 * ixgbe_ptp_cfg_phy_timestamping_e610 - configure PTP timestamping on the PHY
 * @adapter: pointer to the adapter structure
 *
 * This function is intended only for E610 PHY.
 */
static void ixgbe_ptp_cfg_phy_timestamping_e610(struct ixgbe_adapter *adapter)
{
	struct ixgbe_ptp_e600 *ptp = &adapter->ptp;
	struct ixgbe_hw *hw = &adapter->hw;
	u8 ptp_request, flags;
	bool enable;
	s32 status;

	/* Enable only for <= 1 GB. */
	switch (adapter->link_speed) {
	case IXGBE_LINK_SPEED_10_FULL:
	case IXGBE_LINK_SPEED_100_FULL:
	case IXGBE_LINK_SPEED_1GB_FULL:
		enable = true;
		break;
	default:
		enable = false;
		break;
	}

	status = ixgbe_get_ptp_by_phy(hw, &ptp_request, &flags,
				      &ptp->max_drift_threshold);
	if (status) {
		e_dev_err("PTP by PHY get failed, status=%d\n", status);
		return;
	}

	ptp->ptp_by_phy_ena = ptp_request;

	/* Don't reconfigure if already set correctly. */
	if (enable == ptp_request)
		goto skip_set;

	if (adapter->ptp.vlan_ena)
		flags = FIELD_PREP(IXGBE_SET_PTP_BY_PHY_ETHERTYPE_M,
				   IXGBE_SET_PTP_BY_PHY_ETHERTYPE_VLAN_TAG);
	else
		flags = FIELD_PREP(IXGBE_SET_PTP_BY_PHY_ETHERTYPE_M,
				   IXGBE_SET_PTP_BY_PHY_ETHERTYPE_NO_VLAN_TAG);

	status = ixgbe_set_ptp_by_phy(hw, enable, flags);
	if (status) {
		e_dev_err("PTP by PHY set failed, status=%d\n", status);
		return;
	}

skip_set:
	if (!enable) {
		ptp->ptp_by_phy_ena = false;
		return;
	}

	/* Clear timestamping status registers. */
	IXGBE_WRITE_REG(hw, PROXY_TX_STS, 0);
	IXGBE_WRITE_REG(hw, PROXY_RX_STS, 0);

	ptp_request = IXGBE_SET_PTP_BY_PHY_PTP_REQ_SET_PHY_PARAMS;
	status = ixgbe_set_ptp_by_phy(hw, ptp_request, flags);
	if (status)
		e_dev_err("PTP by PHY set PHY params failed, status=%d\n",
			  status);

	ptp_request = IXGBE_SET_PTP_BY_PHY_PTP_REQ_TOD_INIT;
	status = ixgbe_set_ptp_by_phy(hw, ptp_request, 0);
	if (status) {
		e_dev_err("PTP by PHY ToD init failed, status=%d\n", status);
	} else {
		ptp->ptp_by_phy_ena = true;
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
		ptp_schedule_worker(adapter->ptp_clock, 0);
#else /* HAVE_PTP_CANCEL_WORKER_SYNC */
		kthread_mod_delayed_work(ptp->ptp_kworker, &ptp->ptp_aux_work,
					 0);
#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */
	}
}

/**
 * ixgbe_ptp_clear_tx_timestamp_e600 - clear Tx timestamp state
 * @adapter: the private adapter structure
 *
 * This function should be called whenever the state related to a Tx timestamp
 * needs to be cleared. This helps ensure that all related bits are reset for
 * the next Tx timestamp event.
 */
static void ixgbe_ptp_clear_tx_timestamp_e600(struct ixgbe_adapter *adapter)
{
	IXGBE_READ_REG(&adapter->hw, IXGBE_TXSTMPH);
	if (adapter->ptp_tx_skb) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
	}
	clear_bit_unlock(__IXGBE_PTP_TX_IN_PROGRESS, adapter->state);
}

/**
 * ixgbe_ptp_is_tx_ptp - check if packet is a PTP packet
 * @adapter: the private adapter struct
 * @skb: the packet
 *
 * Return: true if packet is a PTP packet, false otherwise.
 */
bool ixgbe_ptp_is_tx_ptp(struct ixgbe_adapter *adapter, struct sk_buff *skb)
{
	unsigned int ptp_class;
	struct ptp_header *hdr;

	if (likely(!adapter->ptp.ptp_by_phy_ena))
		return false;

	ptp_class = ptp_classify_raw(skb);
	if (likely(ptp_class == PTP_CLASS_NONE))
		return false;

	skb_reset_mac_header(skb);
	hdr = ptp_parse_header(skb, ptp_class);

	/* Clear Tx TS bit if it's a PTP packet with invalid header. */
	if (!hdr)
		clear_bit_unlock(__IXGBE_PTP_TX_IN_PROGRESS, adapter->state);
	else
		adapter->ptp.ptp_seq_id = ntohs(hdr->sequence_id);

	return true;
}

/**
 * ixgbe_ptp_tx_phytstamp_e600 - read the PHY Tx timestamp from the register
 * @adapter: the private adapter struct
 * @sts: value of the status register
 *
 * Return:
 * * %0       - success
 * * %-ENXIO  - Tx TS not in progress
 * * %-EINVAL - invalid sequence ID
 * * %other   - FW error code
 */
static int ixgbe_ptp_tx_phytstamp_e600(struct ixgbe_adapter *adapter, u32 sts)
{
	struct sk_buff *skb = adapter->ptp_tx_skb;
	struct skb_shared_hwtstamps shhwtstamps;
	struct ixgbe_hw *hw = &adapter->hw;
	u64 tstamp;
	int err;

	/* Return error if PTP Tx TS not in progress. */
	if (!test_bit(__IXGBE_PTP_TX_IN_PROGRESS, adapter->state)) {
		err = -ENXIO;
		goto err;
	}

	err = FIELD_GET(PROXY_STS_ERR, sts);
	if (err)
		goto err;

	/* Incorrect sequence ID, discard the timestamp. */
	if (FIELD_GET(PROXY_STS_SEQ_ID, sts) != adapter->ptp.ptp_seq_id) {
		err = -EINVAL;
		goto err;
	}

	tstamp = IXGBE_READ_REG(hw, PROXY_TX_TS_LOW);
	tstamp |= (u64)IXGBE_READ_REG(hw, PROXY_TX_TS_HIGH) << 32;
	spin_lock(&adapter->tmreg_lock);
	tstamp = timecounter_cyc2time(&adapter->hw_tc, tstamp);
	spin_unlock(&adapter->tmreg_lock);

	/* Notify the stack and then free the skb after we've unlocked. */
	shhwtstamps.hwtstamp = ns_to_ktime(tstamp);
	skb_tstamp_tx(skb, &shhwtstamps);
err:
	adapter->ptp_tx_skb = NULL;
	clear_bit_unlock(__IXGBE_PTP_TX_IN_PROGRESS, adapter->state);
	dev_kfree_skb_any(skb);
	if (err)
		adapter->tx_hwtstamp_skipped++;

	return err;
}

/**
 * ixgbe_ptp_rx_find_skb_e600 - find skb for a specified sequence ID and add
 *				Rx timestamp if found
 * @adapter: the private adapter struct
 * @seq_id: PTP packet sequence ID
 * @ns: Rx timestamp
 *
 * Return: true if skb found, false otherwise.
 */
static bool ixgbe_ptp_rx_find_skb_e600(struct ixgbe_adapter *adapter,
				       u16 seq_id, u64 tstamp)
{
	struct ixgbe_rx_phy_ts_skb_e610 *rx_skb;
	bool skb_found = false;
	unsigned int i;

	for (i = 0; i < adapter->num_q_vectors; i++) {
		struct ixgbe_q_vector *q_vector = adapter->q_vector[i];

		spin_lock(&q_vector->ptp_skbs_lock_e600);
		list_for_each_entry(rx_skb, &q_vector->ptp_skbs_e600, list) {
			if (rx_skb->seq_id != seq_id)
				continue;

			skb_hwtstamps(rx_skb->skb)->hwtstamp =
				ns_to_ktime(tstamp);
			napi_schedule(&q_vector->napi);
			skb_found = true;
			break;
		}
		spin_unlock(&q_vector->ptp_skbs_lock_e600);

		if (skb_found)
			return true;
	}

	return false;
}

/**
 * ixgbe_ptp_rx_complete_tstamp_e600 - complete cached PHY Rx timestamps
 * @adapter: the private adapter struct
 * @sts: value of the status register
 *
 * Return:
 * * %0       - success
 * * %-ENOMEM - no memory left on the device
 */
static int ixgbe_ptp_rx_complete_tstamp_e600(struct ixgbe_adapter *adapter,
					     u32 sts)
{
	u16 seq_id = FIELD_GET(PROXY_STS_SEQ_ID, sts);
	u64 ns;

	ns = IXGBE_READ_REG(&adapter->hw, PROXY_RX_TS_LOW);
	ns |= (u64)IXGBE_READ_REG(&adapter->hw, PROXY_RX_TS_HIGH) << 32;
	spin_lock(&adapter->tmreg_lock);
	ns = timecounter_cyc2time(&adapter->hw_tc, ns);
	spin_unlock(&adapter->tmreg_lock);

	/* Try to find matching skb. If not found, cache the timestamp. */
	if (!ixgbe_ptp_rx_find_skb_e600(adapter, seq_id, ns)) {
		struct ixgbe_rx_phy_ts_skb_e610 *tstamp;

		tstamp = kzalloc(sizeof(*tstamp), GFP_KERNEL);
		if (!tstamp)
			return -ENOMEM;

		tstamp->ns = ns;
		tstamp->seq_id = seq_id;
		tstamp->acquired = jiffies;
		spin_lock(&adapter->ptp.rx_tstamps_lock);
		list_add_tail(&tstamp->list, &adapter->ptp.rx_tstamps);
		spin_unlock(&adapter->ptp.rx_tstamps_lock);
	}

	return 0;
}

/**
 * ixgbe_ptp_fw_intr_e600 - handle PTP by PHY FW interrupt
 * @adapter: the private adapter struct
 *
 * Return:
 * * %0       - success
 * * %-EINTR  - not a Tx/Rx TS interrupt cause
 * * %-ENXIO  - Tx TS not in progress
 * * %-ENOMEM - no memory left on the device
 * * %-EINVAL - invalid sequence ID
 * * %other   - FW error code
 */
int ixgbe_ptp_fw_intr_e600(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	u32 tx_sts, rx_sts;
	int err = -EINTR;

	/* Handle FW Tx interrupt if status register indicates it. */
	tx_sts = IXGBE_READ_REG(hw, PROXY_TX_STS);
	if (tx_sts & PROXY_STS_TS_RDY && tx_sts & PROXY_STS_TS_INT) {
		err = ixgbe_ptp_tx_phytstamp_e600(adapter, tx_sts);
		IXGBE_WRITE_REG(hw, PROXY_TX_STS, 0);
	} else if (!(tx_sts & PROXY_STS_TS_RDY) && tx_sts & PROXY_STS_TS_INT) {
		atomic_set(&adapter->ptp.reinit_phy_tod, true);
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
		ptp_schedule_worker(adapter->ptp_clock, 0);
#else /* HAVE_PTP_CANCEL_WORKER_SYNC */
		kthread_mod_delayed_work(adapter->ptp.ptp_kworker,
					 &adapter->ptp.ptp_aux_work, 0);
#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */
		err = 0;
		IXGBE_WRITE_REG(hw, PROXY_TX_STS, 0);
	}

	/* Handle FW Rx interrupt if status register indicates it. */
	rx_sts = IXGBE_READ_REG(hw, PROXY_RX_STS);
	if (rx_sts & PROXY_STS_TS_RDY && rx_sts & PROXY_STS_TS_INT) {
		err = ixgbe_ptp_rx_complete_tstamp_e600(adapter, rx_sts);
		IXGBE_WRITE_REG(hw, PROXY_RX_STS, 0);
	} else if (!(rx_sts & PROXY_STS_TS_RDY) && rx_sts & PROXY_STS_TS_INT) {
		IXGBE_WRITE_REG(hw, PROXY_RX_STS, 0);
	}

	IXGBE_WRITE_FLUSH(hw);
	return err;
}

/**
 * ixgbe_ptp_rx_complete_skb_e600 - complete held PTP skbs
 * @q_vector: structure containing interrupt and ring information
 * @budget: amount of work driver is allowed to do this pass, in packets
 *
 * This function takes PTP skbs, which were held in order to add Rx timestamp to
 * them. This is necessary due to the interrupt with Rx TS coming usually after
 * the driver receives the packet.
 * When skb has the timestamp added it's sent further up the stack. Otherwise,
 * it's removed after 500 ms timeout.
 *
 * Return: number of completed skbs.
 */
unsigned int ixgbe_ptp_rx_complete_skb_e600(struct ixgbe_q_vector *q_vector,
					    int *budget)
{
	struct ixgbe_rx_phy_ts_skb_e610 *rx_skb, *n;
	unsigned int cleaned = 0;
	unsigned long flags;

	spin_lock_irqsave(&q_vector->ptp_skbs_lock_e600, flags);
	list_for_each_entry_safe(rx_skb, n, &q_vector->ptp_skbs_e600, list) {
		unsigned long acquired = rx_skb->acquired;

		/* Flush skbs without timestamps after 500 ms. */
		if (time_is_before_jiffies(acquired + msecs_to_jiffies(500))) {
			consume_skb(rx_skb->skb);
			list_del(&rx_skb->list);
			kfree(rx_skb);
			continue;
		}

		if (skb_hwtstamps(rx_skb->skb)->hwtstamp) {
			napi_gro_receive(&q_vector->napi, rx_skb->skb);
			list_del(&rx_skb->list);
			kfree(rx_skb);
			(*budget)--;
			cleaned++;
			if (*budget <= 0)
				break;
		}
	}
	spin_unlock_irqrestore(&q_vector->ptp_skbs_lock_e600, flags);

	return cleaned;
}

/**
 * ixgbe_ptp_rx_phytstamp_e600 - utility function to get RX tstamp PTP reserved
 *				 field
 * @q_vector: structure containing interrupt and ring information
 * @skb: the packet
 * @ptp_class: class of the PTP packet
 *
 * This function will be called by the Rx routine of the timestamp for this
 * packet is stored in the buffer.
 */
void ixgbe_ptp_rx_phytstamp_e600(struct ixgbe_q_vector *q_vector,
				 struct sk_buff *skb, unsigned int ptp_class)
{
	struct ixgbe_rx_phy_ts_skb_e610 *tstamp, *n, *rx_skb;
	struct ixgbe_adapter *adapter = q_vector->adapter;
	struct ptp_header *hdr;
	unsigned long flags;
	__le64 val;
	u16 seq_id;

	q_vector->ptp_hold_rx_skb = false;
	/* Copy the bits out of the skb, and then trim the skb length. */
	skb_copy_bits(skb, skb->len - IXGBE_TS_HDR_LEN, &val, IXGBE_TS_HDR_LEN);
	__pskb_trim(skb, skb->len - IXGBE_TS_HDR_LEN);

	skb_reset_mac_header(skb);
	hdr = ptp_parse_header(skb, ptp_class);
	if (!hdr) {
		u64 ns = le64_to_cpu(val);
		struct timespec64 ts = {
			.tv_sec = upper_32_bits(ns),
			.tv_nsec = lower_32_bits(ns)
		};

		spin_lock_irqsave(&adapter->tmreg_lock, flags);
		ns = timecounter_cyc2time(&adapter->hw_tc,
					  timespec64_to_ns(&ts));
		spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

		skb_hwtstamps(skb)->hwtstamp = ns_to_ktime(ns);
		return;
	}

	q_vector->ptp_hold_rx_skb = true;
	skb_hwtstamps(skb)->hwtstamp = 0;
	seq_id = ntohs(hdr->sequence_id);

	/* Try to find matching timestamp. If not found, cache the skb. */
	spin_lock_irqsave(&adapter->ptp.rx_tstamps_lock, flags);
	list_for_each_entry_safe(tstamp, n, &adapter->ptp.rx_tstamps, list) {
		/* Flush timestamps without skbs after 500 ms. */
		if (time_is_before_jiffies(tstamp->acquired +
					   msecs_to_jiffies(500))) {
			list_del(&tstamp->list);
			kfree(tstamp);
			continue;
		}

		if (seq_id != tstamp->seq_id)
			continue;

		skb_hwtstamps(skb)->hwtstamp = ns_to_ktime(tstamp->ns);
		list_del(&tstamp->list);
		kfree(tstamp);
		q_vector->ptp_hold_rx_skb = false;

		break;
	}
	spin_unlock_irqrestore(&adapter->ptp.rx_tstamps_lock, flags);

	if (!q_vector->ptp_hold_rx_skb)
		return;

	/* Add skb to the list. */
	rx_skb = kzalloc(sizeof(*rx_skb), GFP_KERNEL);
	if (!rx_skb)
		return;

	rx_skb->skb = skb;
	rx_skb->acquired = jiffies;
	rx_skb->seq_id = seq_id;
	spin_lock_irqsave(&q_vector->ptp_skbs_lock_e600, flags);
	list_add_tail(&rx_skb->list, &q_vector->ptp_skbs_e600);
	spin_unlock_irqrestore(&q_vector->ptp_skbs_lock_e600, flags);
}

/**
 * ixgbe_ptp_by_phy_set_params_e600 - configure PTP by PHY parameters
 * @adapter: pointer to the adapter structure
 */
static void ixgbe_ptp_by_phy_set_params_e600(struct ixgbe_adapter *adapter)
{
	u8 ptp_request, flags;
	s32 status;

	ptp_request = IXGBE_SET_PTP_BY_PHY_PTP_REQ_SET_PHY_PARAMS;
	if (adapter->ptp.vlan_ena)
		flags = FIELD_PREP(IXGBE_SET_PTP_BY_PHY_ETHERTYPE_M,
				   IXGBE_SET_PTP_BY_PHY_ETHERTYPE_VLAN_TAG);
	else
		flags = FIELD_PREP(IXGBE_SET_PTP_BY_PHY_ETHERTYPE_M,
				   IXGBE_SET_PTP_BY_PHY_ETHERTYPE_NO_VLAN_TAG);

	status = ixgbe_set_ptp_by_phy(&adapter->hw, ptp_request, flags);
	if (status)
		e_dev_err("PTP by PHY set PHY params failed, status=%d\n",
			  status);
}

/**
 * ixgbe_do_aux_work - do PTP periodic work
 * @info: Driver's PTP info structure
 *
 * Do periodic PTP work for PTP by PHY feature. This consists of:
 * - updating cached PHC time at least once per 4.3 seconds,
 * - setting PTP by PHY parameters on VLAN state change, which cannot be
 *   performed in atomic context,
 * - reinitializing PHY ToD on PHY ToD drift interrupt, which cannot be
 *   performed in atomic context.
 *
 * Reschedule work every second to have a safe margin and to ensure that cached
 * PHC time never goes stale, even with OS scheduling delays.
 *
 * Return: delay of the next auxiliary work scheduling time (>=0) or negative
 *         value in case further scheduling is not required
 */
static long ixgbe_do_aux_work(struct ptp_clock_info *info)
{
	struct ixgbe_adapter *adapter = container_of(info, struct ixgbe_adapter,
						     ptp_caps);
	struct timespec64 ts;

	ixgbe_ptp_gettime_e600(info, &ts);

	if (atomic_cmpxchg(&adapter->ptp.vlan_change, true, false))
		ixgbe_ptp_by_phy_set_params_e600(adapter);

	if (atomic_cmpxchg(&adapter->ptp.reinit_phy_tod, true, false)) {
		u8 request = IXGBE_SET_PTP_BY_PHY_PTP_REQ_TOD_INIT;
		s32 status;

		status = ixgbe_set_ptp_by_phy(&adapter->hw, request, 0);
		if (status)
			e_dev_err("PTP by PHY ToD reinit failed, status=%d\n",
				  status);
	}

	return msecs_to_jiffies(MSEC_PER_SEC);
}

#ifndef HAVE_PTP_CANCEL_WORKER_SYNC
static void ptp_aux_kworker(struct kthread_work *work)
{
	struct ixgbe_ptp_e600 *ptp = container_of(work, struct ixgbe_ptp_e600,
						  ptp_aux_work.work);
	struct ixgbe_adapter *adapter = container_of(ptp, struct ixgbe_adapter,
						     ptp);
	struct ptp_clock_info *info = &adapter->ptp_caps;
	long delay;

	delay = ixgbe_do_aux_work(info);

	if (delay >= 0)
		kthread_queue_delayed_work(ptp->ptp_kworker, &ptp->ptp_aux_work,
					   delay);
}

#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */
/**
 * ixgbe_ptp_cfg_phy_vlan_e600 - configure PTP vlan tag timestamping on PHY
 * @adapter: pointer to the adapter structure
 * @enabled: true to indicate to the PHY that VLAN is enabled, false otherwise
 */
void ixgbe_ptp_cfg_phy_vlan_e600(struct ixgbe_adapter *adapter, bool enabled)
{
	if (adapter->ptp.vlan_ena == enabled)
		return;

	adapter->ptp.vlan_ena = enabled;
	if (!adapter->ptp.ptp_by_phy_ena)
		return;

	/* Schedule worker to handle VLAN change. */
	atomic_set(&adapter->ptp.vlan_change, true);
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
	ptp_schedule_worker(adapter->ptp_clock, 0);
#else /* HAVE_PTP_CANCEL_WORKER_SYNC */
	kthread_mod_delayed_work(adapter->ptp.ptp_kworker,
				 &adapter->ptp.ptp_aux_work, 0);
#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */
}

/**
 * ixgbe_ptp_set_timestamp_mode_e600 - setup the hardware for the requested mode
 * @adapter: the private ixgbe adapter structure
 * @config: the hwtstamp configuration requested
 *
 * Outgoing time stamping can be enabled and disabled. Play nice and
 * disable it when requested, although it shouldn't cause any overhead
 * when no packet needs it. At most one packet in the queue may be
 * marked for time stamping, otherwise it would be impossible to tell
 * for sure to which packet the hardware time stamp belongs.
 *
 * Incoming time stamping has to be configured via the hardware
 * filters. Not all combinations are supported, in particular event
 * type has to be specified. Matching the kind of event packet is
 * not supported, with the exception of "all V2 events regardless of
 * level 2 or 4".
 *
 * Since hardware always timestamps Path delay packets when timestamping V2
 * packets, regardless of the type specified in the register, only use V2
 * Event mode. This more accurately tells the user what the hardware is going
 * to do anyways.
 *
 * Note: this may modify the hwtstamp configuration towards a more general
 * mode, if required to support the specifically requested mode.
 *
 * Return: 0 on success, -EINVAL on invalid flags, -ERANGE on invalid filter.
 */
int ixgbe_ptp_set_timestamp_mode_e600(struct ixgbe_adapter *adapter,
				      struct hwtstamp_config *config)
{
	int rx_mtrl_filter = HWTSTAMP_FILTER_NONE;
	struct ixgbe_hw *hw = &adapter->hw;
	u32 val;

	/* Reserved for future extensions. */
	if (config->flags)
		return -EINVAL;

	/* Enable/disable TX. */
	val = IXGBE_READ_REG(hw, IXGBE_TSYNCTXCTL);
	switch (config->tx_type) {
	case HWTSTAMP_TX_OFF:
		val &= ~IXGBE_TSYNCTXCTL_ENABLED;
		break;
	case HWTSTAMP_TX_ON:
		val |= IXGBE_TSYNCTXCTL_ENABLED;
		break;
	default:
		return -ERANGE;
	}
	IXGBE_WRITE_REG(hw, IXGBE_TSYNCTXCTL, val);

	/* Enable/disable RX. */
	val = IXGBE_READ_REG(hw, IXGBE_TSYNCRXCTL);
	switch (config->rx_filter) {
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
#ifdef HAVE_HWTSTAMP_FILTER_NTP_ALL
	case HWTSTAMP_FILTER_NTP_ALL:
#endif /* HAVE_HWTSTAMP_FILTER_NTP_ALL */
	case HWTSTAMP_FILTER_ALL:
		/* Per-packet timestamping only works if the filter is set to
		 * all packets. Since this is desired, always timestamp
		 * all packets as long as any Rx filter was configured.
		 *
		 * Enable timestamping all packets only if at least some packets
		 * were requested. Otherwise, play nice and disable
		 * timestamping.
		 */
		val = IXGBE_TSYNCRXCTL_ENABLED |
		      IXGBE_TSYNCRXCTL_TYPE_ALL |
		      IXGBE_TSYNCRXCTL_TSIP_UT_EN;

		/* Cache original Rx filter for RXMTRL. */
		rx_mtrl_filter = config->rx_filter;
		config->rx_filter = HWTSTAMP_FILTER_ALL;
		adapter->flags |= IXGBE_FLAG_RX_HWTSTAMP_ENABLED;
		adapter->flags &= ~IXGBE_FLAG_RX_HWTSTAMP_IN_REGISTER;
		break;
	case HWTSTAMP_FILTER_NONE:
	default:
		val &= ~(IXGBE_TSYNCRXCTL_ENABLED | IXGBE_TSYNCRXCTL_TYPE_MASK);
		adapter->flags &= ~(IXGBE_FLAG_RX_HWTSTAMP_ENABLED |
				    IXGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
		if (config->rx_filter != HWTSTAMP_FILTER_NONE) {
			/* Register RXMTRL must be set in order to do V1
			 * packets, therefore it is not possible to timestamp
			 * both V1 Sync and Delay_Req messages unless hardware
			 * supports timestamping all packets => return error.
			 */
			config->rx_filter = HWTSTAMP_FILTER_NONE;
			return -ERANGE;
		}
	}
	IXGBE_WRITE_REG(hw, IXGBE_TSYNCRXCTL, val);

	/* Define which PTP packets are timestamped. */
	switch (rx_mtrl_filter) {
	case HWTSTAMP_FILTER_NONE:
		val = 0;
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		val = IXGBE_RXMTRL_V1_SYNC_MSG | PTP_EV_PORT << 16;
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		val = IXGBE_RXMTRL_V1_DELAY_REQ_MSG | PTP_EV_PORT << 16;
		break;
	default:
		val = PTP_EV_PORT << 16;
	}
	IXGBE_WRITE_REG(hw, IXGBE_RXMTRL, val);

	/* Define ethertype filter for timestamping L2 packets. */
	if (config->rx_filter != HWTSTAMP_FILTER_NONE)
		IXGBE_WRITE_REG(hw, IXGBE_ETQF(IXGBE_ETQF_FILTER_1588),
				(IXGBE_ETQF_FILTER_EN |
				 IXGBE_ETQF_1588 |
				 ETH_P_1588));
	else
		IXGBE_WRITE_REG(hw, IXGBE_ETQF(IXGBE_ETQF_FILTER_1588), 0);

	IXGBE_WRITE_FLUSH(hw);

	ixgbe_ptp_clear_tx_timestamp_e600(adapter);
	return 0;
}

/**
 * ixgbe_ptp_link_up_e600 - perform PTP adjustments on a link up
 * @adapter: pointer to the adapter structure
 */
void ixgbe_ptp_link_up_e600(struct ixgbe_adapter *adapter)
{
	if (adapter->hw.dev_caps.common_cap.ptp_by_phy_ll) {
		ixgbe_ptp_cfg_phy_timestamping_e610(adapter);
		/* Sleep for at least 2 seconds for the PHY to reset, so that
		 * the driver does not report any link flaps.
		 */
		msleep(MSEC_PER_SEC * 3);
	}
}

static const struct cyclecounter ixgbe_ptp_cc_e600 = {
	.read	= ixgbe_ptp_read_phc_e600,
	.mask	= CYCLECOUNTER_MASK(32),
	.mult	= IXGBE_E600_CC_MULT,
	.shift	= IXGBE_E600_CC_SHIFT,
};

/**
 * ixgbe_ptp_start_clock_e600 - create the cycle counter from hw
 * @adapter: pointer to the adapter structure
 *
 * This function should be called to set the proper values for the TIMINCA
 * register and tell the cyclecounter structure what the tick rate of SYSTIME
 * is. It does not directly modify SYSTIME registers or the timecounter
 * structure. It should be called whenever a new TIMINCA value is necessary,
 * such as during initialization or when the link speed changes.
 */
static void ixgbe_ptp_start_clock_e600(struct ixgbe_adapter *adapter)
{
	struct ixgbe_hw *hw = &adapter->hw;
	unsigned long flags;
	u32 tsauxc;

	adapter->hw_cc = ixgbe_ptp_cc_e600;
	/* enable SYSTIME counter */
	tsauxc = IXGBE_READ_REG(hw, IXGBE_TSAUXC);
	IXGBE_WRITE_REG(hw, IXGBE_TSAUXC,
			tsauxc | IXGBE_TSAUXC_DISABLE_SYSTIME);
	IXGBE_WRITE_REG(hw, IXGBE_TIMINCA, 0);
	IXGBE_WRITE_REG(hw, IXGBE_SYSTIMR, 0);
	IXGBE_WRITE_REG(hw, IXGBE_SYSTIML, 0);
	IXGBE_WRITE_REG(hw, IXGBE_SYSTIMH, 0);
	IXGBE_WRITE_REG(hw, IXGBE_TSAUXC,
			tsauxc & ~IXGBE_TSAUXC_DISABLE_SYSTIME);
	IXGBE_WRITE_REG(hw, IXGBE_TSIM, IXGBE_TSIM_TXTS);
	IXGBE_WRITE_REG(hw, IXGBE_EIMS, IXGBE_EIMS_TIMESYNC);
	IXGBE_WRITE_FLUSH(hw);

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_init(&adapter->hw_tc, &adapter->hw_cc,
			 ktime_get_real_ns());
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
}

/**
 * ixgbe_ptp_reset_e600
 * @adapter: the ixgbe private board structure
 *
 * When the MAC resets, all the hardware bits for timesync are reset. This
 * function is used to re-enable the device for PTP based on current settings.
 * We do lose the current clock time, so just reset the cyclecounter to the
 * system real clock time.
 *
 * This function will maintain hwtstamp_config settings, and resets the SDP
 * output if it was enabled.
 */
void ixgbe_ptp_reset_e600(struct ixgbe_adapter *adapter)
{
	/* reset the hardware timestamping mode */
	ixgbe_ptp_start_clock_e600(adapter);
	ixgbe_ptp_set_timestamp_mode_e600(adapter, &adapter->tstamp_config);
}

/**
 * ixgbe_ptp_create_clock - create a PTP clock device
 * @adapter: the ixgbe private adapter structure
 *
 * This function performs setup of the user entry point function table and
 * initializes the PTP clock device, which is used to access the clock-like
 * features of the PTP core. It will be called by ixgbe_ptp_init, and may
 * reuse a previously initialized clock (such as during a suspend/resume
 * cycle).
 *
 * Return: 0 on success, negative error code otherwise.
 */
static long ixgbe_ptp_create_clock_e600(struct ixgbe_adapter *adapter)
{
	struct ptp_clock_info *info = &adapter->ptp_caps;
	struct net_device *netdev = adapter->netdev;
	long err;

	/* Do nothing if we already have a clock device. */
	if (!IS_ERR_OR_NULL(adapter->ptp_clock))
		return 0;

	snprintf(adapter->ptp_caps.name, 16, "%s", netdev->name);
	info->owner = THIS_MODULE;
	info->max_adj = 500000000;
#ifdef HAVE_PTP_CLOCK_INFO_ADJFINE
	info->adjfine = ixgbe_ptp_adjfine_e600;
#else
	info->adjfreq = ixgbe_ptp_adjfreq_e600;
#endif
	info->adjtime = ixgbe_ptp_adjtime_e600;
#ifdef HAVE_PTP_CLOCK_INFO_GETTIME64
	info->gettime64 = ixgbe_ptp_gettime_e600;
	info->settime64 = ixgbe_ptp_settime_e600;
#else /* HAVE_PTP_CLOCK_INFO_GETTIME64 */
	info->gettime = ixgbe_ptp_gettime32_e600;
	info->settime = ixgbe_ptp_settime32_e600;
#endif /* !HAVE_PTP_CLOCK_INFO_GETTIME64 */
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
	info->do_aux_work = ixgbe_do_aux_work;
#endif /* HAVE_PTP_CANCEL_WORKER_SYNC */
	info->n_pins = 1;
	info->n_per_out = 1;
	info->pin_config = adapter->ptp.pin_config;
	info->enable = ixgbe_ptp_gpio_enable_e600;
	info->verify = ixgbe_ptp_verify_pin_e600;

	adapter->ptp_clock = ptp_clock_register(&adapter->ptp_caps,
						ixgbe_pf_to_dev(adapter));
	if (IS_ERR(adapter->ptp_clock)) {
		err = PTR_ERR(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_err("ptp_clock_register failed\n");
		return err;
	} else if (adapter->ptp_clock) {
		e_dev_info("Registered PHC device on %s\n", netdev->name);
	}

	return 0;
}

/**
 * ixgbe_ptp_init_e600 - initialize PTP support on HW and SW
 * @adapter: the ixgbe private adapter structure
 *
 * This function performs the required steps for enabling PTP support.
 */
void ixgbe_ptp_init_e600(struct ixgbe_adapter *adapter)
{
	adapter->tstamp_config.rx_filter = HWTSTAMP_FILTER_NONE;
	adapter->tstamp_config.tx_type = HWTSTAMP_TX_OFF;

	adapter->ptp.pin_config[0] = e600_pin_cfg[0];
	spin_lock_init(&adapter->tmreg_lock);
	INIT_LIST_HEAD(&adapter->ptp.rx_tstamps);
	spin_lock_init(&adapter->ptp.rx_tstamps_lock);

	/* Obtain a PTP device, or re-use an existing device. */
	if (ixgbe_ptp_create_clock_e600(adapter))
		return;

	INIT_WORK(&adapter->ptp_tx_work, ixgbe_ptp_tx_hwtstamp_work);

#ifndef HAVE_PTP_CANCEL_WORKER_SYNC
	kthread_init_delayed_work(&adapter->ptp.ptp_aux_work, ptp_aux_kworker);
	adapter->ptp.ptp_kworker = kthread_create_worker(0, "ixgbe_ptp");
	if (IS_ERR(adapter->ptp.ptp_kworker)) {
		e_dev_err("Failed to create ptp aux_worker err=%li\n",
			  PTR_ERR(adapter->ptp.ptp_kworker));
		return;
	}
#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */
	ixgbe_ptp_reset_e600(adapter);
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
	ptp_schedule_worker(adapter->ptp_clock, 0);
#else /* HAVE_PTP_CANCEL_WORKER_SYNC */
	kthread_mod_delayed_work(adapter->ptp.ptp_kworker,
				 &adapter->ptp.ptp_aux_work, 0);
#endif /* !HAVE_PTP_CANCEL_WORKER_SYNC */

	set_bit(__IXGBE_PTP_RUNNING, adapter->state);
}

/**
 * ixgbe_ptp_release_e600 - release PTP resources
 * @adapter: pointer to adapter struct
 *
 * Completely destroy the PTP device, should only be called when the device is
 * being fully closed.
 */
void ixgbe_ptp_release_e600(struct ixgbe_adapter *adapter)
{
	struct ptp_perout_request rq = {};

	if (!test_and_clear_bit(__IXGBE_PTP_RUNNING, adapter->state))
		return;

	/* Disable SDP. */
	ixgbe_ptp_setup_sdp_e600(adapter, &rq, false);

	/* Ensure that we cancel any pending PTP Tx work item in progress. */
	cancel_work_sync(&adapter->ptp_tx_work);
#ifdef HAVE_PTP_CANCEL_WORKER_SYNC
	ptp_cancel_worker_sync(adapter->ptp_clock);
#else /* !HAVE_PTP_CANCEL_WORKER_SYNC */
	kthread_cancel_delayed_work_sync(&adapter->ptp.ptp_aux_work);
#endif /* HAVE_PTP_CANCEL_WORKER_SYNC */

	ixgbe_ptp_clear_tx_timestamp_e600(adapter);

	if (adapter->ptp_clock) {
		ptp_clock_unregister(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_info("Removed PHC device on %s\n", adapter->netdev->name);
	}
}
