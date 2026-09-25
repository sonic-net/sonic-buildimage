#undef TRACE_SYSTEM
#define TRACE_SYSTEM sirilx_gnss

#if !defined(SIRILX_GNSS_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define SIRILX_GNSS_TP_H

#include <linux/tracepoint.h>

/*
 * sirilx GNSS reset UART
 */
TRACE_EVENT(sirilx_gnss_reset_uart,
	    TP_PROTO(const char *name, int loops, int usecs, u32 reg),
	    TP_ARGS(name, loops, usecs, reg),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    __field(int,          loops)
		    __field(int,          usecs)
		    __field(u32,          reg)
		    ),
	    TP_fast_assign(
		    __entry->name  = name;
		    __entry->loops = loops;
		    __entry->usecs = usecs;
		    __entry->reg   = reg;
		    ),
	    TP_printk("%s loops=%d usecs=%d reg=%x",
		      __entry->name,
		      __entry->loops,
		      __entry->usecs,
		      __entry->reg
		    )
	);

/*
 * sirilx GNSS receive
 */
TRACE_EVENT(sirilx_gnss_rx,
	    TP_PROTO(const char *name, u32 depth, int overrun, u32 rx),
	    TP_ARGS(name, depth, overrun, rx),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    __field(u32,          depth)
		    __field(int,          overrun)
		    __field(u32,          rx)
		    ),
	    TP_fast_assign(
		    __entry->name    = name;
		    __entry->depth   = depth;
		    __entry->overrun = overrun;
		    __entry->rx      = rx;
		    ),
	    TP_printk("%s depth=%u overrun=%d rx=%02x",
		      __entry->name,
		      __entry->depth,
		      __entry->overrun,
		      __entry->rx
		    )
	);

/*
 * sirilx GNSS transmit
 */
TRACE_EVENT(sirilx_gnss_tx,
	    TP_PROTO(const char *name, u32 depth, u32 tx),
	    TP_ARGS(name, depth, tx),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    __field(u32,          depth)
		    __field(u32,          tx)
		    ),
	    TP_fast_assign(
		    __entry->name  = name;
		    __entry->depth = depth;
		    __entry->tx    = tx;
		    ),
	    TP_printk("%s depth=%u tx=%02x",
		      __entry->name,
		      __entry->depth,
		      __entry->tx
		    )
	);

/*
 * sirilx GNSS receive fifo overflow
 */
TRACE_EVENT(sirilx_gnss_rx_overflow,
	    TP_PROTO(const char *name, u32 depth, int overrun),
	    TP_ARGS(name, depth, overrun),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    __field(u32,          depth)
		    __field(int,          overrun)
		    ),
	    TP_fast_assign(
		    __entry->name    = name;
		    __entry->depth   = depth;
		    __entry->overrun = overrun;
		    ),
	    TP_printk("%s depth=%u overrun=%d",
		      __entry->name,
		      __entry->depth,
		      __entry->overrun
		    )
	);

/*
 * sirilx GNSS receive interrupt
 */
TRACE_EVENT(sirilx_gnss_irq,
	    TP_PROTO(const char *name),
	    TP_ARGS(name),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    ),
	    TP_fast_assign(
		    __entry->name = name;
		    ),
	    TP_printk("%s",
		      __entry->name
		    )
	);

/*
 * sirilx GNSS stop rx
 */
TRACE_EVENT(sirilx_gnss_stop_rx,
	    TP_PROTO(const char *name),
	    TP_ARGS(name),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    ),
	    TP_fast_assign(
		    __entry->name = name;
		    ),
	    TP_printk("%s",
		      __entry->name
		    )
	);

/*
 * sirilx GNSS enable UART
 */
TRACE_EVENT(sirilx_gnss_enable_uart,
	    TP_PROTO(const char *name),
	    TP_ARGS(name),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    ),
	    TP_fast_assign(
		    __entry->name = name;
		    ),
	    TP_printk("%s",
		      __entry->name
		    )
	);

/*
 * sirilx GNSS disable UART
 */
TRACE_EVENT(sirilx_gnss_disable_uart,
	    TP_PROTO(const char *name),
	    TP_ARGS(name),
	    TP_STRUCT__entry(
		    __field(const char *, name)
		    ),
	    TP_fast_assign(
		    __entry->name = name;
		    ),
	    TP_printk("%s",
		      __entry->name
		    )
	);

#endif /* SIRILX_GNSS_TP_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE sirilx_gnss_tp
#include <trace/define_trace.h>

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
