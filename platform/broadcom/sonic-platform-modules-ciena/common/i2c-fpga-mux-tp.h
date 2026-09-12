#undef TRACE_SYSTEM
#define TRACE_SYSTEM ciena_i2c

#if !defined(CIENA_I2C_MUX_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define CIENA_I2C_MUX_TP_H

#include <linux/i2c.h>
#include "i2c-fpga-mux.h"
#include "i2c-fpga-mux-priv.h"
#include <linux/tracepoint.h>

/*
 * Ciena I2C mux_select()
 */
TRACE_EVENT(ciena_i2c_mux_select_chan,
			TP_PROTO(const struct i2c_fpga_mux_priv *priv, u32 chan, u32 newchan),
			TP_ARGS(priv, chan, newchan),
			TP_STRUCT__entry(
				__field(unsigned, base_nr)
				__field(int, adap_nr)
				__field(u32, chan)
				__field(u32, newchan)
				),
			TP_fast_assign(
				__entry->base_nr 	= priv->base_nr;
				__entry->adap_nr 	= priv->parent->nr;
				__entry->chan 		= chan;
				__entry->newchan 	= newchan;
				),
			TP_printk("base=%u adap=%d chan=%u newchan=%u",
					  __entry->base_nr,
					  __entry->adap_nr,
					  __entry->chan,
					  __entry->newchan
				)
	);
TRACE_EVENT(ciena_i2c_mux_select_debounce_finished,
			TP_PROTO(const struct i2c_fpga_mux_priv *priv, u32 chan, u32 dly_us),
			TP_ARGS(priv, chan, dly_us),
			TP_STRUCT__entry(
				__field(unsigned, base_nr)
				__field(int, adap_nr)
				__field(u32, chan)
				__field(u32, dly_us)
				),
			TP_fast_assign(
				__entry->base_nr 	= priv->base_nr;
				__entry->adap_nr 	= priv->parent->nr;
				__entry->chan 		= chan;
				__entry->dly_us 	= dly_us;
				),
			TP_printk("base=%u adap=%d chan=%u dly_us=%u",
					  __entry->base_nr,
					  __entry->adap_nr,
					  __entry->chan,
					  __entry->dly_us
				)
	);

/*
 * Ciena I2C mux_deselect()
 */
TRACE_EVENT(ciena_i2c_mux_deselect_debounce_finished,
			TP_PROTO(const struct i2c_fpga_mux_priv *priv, u32 chan, u32 dly_us),
			TP_ARGS(priv, chan, dly_us),
			TP_STRUCT__entry(
				__field(unsigned, base_nr)
				__field(int, adap_nr)
				__field(u32, chan)
				__field(u32, dly_us)
				),
			TP_fast_assign(
				__entry->base_nr 	= priv->base_nr;
				__entry->adap_nr 	= priv->parent->nr;
				__entry->chan 		= chan;
				__entry->dly_us 	= dly_us;
				),
			TP_printk("base=%u adap=%d chan=%u dly_us=%u",
					  __entry->base_nr,
					  __entry->adap_nr,
					  __entry->chan,
					  __entry->dly_us
				)
	);
TRACE_EVENT(ciena_i2c_mux_deselect_parked,
			TP_PROTO(const struct i2c_fpga_mux_priv *priv, u32 chan),
			TP_ARGS(priv, chan),
			TP_STRUCT__entry(
				__field(unsigned, base_nr)
				__field(int, adap_nr)
				__field(u32, chan)
				),
			TP_fast_assign(
				__entry->base_nr 	= priv->base_nr;
				__entry->adap_nr 	= priv->parent->nr;
				__entry->chan 		= chan;
				),
			TP_printk("base=%u adap=%d chan=%u",
					  __entry->base_nr,
					  __entry->adap_nr,
					  __entry->chan
				)
	);

#endif /* CIENA_I2C_MUX_TP_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE i2c-fpga-mux-tp
#include <trace/define_trace.h>

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
