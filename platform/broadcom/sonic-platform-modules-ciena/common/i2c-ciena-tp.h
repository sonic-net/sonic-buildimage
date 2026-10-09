#undef TRACE_SYSTEM
#define TRACE_SYSTEM ciena_i2c

#if !defined(CIENA_I2C_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define CIENA_I2C_TP_H

#include <linux/i2c.h>
#include <linux/tracepoint.h>

/*
 * Ciena I2C register read
 */
TRACE_EVENT(ciena_i2c_reg_rd,
	    TP_PROTO(int adap, int reg, u32 data),
	    TP_ARGS(adap, reg, data),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(int, reg)
		    __field(u32, data)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr 	= adap;
		    __entry->reg 		= reg;
		    __entry->data 		= data;
		    ),
	    TP_printk("adap=%d reg=%d dat=%x",
		      __entry->adap_nr,
		      __entry->reg,
		      __entry->data
		    )
	);

#define ciena_i2c_opstr(__op)	(				\
		(1   == __op) ? "reset"        :		\
		(2   == __op) ? "start"        :		\
		(4   == __op) ? "stop"         :		\
		(8   == __op) ? "write"        :		\
		(16  == __op) ? "readnack"     :		\
		(32  == __op) ? "readack"      :		\
		(264 == __op) ? "writeignnack" : "???" )
/*
 * Ciena I2C register write
 */
TRACE_EVENT(ciena_i2c_reg_wr,
	    TP_PROTO(int adap, int reg, u32 data),
	    TP_ARGS(adap, reg, data),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(int, reg)
		    __field(u32, data)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr 	= adap;
		    __entry->reg 		= reg;
		    __entry->data 		= data;
		    ),
	    TP_printk("adap=%d reg=%d dat=%x%s%s",
		      __entry->adap_nr,
		      __entry->reg,
		      __entry->data,
		      (0 == __entry->reg) ? " " : "",
		      (0 == __entry->reg) ? ciena_i2c_opstr(__entry->data) : ""
		    )
	);


/*
 * Ciena I2C operation
 */
TRACE_EVENT(ciena_i2c_op,
	    TP_PROTO(int adap, int op, unsigned int retry, int rc),
	    TP_ARGS(adap, op, retry, rc),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(int, op)
		    __field(unsigned int, retry)
		    __field(int, rc)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr 	= adap;
		    __entry->op 		= op;
		    __entry->retry 		= retry;
		    __entry->rc 		= rc;
		    ),
	    TP_printk("adap=%d op=%d rty=%d rc=%d %s",
		      __entry->adap_nr,
		      __entry->op,
		      __entry->retry,
		      __entry->rc,
		      ciena_i2c_opstr(__entry->op)
		    )
	);

/*
 * Ciena I2C message read
 */
TRACE_EVENT(ciena_i2c_msg_rd,
	    TP_PROTO(int adap, const struct i2c_msg *msg, int rc, u16 numb),
	    TP_ARGS(adap, msg, rc, numb),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(u16, addr)
		    __field(u16, flags)
		    __field(u16, len)
		    __field(int, rc)
		    __field(u16, numb)
		    __dynamic_array(u8, buf, msg->len)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->addr 		= msg->addr;
		    __entry->flags 		= msg->flags;
		    __entry->len 		= msg->len;
		    __entry->rc 		= rc;
		    __entry->numb 		= numb;
		    memcpy(__get_dynamic_array(buf), msg->buf, msg->len);
		    ),
	    TP_printk("adap=%d add=%04x flg=%04x len=%d rc=%d nmb=%d [%*phD]",
		      __entry->adap_nr,
		      __entry->addr,
		      __entry->flags,
		      __entry->len,
		      __entry->rc,
		      __entry->numb,
		      __entry->len, __get_dynamic_array(buf)
		    )
	);

/*
 * Ciena I2C message write
 */
TRACE_EVENT(ciena_i2c_msg_wr,
	    TP_PROTO(int adap, const struct i2c_msg *msg, int rc, u16 numb),
	    TP_ARGS(adap, msg, rc, numb),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(u16, addr)
		    __field(u16, flags)
		    __field(u16, len)
		    __field(int, rc)
		    __field(u16, numb)
		    __dynamic_array(u8, buf, msg->len)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->addr 		= msg->addr;
		    __entry->flags 		= msg->flags;
		    __entry->len 		= msg->len;
		    __entry->rc 		= rc;
		    __entry->numb 		= numb;
		    memcpy(__get_dynamic_array(buf), msg->buf, msg->len);
		    ),
	    TP_printk("adap=%d add=%04x flg=%04x len=%d rc=%d nmb=%d [%*phD]",
		      __entry->adap_nr,
		      __entry->addr,
		      __entry->flags,
		      __entry->len,
		      __entry->rc,
		      __entry->numb,
		      __entry->len, __get_dynamic_array(buf)
		    )
	);


/*
 * Ciena I2C acknowledge polling
 */
TRACE_EVENT(ciena_i2c_ack_poll,
	    TP_PROTO(int adap, unsigned int addr, int rc),
	    TP_ARGS(adap, addr, rc),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(u16, addr)
		    __field(int, rc)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->addr 		= addr;
		    __entry->rc 		= rc;
		    ),
	    TP_printk("adap=%d add=%04x rc=%d",
		      __entry->adap_nr,
		      __entry->addr,
		      __entry->rc
		    )
	);


/*
 * Ciena I2C master_xfer_entry
 */
TRACE_EVENT(ciena_i2c_xfer_entry,
	    TP_PROTO(int adap, struct i2c_msg *msgs, int num),
	    TP_ARGS(adap, msgs, num),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(void *, msgs)
		    __field(int, num)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->msgs 		= msgs;
		    __entry->num 		= num;
		    ),
	    TP_printk("adap=%d msgs=%p num=%d",
		      __entry->adap_nr,
		      __entry->msgs,
		      __entry->num
		    )
	);

/*
 * Ciena I2C master_xfer_exit
 */
TRACE_EVENT(ciena_i2c_xfer_exit,
	    TP_PROTO(int adap, struct i2c_msg *msgs, int num, int rc),
	    TP_ARGS(adap, msgs, num, rc),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(void *, msgs)
		    __field(int, num)
		    __field(int, rc)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->msgs 		= msgs;
		    __entry->num 		= num;
		    __entry->rc 		= rc;
		    ),
	    TP_printk("adap=%d msgs=%p num=%d rc=%d",
		      __entry->adap_nr,
		      __entry->msgs,
		      __entry->num,
		      __entry->rc
		    )
	);

/*
 * Ciena I2C high latency
 */
TRACE_EVENT(ciena_i2c_latency,
	    TP_PROTO(int adap, s64 gap),
	    TP_ARGS(adap, gap),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(s64, gap)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->gap 		= gap;
		    ),
	    TP_printk("adap=%d gap=%lld",
		      __entry->adap_nr,
		      __entry->gap
		    )
	);

/*
 * Ciena I2C priority bump
 */
TRACE_EVENT(ciena_i2c_prio_bump,
	    TP_PROTO(int adap, int wait_done),
	    TP_ARGS(adap, wait_done),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(int, wait_done)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr 	= adap;
		    __entry->wait_done 	= wait_done;
		    ),
	    TP_printk("adap=%d wait_done=%d",
		      __entry->adap_nr,
		      __entry->wait_done
		    )
	);

/*
 * Ciena I2C interrupt
 */
TRACE_EVENT(ciena_i2c_irq,
	    TP_PROTO(int adap, int wait_done),
	    TP_ARGS(adap, wait_done),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(int, wait_done)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr 	= adap;
		    __entry->wait_done 	= wait_done;
		    ),
	    TP_printk("adap=%d wait_done=%d",
		      __entry->adap_nr,
		      __entry->wait_done
		    )
	);

/*
 * Ciena I2C pre-start gap
 */
TRACE_EVENT(ciena_i2c_pre_start,
	    TP_PROTO(int adap, s64 gap),
	    TP_ARGS(adap, gap),
	    TP_STRUCT__entry(
		    __field(int, adap_nr)
		    __field(s64, gap)
		    ),
	    TP_fast_assign(
		    __entry->adap_nr		= adap;
		    __entry->gap 		= gap;
		    ),
	    TP_printk("adap=%d gap=%lld",
		      __entry->adap_nr,
		      __entry->gap
		    )
	);

#endif /* CIENA_I2C_TP_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE i2c-ciena-tp
#include <trace/define_trace.h>

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
