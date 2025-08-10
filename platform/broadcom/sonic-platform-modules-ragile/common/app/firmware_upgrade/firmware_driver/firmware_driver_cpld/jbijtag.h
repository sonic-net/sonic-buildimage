/****************************************************************************/
/*                                                                          */
/*  Module:         jbijtag.h                                               */
/*                                                                          */
/*                  Copyright (C) Altera Corporation 1998-2001              */
/*                                                                          */
/*  Description:    Definitions of JTAG constants, types, and functions     */
/*                                                                          */
/****************************************************************************/

#ifndef INC_JBIJTAG_H
#define INC_JBIJTAG_H

/****************************************************************************/
/*                                                                          */
/*  Function Prototypes                                                     */
/*                                                                          */
/****************************************************************************/
typedef enum
{
    JBI_ILLEGAL_JTAG_STATE = -1,
    RESET = 0,
    IDLE = 1,
    DRSELECT = 2,
    DRCAPTURE = 3,
    DRSHIFT = 4,
    DREXIT1 = 5,
    DRPAUSE = 6,
    DREXIT2 = 7,
    DRUPDATE = 8,
    IRSELECT = 9,
    IRCAPTURE = 10,
    IRSHIFT = 11,
    IREXIT1 = 12,
    IRPAUSE = 13,
    IREXIT2 = 14,
    IRUPDATE = 15

} JBIE_JTAG_STATE;

JBI_RETURN_TYPE jbi_init_jtag
(
    void
);

JBI_RETURN_TYPE jbi_set_drstop_state
(
    JBIE_JTAG_STATE state
);

JBI_RETURN_TYPE jbi_set_irstop_state
(
    JBIE_JTAG_STATE state
);

JBI_RETURN_TYPE jbi_set_dr_preamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *preamble_data
);

JBI_RETURN_TYPE jbi_set_ir_preamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *preamble_data
);

JBI_RETURN_TYPE jbi_set_dr_postamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *postamble_data
);

JBI_RETURN_TYPE jbi_set_ir_postamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *postamble_data
);

JBI_RETURN_TYPE jbi_goto_jtag_state
(
    JBIE_JTAG_STATE state
);

JBI_RETURN_TYPE jbi_do_wait_cycles
(
    long cycles,
    JBIE_JTAG_STATE wait_state
);

JBI_RETURN_TYPE jbi_do_wait_microseconds
(
    long microseconds,
    JBIE_JTAG_STATE wait_state
);

JBI_RETURN_TYPE jbi_do_irscan
(
    unsigned int count,
    unsigned char *tdi_data,
    unsigned int start_index
);

JBI_RETURN_TYPE jbi_swap_ir
(
    unsigned int count,
    unsigned char *in_data,
    unsigned int in_index,
    unsigned char *out_data,
    unsigned int out_index
);

JBI_RETURN_TYPE jbi_do_drscan
(
    unsigned int count,
    unsigned char *tdi_data,
    unsigned long start_index
);

JBI_RETURN_TYPE jbi_swap_dr
(
    unsigned int count,
    unsigned char *in_data,
    unsigned long in_index,
    unsigned char *out_data,
    unsigned int out_index
);

void jbi_free_jtag_padding_buffers
(
    int reset_jtag
);

JBI_RETURN_TYPE jbi_do_drscan_multi_page
(
    unsigned int variable_id,
    unsigned long long_count,
    unsigned long long_index,
    int version
);

#endif /* INC_JBIJTAG_H */
