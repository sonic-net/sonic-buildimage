#undef TRACE_SYSTEM
#define TRACE_SYSTEM ciena_cic

#if !defined(CIENA_CIC_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define CIENA_CIC_TP_H

#include <linux/tracepoint.h>

/*
 * Ciena CIC incoming irq
 */
TRACE_EVENT(ciena_cic_in,
	    TP_PROTO(const char *cic, unsigned virq, u32 master, u32 mask),
	    TP_ARGS(cic, virq, master, mask),
	    TP_STRUCT__entry(
		    __field(unsigned,     virq)
		    __string(nm,          cic)
		    __field(u32,          master)
		    __field(u32,          mask)
		    ),
	    TP_fast_assign(
		    __entry->virq    = virq;
		    __assign_str(nm);
		    __entry->master  = master;
		    __entry->mask    = mask;
		    ),
	    TP_printk("virq=%u %s master=%x/%x",
		      __entry->virq,
		      __get_str(nm),
		      __entry->master,
		      __entry->mask
		    )
	);

/*
 * Ciena CIC outgoing irq
 */
TRACE_EVENT(ciena_cic_out,
	    TP_PROTO(const char *cic, unsigned virq, const char *irqname,
		     u32 mask, const char *regname, int bit, unsigned result),
	    TP_ARGS(cic, virq, irqname, mask, regname, bit, result),
	    TP_STRUCT__entry(
		    __field(unsigned,     virq)
		    __string(nm,          cic)
		    __string(irqnm,       irqname)
		    __field(unsigned,     regval)
		    __field(u32,          mask)
		    __string(regnm,       regname)
		    __field(int,          bit)
		    ),
	    TP_fast_assign(
		    __entry->virq     = virq;
		    __assign_str(nm);
		    __assign_str(irqnm);
		    __entry->regval   = result;
		    __entry->mask     = mask;
		    __assign_str(regnm);
		    __entry->bit      = bit;
		    ),
	    TP_printk("virq=%u %s %s %s.%d reg=%x/%x",
		      __entry->virq,
		      __get_str(nm),
		      __get_str(irqnm),
		      __get_str(regnm),
		      __entry->bit,
		      __entry->regval,
		      __entry->mask
		    )
	);

/*
 * Ciena CIC register read
 */
TRACE_EVENT(ciena_cic_reg_rd,
	    TP_PROTO(const char *cic, const char *name, const char *suffix,
		     unsigned reg, unsigned val),
	    TP_ARGS(cic, name, suffix, reg, val),
	    TP_STRUCT__entry(
		    __field(unsigned, reg)
		    __field(unsigned, val)
		    __field(bool,     nosuf)
		    __string(nm,      cic)
		    __string(regnm,   name)
		    __string(suffix,  suffix ?: "")
		    ),
	    TP_fast_assign(
		    __entry->reg       = reg;
		    __entry->val       = val;
		    __entry->nosuf     = (NULL == suffix);
		    __assign_str(nm);
		    __assign_str(regnm);
		    __assign_str(suffix);
		    ),
	    TP_printk("%s %s%s%s reg=%x val=%x",
		      __get_str(nm),
		      __get_str(regnm),
		      __entry->nosuf ? "" : "_",
		      __get_str(suffix),
		      __entry->reg,
		      __entry->val
		    )
	);

/*
 * Ciena CIC register write
 */
TRACE_EVENT(ciena_cic_reg_wr,
	    TP_PROTO(const char *cic, const char *name, const char *suffix,
		     unsigned reg, unsigned val),
	    TP_ARGS(cic, name, suffix, reg, val),
	    TP_STRUCT__entry(
		    __field(unsigned, reg)
		    __field(unsigned, val)
		    __field(bool,     nosuf)
		    __string(nm,      cic)
		    __string(regnm,   name)
		    __string(suffix,  suffix ?: "")
		    ),
	    TP_fast_assign(
		    __entry->reg       = reg;
		    __entry->val       = val;
		    __entry->nosuf     = (NULL == suffix);
		    __assign_str(nm);
		    __assign_str(regnm);
		    __assign_str(suffix);
		    ),
	    TP_printk("%s %s%s%s reg=%x val=%x",
		      __get_str(nm),
		      __get_str(regnm),
		      __entry->nosuf ? "" : "_",
		      __get_str(suffix),
		      __entry->reg,
		      __entry->val
		    )
	);

#ifdef CREATE_TRACE_POINTS
EXPORT_TRACEPOINT_SYMBOL_GPL(ciena_cic_in);
EXPORT_TRACEPOINT_SYMBOL_GPL(ciena_cic_out);
EXPORT_TRACEPOINT_SYMBOL_GPL(ciena_cic_reg_rd);
EXPORT_TRACEPOINT_SYMBOL_GPL(ciena_cic_reg_wr);
#endif

#endif /* CIENA_CIC_TP_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE generic_cic_tp
#include <trace/define_trace.h>

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
