/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2023 Advanced Micro Devices, Inc */

/*
 * See for references
 *  https://www.kernel.org/doc/html/latest/trace/tracepoints.html
 *  http://lwn.net/Articles/379903
 *  http://lwn.net/Articles/381064
 *  http://lwn.net/Articles/383362
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM ionic

#if !defined(_IONIC_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _IONIC_TRACE_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(ionic_q_start_stop_template,
	TP_PROTO(struct ionic_queue *q),

	TP_ARGS(q),

	TP_STRUCT__entry(
		__field(u16, index)
		__string(devname, q->lif->netdev->name)
	),

	TP_fast_assign(
		__entry->index = q->index;
		__assign_str(devname);
	),

	TP_printk("%s: queue[%u]", __get_str(devname), __entry->index)
);

DEFINE_EVENT(ionic_q_start_stop_template, ionic_q_stop,
	     TP_PROTO(struct ionic_queue *q),
	     TP_ARGS(q)
);

DEFINE_EVENT(ionic_q_start_stop_template, ionic_q_start,
	     TP_PROTO(struct ionic_queue *q),
	     TP_ARGS(q)
);

#endif /* _IONIC_TRACE_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE ionic_trace
#include <trace/define_trace.h>

