#undef TRACE_SYSTEM
#define TRACE_SYSTEM ciena_i2c

#if !defined(CIENA_I2C_SMB_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define CIENA_I2C_SMB_TP_H

#include <linux/i2c.h>
#include <linux/tracepoint.h>

/*
 * Ciena I2C-SMB write
 */
TRACE_EVENT(ciena_i2c_smb_write,
	    TP_PROTO(int a, __u16 d, __u16 f, __u8 w, __u16 l, __u8 *b, int r),
	    TP_ARGS(a, d, f, w, l, b, r),
	    TP_STRUCT__entry(
		    __field(int,     adapter_nr  )
		    __field(__u16,   dev_addr    )
		    __field(__u16,   len         )
		    __field(int,     rc          )
		    __field(__u16,   fake_addr   )
		    __field(__u8,    fake_addr_w )
		    __dynamic_array(__u8, buf, l)
		    ),
	    TP_fast_assign(
		    __entry->adapter_nr  = a;
		    __entry->dev_addr    = d;
		    __entry->len         = l;
		    __entry->rc          = r;
		    __entry->fake_addr   = (f >> 8) | (f << 8);
		    __entry->fake_addr_w = w;
		    memcpy(__get_dynamic_array(buf), b, l);
		    ),
	    TP_printk("adap=%d da=%03x fa=%*phN [%*phD] rc=%d",
		      __entry->adapter_nr,
		      __entry->dev_addr,
		      __entry->fake_addr_w, &__entry->fake_addr,
		      __entry->len, __get_dynamic_array(buf),
		      __entry->rc
		    )
	);
/*
 * Ciena I2C-SMB read
 */
TRACE_EVENT(ciena_i2c_smb_read,
	    TP_PROTO(int a, __u16 d, __u16 f, __u8 w, __u16 l, __u8 *b, int r),
	    TP_ARGS(a, d, f, w, l, b, r),
	    TP_STRUCT__entry(
		    __field(int,     adapter_nr  )
		    __field(__u16,   dev_addr    )
		    __field(__u16,   len         )
		    __field(int,     rc          )
		    __field(__u16,   fake_addr   )
		    __field(__u8,    fake_addr_w )
		    __dynamic_array(__u8, buf, l)
		    ),
	    TP_fast_assign(
		    __entry->adapter_nr  = a;
		    __entry->dev_addr    = d;
		    __entry->len         = l;
		    __entry->rc          = r;
		    __entry->fake_addr   = (f >> 8) | (f << 8);
		    __entry->fake_addr_w = w;
		    memcpy(__get_dynamic_array(buf), b, l);
		    ),
	    TP_printk("adap=%d da=%03x fa=%*phN [%*phD] rc=%d",
		      __entry->adapter_nr,
		      __entry->dev_addr,
		      __entry->fake_addr_w, &__entry->fake_addr,
		      __entry->len, __get_dynamic_array(buf),
		      __entry->rc
		    )
	);

/*
 * Ciena I2C-SMB ACK poll
 */
TRACE_EVENT(ciena_i2c_smb_ack_poll,
	    TP_PROTO(int a, __u16 d, __u16 c),
	    TP_ARGS(a, d, c),
	    TP_STRUCT__entry(
		    __field(int,     adapter_nr  )
		    __field(__u16,   dev_addr    )
		    __field(__u16,   command     )
		    ),
	    TP_fast_assign(
		    __entry->adapter_nr  = a;
		    __entry->dev_addr    = d;
		    __entry->command     = c;
		    ),
	    TP_printk("adap=%d da=%03x com=%02x",
		      __entry->adapter_nr,
		      __entry->dev_addr,
		      __entry->command
		    )
	);


#endif /* CIENA_I2C_SMB_TP_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE i2c-ciena-smb-tp
#include <trace/define_trace.h>

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
