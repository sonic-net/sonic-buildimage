/****************************************************************************/
/*                                                                          */
/*  Module:         jbijtag.c                                               */
/*                                                                          */
/*                  Copyright (C) Altera Corporation 1998-2001              */
/*                                                                          */
/*  Description:    Contains JTAG interface functions                       */
/*                                                                          */
/*  Revisions:      2.2  updated state transition paths                     */
/*                  2.0  added multi-page scan code for 16-bit PORT         */
/*                                                                          */
/****************************************************************************/

#include "jbiport.h"
#include "jbiexprt.h"
#include "jbicomp.h"
#include "jbijtag.h"

#define NULL 0

char *jbi_workspace = NULL;
long jbi_workspace_size = 0L;

/****************************************************************************/
/*                                                                          */
/*  Enumerated Types                                                        */
/*                                                                          */
/****************************************************************************/

/* maximum JTAG IR and DR lengths (in bits) */
#define JBIC_MAX_JTAG_IR_PREAMBLE   256
#define JBIC_MAX_JTAG_IR_POSTAMBLE  256
#define JBIC_MAX_JTAG_IR_LENGTH     512
#define JBIC_MAX_JTAG_DR_PREAMBLE  1024
#define JBIC_MAX_JTAG_DR_POSTAMBLE 1024
#define JBIC_MAX_JTAG_DR_LENGTH    2048

/*
*   Global variable to store the current JTAG state
*/
JBIE_JTAG_STATE jbi_jtag_state = JBI_ILLEGAL_JTAG_STATE;

/*
*   Store current stop-state for DR and IR scan commands
*/
JBIE_JTAG_STATE jbi_drstop_state = IDLE;
JBIE_JTAG_STATE jbi_irstop_state = IDLE;

/*
*   Store current padding values
*/
unsigned int jbi_dr_preamble  = 0;
unsigned int jbi_dr_postamble = 0;
unsigned int jbi_ir_preamble  = 0;
unsigned int jbi_ir_postamble = 0;
unsigned int jbi_dr_length    = 0;
unsigned int jbi_ir_length    = 0;
unsigned char *jbi_dr_preamble_data  = NULL;
unsigned char *jbi_dr_postamble_data = NULL;
unsigned char *jbi_ir_preamble_data  = NULL;
unsigned char *jbi_ir_postamble_data = NULL;
unsigned char *jbi_dr_buffer         = NULL;
unsigned char *jbi_ir_buffer         = NULL;

/*
*   This structure shows, for each JTAG state, which state is reached after
*   a single TCK clock cycle with TMS high or TMS low, respectively.  This
*   describes all possible state transitions in the JTAG state machine.
*/
struct JBIS_JTAG_MACHINE
{
    JBIE_JTAG_STATE tms_high;
    JBIE_JTAG_STATE tms_low;
} jbi_jtag_state_transitions[] =
{
/* RESET     */ { RESET,    IDLE },
/* IDLE      */ { DRSELECT, IDLE },
/* DRSELECT  */ { IRSELECT, DRCAPTURE },
/* DRCAPTURE */ { DREXIT1,  DRSHIFT },
/* DRSHIFT   */ { DREXIT1,  DRSHIFT },
/* DREXIT1   */ { DRUPDATE, DRPAUSE },
/* DRPAUSE   */ { DREXIT2,  DRPAUSE },
/* DREXIT2   */ { DRUPDATE, DRSHIFT },
/* DRUPDATE  */ { DRSELECT, IDLE },
/* IRSELECT  */ { RESET,    IRCAPTURE },
/* IRCAPTURE */ { IREXIT1,  IRSHIFT },
/* IRSHIFT   */ { IREXIT1,  IRSHIFT },
/* IREXIT1   */ { IRUPDATE, IRPAUSE },
/* IRPAUSE   */ { IREXIT2,  IRPAUSE },
/* IREXIT2   */ { IRUPDATE, IRSHIFT },
/* IRUPDATE  */ { DRSELECT, IDLE }
};

/*
*   This table contains the TMS value to be used to take the NEXT STEP on
*   the path to the desired state.  The array index is the current state,
*   and the bit position is the desired endstate.  To find out which state
*   is used as the intermediate state, look up the TMS value in the
*   jbi_jtag_state_transitions[] table.
*/
unsigned short jbi_jtag_path_map[16] =
{
/*    RST     RTI    SDRS     CDR     SDR    E1DR     PDR    E2DR       */
    0x0001, 0xFFFD, 0xFE01, 0xFFE7, 0xFFEF, 0xFF0F, 0xFFBF, 0xFFFF,
/*    UDR    SIRS     CIR     SIR    E1IR     PIR    E2IR     UIR       */
    0xFEFD, 0x0001, 0xF3FF, 0xF7FF, 0x87FF, 0xDFFF, 0xFFFF, 0x7FFD
};

/*
*   Flag bits for jbi_jtag_io() function
*/
#define TMS_HIGH   1
#define TMS_LOW    0
#define TDI_HIGH   1
#define TDI_LOW    0
#define READ_TDO   1
#define IGNORE_TDO 0

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_init_jtag()

/*                                                                          */
/****************************************************************************/
{
    /* initial JTAG state is unknown */
    jbi_jtag_state = JBI_ILLEGAL_JTAG_STATE;

    /* initialize global variables to default state */
    jbi_drstop_state = IDLE;
    jbi_irstop_state = IDLE;
    jbi_dr_preamble  = 0;
    jbi_dr_postamble = 0;
    jbi_ir_preamble  = 0;
    jbi_ir_postamble = 0;
    jbi_dr_length    = 0;
    jbi_ir_length    = 0;

    if (jbi_workspace != NULL)
    {
        jbi_dr_preamble_data = (unsigned char *) jbi_workspace;
        jbi_dr_postamble_data = &jbi_dr_preamble_data[JBIC_MAX_JTAG_DR_PREAMBLE / 8];
        jbi_ir_preamble_data = &jbi_dr_postamble_data[JBIC_MAX_JTAG_DR_POSTAMBLE / 8];
        jbi_ir_postamble_data = &jbi_ir_preamble_data[JBIC_MAX_JTAG_IR_PREAMBLE / 8];
        jbi_dr_buffer = &jbi_ir_postamble_data[JBIC_MAX_JTAG_IR_POSTAMBLE / 8];
        jbi_ir_buffer = &jbi_dr_buffer[JBIC_MAX_JTAG_DR_LENGTH / 8];
    }
    else
    {
        jbi_dr_preamble_data  = NULL;
        jbi_dr_postamble_data = NULL;
        jbi_ir_preamble_data  = NULL;
        jbi_ir_postamble_data = NULL;
        jbi_dr_buffer         = NULL;
        jbi_ir_buffer         = NULL;
    }

    return (JBIC_SUCCESS);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_drstop_state
(
    JBIE_JTAG_STATE state
)

/*                                                                          */
/****************************************************************************/
{
    jbi_drstop_state = state;

    return (JBIC_SUCCESS);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_irstop_state
(
    JBIE_JTAG_STATE state
)

/*                                                                          */
/****************************************************************************/
{
    jbi_irstop_state = state;

    return (JBIC_SUCCESS);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_dr_preamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *preamble_data
)

/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned int i;
    unsigned int j;

    if (jbi_workspace != NULL)
    {
        if (count > JBIC_MAX_JTAG_DR_PREAMBLE)
        {
            status = JBIC_OUT_OF_MEMORY;
        }
        else
        {
            jbi_dr_preamble = count;
        }
    }
    else
    {
        if (count > jbi_dr_preamble)
        {
            jbi_free(jbi_dr_preamble_data);
            jbi_dr_preamble_data = (unsigned char *) jbi_malloc((count + 7) >> 3);

            if (jbi_dr_preamble_data == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_dr_preamble = count;
            }
        }
        else
        {
            jbi_dr_preamble = count;
        }
    }

    if (status == JBIC_SUCCESS)
    {
        for (i = 0; i < count; ++i)
        {
            j = i + start_index;

            if (preamble_data == NULL)
            {
                jbi_dr_preamble_data[i >> 3] |= (1 << (i & 7));
            }
            else
            {
                if (preamble_data[j >> 3] & (1 << (j & 7)))
                {
                    jbi_dr_preamble_data[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    jbi_dr_preamble_data[i >> 3] &=
                        ~(unsigned int) (1 << (i & 7));
                }
            }
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_ir_preamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *preamble_data
)

/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned int i;
    unsigned int j;

    if (jbi_workspace != NULL)
    {
        if (count > JBIC_MAX_JTAG_IR_PREAMBLE)
        {
            status = JBIC_OUT_OF_MEMORY;
        }
        else
        {
            jbi_ir_preamble = count;
        }
    }
    else
    {
        if (count > jbi_ir_preamble)
        {
            jbi_free(jbi_ir_preamble_data);
            jbi_ir_preamble_data = (unsigned char *) jbi_malloc((count + 7) >> 3);

            if (jbi_ir_preamble_data == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_ir_preamble = count;
            }
        }
        else
        {
            jbi_ir_preamble = count;
        }
    }

    if (status == JBIC_SUCCESS)
    {
        for (i = 0; i < count; ++i)
        {
            j = i + start_index;

            if (preamble_data == NULL)
            {
                jbi_ir_preamble_data[i >> 3] |= (1 << (i & 7));
            }
            else
            {
                if (preamble_data[j >> 3] & (1 << (j & 7)))
                {
                    jbi_ir_preamble_data[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    jbi_ir_preamble_data[i >> 3] &=
                        ~(unsigned int) (1 << (i & 7));
                }
            }
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_dr_postamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *postamble_data
)

/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned int i;
    unsigned int j;

    if (jbi_workspace != NULL)
    {
        if (count > JBIC_MAX_JTAG_DR_POSTAMBLE)
        {
            status = JBIC_OUT_OF_MEMORY;
        }
        else
        {
            jbi_dr_postamble = count;
        }
    }
    else
    {
        if (count > jbi_dr_postamble)
        {
            jbi_free(jbi_dr_postamble_data);
            jbi_dr_postamble_data = (unsigned char *) jbi_malloc((count + 7) >> 3);

            if (jbi_dr_postamble_data == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_dr_postamble = count;
            }
        }
        else
        {
            jbi_dr_postamble = count;
        }
    }

    if (status == JBIC_SUCCESS)
    {
        for (i = 0; i < count; ++i)
        {
            j = i + start_index;

            if (postamble_data == NULL)
            {
                jbi_dr_postamble_data[i >> 3] |= (1 << (i & 7));
            }
            else
            {
                if (postamble_data[j >> 3] & (1 << (j & 7)))
                {
                    jbi_dr_postamble_data[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    jbi_dr_postamble_data[i >> 3] &=
                        ~(unsigned int) (1 << (i & 7));
                }
            }
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_set_ir_postamble
(
    unsigned int count,
    unsigned int start_index,
    unsigned char *postamble_data
)

/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned int i;
    unsigned int j;

    if (jbi_workspace != NULL)
    {
        if (count > JBIC_MAX_JTAG_IR_POSTAMBLE)
        {
            status = JBIC_OUT_OF_MEMORY;
        }
        else
        {
            jbi_ir_postamble = count;
        }
    }
    else
    {
        if (count > jbi_ir_postamble)
        {
            jbi_free(jbi_ir_postamble_data);
            jbi_ir_postamble_data = (unsigned char *) jbi_malloc((count + 7) >> 3);

            if (jbi_ir_postamble_data == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_ir_postamble = count;
            }
        }
        else
        {
            jbi_ir_postamble = count;
        }
    }

    if (status == JBIC_SUCCESS)
    {
        for (i = 0; i < count; ++i)
        {
            j = i + start_index;

            if (postamble_data == NULL)
            {
                jbi_ir_postamble_data[i >> 3] |= (1 << (i & 7));
            }
            else
            {
                if (postamble_data[j >> 3] & (1 << (j & 7)))
                {
                    jbi_ir_postamble_data[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    jbi_ir_postamble_data[i >> 3] &=
                        ~(unsigned int) (1 << (i & 7));
                }
            }
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

void jbi_jtag_reset_idle(void)

/*                                                                          */
/****************************************************************************/
{
    int i;

    /*
    *   Go to Test Logic Reset (no matter what the starting state may be)
    */
    for (i = 0; i < 5; ++i)
    {
        jbi_jtag_io(TMS_HIGH, TDI_LOW, IGNORE_TDO);
    }

    /*
    *   Now step to Run Test / Idle
    */
    jbi_jtag_io(TMS_LOW, TDI_LOW, IGNORE_TDO);

    jbi_jtag_state = IDLE;
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_goto_jtag_state
(
    JBIE_JTAG_STATE state
)

/*                                                                          */
/****************************************************************************/
{
    int tms;
    int count = 0;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned int tmp_state;

    if (jbi_jtag_state == JBI_ILLEGAL_JTAG_STATE)
    {
        /* initialize JTAG chain to known state */
        jbi_jtag_reset_idle();
    }

    if (jbi_jtag_state == state)
    {
        /*
        *   We are already in the desired state.  If it is a stable state,
        *   loop here.  Otherwise do nothing (no clock cycles).
        */
        if ((state == IDLE) ||
            (state == DRSHIFT) ||
            (state == DRPAUSE) ||
            (state == IRSHIFT) ||
            (state == IRPAUSE))
        {
            jbi_jtag_io(TMS_LOW, TDI_LOW, IGNORE_TDO);
        }
        else if (state == RESET)
        {
            jbi_jtag_io(TMS_HIGH, TDI_LOW, IGNORE_TDO);
        }
    }
    else
    {
        while ((jbi_jtag_state != state) && (count < 9))
        {
            /*
            *   Get TMS value to take a step toward desired state
            */
           if (state < 0) {
               tmp_state = 0;
           } else {
               tmp_state = state;
           }
            tms = (jbi_jtag_path_map[jbi_jtag_state] & (1 << tmp_state)) ?
                TMS_HIGH : TMS_LOW;

            /*
            *   Take a step
            */
            jbi_jtag_io(tms, TDI_LOW, IGNORE_TDO);

            if (tms)
            {
                jbi_jtag_state =
                    jbi_jtag_state_transitions[jbi_jtag_state].tms_high;
            }
            else
            {
                jbi_jtag_state =
                    jbi_jtag_state_transitions[jbi_jtag_state].tms_low;
            }

            ++count;
        }
    }

    if (jbi_jtag_state != state)
    {
        status = JBIC_INTERNAL_ERROR;
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_do_wait_cycles
(
    long cycles,
    JBIE_JTAG_STATE wait_state
)

/*                                                                          */
/*  Description:    Causes JTAG hardware to loop in the specified stable    */
/*                  state for the specified number of TCK clock cycles.     */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    int tms;
    long count;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;

    if (jbi_jtag_state != wait_state)
    {
        status = jbi_goto_jtag_state(wait_state);
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Set TMS high to loop in RESET state
        *   Set TMS low to loop in any other stable state
        */
        tms = (wait_state == RESET) ? TMS_HIGH : TMS_LOW;

        for (count = 0L; count < cycles; count++)
        {
            jbi_jtag_io(tms, TDI_LOW, IGNORE_TDO);
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_do_wait_microseconds
(
    long microseconds,
    JBIE_JTAG_STATE wait_state
)

/*                                                                          */
/*  Description:    Causes JTAG hardware to sit in the specified stable     */
/*                  state for the specified duration of real time.  If      */
/*                  no JTAG operations have been performed yet, then only   */
/*                  a delay is performed.  This permits the WAIT USECS      */
/*                  statement to be used in VECTOR programs without causing */
/*                  any JTAG operations.                                    */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;

    if ((jbi_jtag_state != JBI_ILLEGAL_JTAG_STATE) &&
        (jbi_jtag_state != wait_state))
    {
        status = jbi_goto_jtag_state(wait_state);
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Wait for specified time interval
        */
        jbi_delay(microseconds);
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

void jbi_jtag_concatenate_data
(
    unsigned char *buffer,
    unsigned char *preamble_data,
    unsigned int preamble_count,
    unsigned char *target_data,
    unsigned long start_index,
    unsigned int target_count,
    unsigned char *postamble_data,
    unsigned int postamble_count
)

/*                                                                          */
/*  Description:    Copies preamble data, target data, and postamble data   */
/*                  into one buffer for IR or DR scans.                     */
/*                                                                          */
/*  Returns:        nothing                                                 */
/*                                                                          */
/****************************************************************************/
{
    unsigned long i;
    unsigned long j;
    unsigned long k;

    for (i = 0L; i < preamble_count; ++i)
    {
        if (preamble_data[i >> 3L] & (1L << (i & 7L)))
        {
            buffer[i >> 3L] |= (1L << (i & 7L));
        }
        else
        {
            buffer[i >> 3L] &= ~(unsigned int) (1L << (i & 7L));
        }
    }

    j = start_index;
    k = preamble_count + target_count;
    for (; i < k; ++i, ++j)
    {
        if (target_data[j >> 3L] & (1L << (j & 7L)))
        {
            buffer[i >> 3L] |= (1L << (i & 7L));
        }
        else
        {
            buffer[i >> 3L] &= ~(unsigned int) (1L << (i & 7L));
        }
    }

    j = 0L;
    k = preamble_count + target_count + postamble_count;
    for (; i < k; ++i, ++j)
    {
        if (postamble_data[j >> 3L] & (1L << (j & 7L)))
        {
            buffer[i >> 3L] |= (1L << (i & 7L));
        }
        else
        {
            buffer[i >> 3L] &= ~(unsigned int) (1L << (i & 7L));
        }
    }
}

int jbi_jtag_drscan
(
    int start_state,
    int count,
    unsigned char *tdi,
    unsigned char *tdo
)
{
    int i = 0;
    int tdo_bit = 0;
    int status = 1;

    /*
    *   First go to DRSHIFT state
    */
    switch (start_state)
    {
    case 0:                     /* IDLE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(0, 0, 0);   /* DRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* DRSHIFT */
        break;

    case 1:                     /* DRPAUSE */
        jbi_jtag_io(1, 0, 0);   /* DREXIT2 */
        jbi_jtag_io(1, 0, 0);   /* DRUPDATE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(0, 0, 0);   /* DRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* DRSHIFT */
        break;

    case 2:                     /* IRPAUSE */
        jbi_jtag_io(1, 0, 0);   /* IREXIT2 */
        jbi_jtag_io(1, 0, 0);   /* IRUPDATE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(0, 0, 0);   /* DRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* DRSHIFT */
        break;

    default:
        status = 0;
    }

    if (status)
    {
        /* loop in the SHIFT-DR state */
        for (i = 0; i < count; i++)
        {
            tdo_bit = jbi_jtag_io(
                (i == count - 1),
                tdi[i >> 3] & (1 << (i & 7)),
                (tdo != NULL));

            if (tdo != NULL)
            {
                if (tdo_bit)
                {
                    tdo[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    tdo[i >> 3] &= ~(unsigned int) (1 << (i & 7));
                }
            }
        }

        jbi_jtag_io(0, 0, 0);   /* DRPAUSE */
    }

    return (status);
}

int jbi_jtag_irscan
(
    int start_state,
    int count,
    unsigned char *tdi,
    unsigned char *tdo
)
{
    int i = 0;
    int tdo_bit = 0;
    int status = 1;

    /*
    *   First go to IRSHIFT state
    */
    switch (start_state)
    {
    case 0:                     /* IDLE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(1, 0, 0);   /* IRSELECT */
        jbi_jtag_io(0, 0, 0);   /* IRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* IRSHIFT */
        break;

    case 1:                     /* DRPAUSE */
        jbi_jtag_io(1, 0, 0);   /* DREXIT2 */
        jbi_jtag_io(1, 0, 0);   /* DRUPDATE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(1, 0, 0);   /* IRSELECT */
        jbi_jtag_io(0, 0, 0);   /* IRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* IRSHIFT */
        break;

    case 2:                     /* IRPAUSE */
        jbi_jtag_io(1, 0, 0);   /* IREXIT2 */
        jbi_jtag_io(1, 0, 0);   /* IRUPDATE */
        jbi_jtag_io(1, 0, 0);   /* DRSELECT */
        jbi_jtag_io(1, 0, 0);   /* IRSELECT */
        jbi_jtag_io(0, 0, 0);   /* IRCAPTURE */
        jbi_jtag_io(0, 0, 0);   /* IRSHIFT */
        break;

    default:
        status = 0;
    }

    if (status)
    {
        /* loop in the SHIFT-IR state */
        for (i = 0; i < count; i++)
        {
            tdo_bit = jbi_jtag_io(
                (i == count - 1),
                tdi[i >> 3] & (1 << (i & 7)),
                (tdo != NULL));

            if (tdo != NULL)
            {
                if (tdo_bit)
                {
                    tdo[i >> 3] |= (1 << (i & 7));
                }
                else
                {
                    tdo[i >> 3] &= ~(unsigned int) (1 << (i & 7));
                }
            }
        }

        jbi_jtag_io(0, 0, 0);   /* IRPAUSE */
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

void jbi_jtag_extract_target_data
(
    unsigned char *buffer,
    unsigned char *target_data,
    unsigned int start_index,
    unsigned int preamble_count,
    unsigned int target_count
)

/*                                                                          */
/*  Description:    Copies target data from scan buffer, filtering out      */
/*                  preamble and postamble data.                            */
/*                                                                          */
/*  Returns:        nothing                                                 */
/*                                                                          */
/****************************************************************************/
{
    unsigned int i;
    unsigned int j;
    unsigned int k;

    j = preamble_count;
    k = start_index + target_count;
    for (i = start_index; i < k; ++i, ++j)
    {
        if (buffer[j >> 3] & (1 << (j & 7)))
        {
            target_data[i >> 3] |= (1 << (i & 7));
        }
        else
        {
            target_data[i >> 3] &= ~(unsigned int) (1 << (i & 7));
        }
    }
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_do_irscan
(
    unsigned int count,
    unsigned char *tdi_data,
    unsigned int start_index
)

/*                                                                          */
/*  Description:    Shifts data into instruction register                   */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    int start_code = 0;
    unsigned int alloc_chars = 0;
    unsigned int shift_count = jbi_ir_preamble + count + jbi_ir_postamble;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    JBIE_JTAG_STATE start_state = JBI_ILLEGAL_JTAG_STATE;

    switch (jbi_jtag_state)
    {
    case JBI_ILLEGAL_JTAG_STATE:
    case RESET:
    case IDLE:
        start_code = 0;
        start_state = IDLE;
        break;

    case DRSELECT:
    case DRCAPTURE:
    case DRSHIFT:
    case DREXIT1:
    case DRPAUSE:
    case DREXIT2:
    case DRUPDATE:
        start_code = 1;
        start_state = DRPAUSE;
        break;

    case IRSELECT:
    case IRCAPTURE:
    case IRSHIFT:
    case IREXIT1:
    case IRPAUSE:
    case IREXIT2:
    case IRUPDATE:
        start_code = 2;
        start_state = IRPAUSE;
        break;

    default:
        status = JBIC_INTERNAL_ERROR;
        break;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_jtag_state != start_state)
        {
            status = jbi_goto_jtag_state(start_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_workspace != NULL)
        {
            if (shift_count > JBIC_MAX_JTAG_IR_LENGTH)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
        }
        else if (shift_count > jbi_ir_length)
        {
            alloc_chars = (shift_count + 7) >> 3;
            jbi_free(jbi_ir_buffer);
            jbi_ir_buffer = (unsigned char *) jbi_malloc(alloc_chars);

            if (jbi_ir_buffer == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_ir_length = alloc_chars * 8;
            }
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Copy preamble data, IR data, and postamble data into a buffer
        */
        jbi_jtag_concatenate_data
        (
            jbi_ir_buffer,
            jbi_ir_preamble_data,
            jbi_ir_preamble,
            tdi_data,
            start_index,
            count,
            jbi_ir_postamble_data,
            jbi_ir_postamble
        );

        /*
        *   Do the IRSCAN
        */
        jbi_jtag_irscan
        (
            start_code,
            shift_count,
            jbi_ir_buffer,
            NULL
        );

        /* jbi_jtag_irscan() always ends in IRPAUSE state */
        jbi_jtag_state = IRPAUSE;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_irstop_state != IRPAUSE)
        {
            status = jbi_goto_jtag_state(jbi_irstop_state);
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_swap_ir
(
    unsigned int count,
    unsigned char *in_data,
    unsigned int in_index,
    unsigned char *out_data,
    unsigned int out_index
)

/*                                                                          */
/*  Description:    Shifts data into instruction register, capturing output */
/*                  data                                                    */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    int start_code = 0;
    unsigned int alloc_chars = 0;
    unsigned int shift_count = jbi_ir_preamble + count + jbi_ir_postamble;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    JBIE_JTAG_STATE start_state = JBI_ILLEGAL_JTAG_STATE;

    switch (jbi_jtag_state)
    {
    case JBI_ILLEGAL_JTAG_STATE:
    case RESET:
    case IDLE:
        start_code = 0;
        start_state = IDLE;
        break;

    case DRSELECT:
    case DRCAPTURE:
    case DRSHIFT:
    case DREXIT1:
    case DRPAUSE:
    case DREXIT2:
    case DRUPDATE:
        start_code = 1;
        start_state = DRPAUSE;
        break;

    case IRSELECT:
    case IRCAPTURE:
    case IRSHIFT:
    case IREXIT1:
    case IRPAUSE:
    case IREXIT2:
    case IRUPDATE:
        start_code = 2;
        start_state = IRPAUSE;
        break;

    default:
        status = JBIC_INTERNAL_ERROR;
        break;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_jtag_state != start_state)
        {
            status = jbi_goto_jtag_state(start_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_workspace != NULL)
        {
            if (shift_count > JBIC_MAX_JTAG_IR_LENGTH)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
        }
        else if (shift_count > jbi_ir_length)
        {
            alloc_chars = (shift_count + 7) >> 3;
            jbi_free(jbi_ir_buffer);
            jbi_ir_buffer = (unsigned char *) jbi_malloc(alloc_chars);

            if (jbi_ir_buffer == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_ir_length = alloc_chars * 8;
            }
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Copy preamble data, IR data, and postamble data into a buffer
        */
        jbi_jtag_concatenate_data
        (
            jbi_ir_buffer,
            jbi_ir_preamble_data,
            jbi_ir_preamble,
            in_data,
            in_index,
            count,
            jbi_ir_postamble_data,
            jbi_ir_postamble
        );

        /*
        *   Do the IRSCAN
        */
        jbi_jtag_irscan
        (
            start_code,
            shift_count,
            jbi_ir_buffer,
            jbi_ir_buffer
        );

        /* jbi_jtag_irscan() always ends in IRPAUSE state */
        jbi_jtag_state = IRPAUSE;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_irstop_state != IRPAUSE)
        {
            status = jbi_goto_jtag_state(jbi_irstop_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Now extract the returned data from the buffer
        */
        jbi_jtag_extract_target_data
        (
            jbi_ir_buffer,
            out_data,
            out_index,
            jbi_ir_preamble,
            count
        );
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_do_drscan
(
    unsigned int count,
    unsigned char *tdi_data,
    unsigned long start_index
)

/*                                                                          */
/*  Description:    Shifts data into data register (ignoring output data)   */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    int start_code = 0;
    unsigned int alloc_chars = 0;
    unsigned int shift_count = jbi_dr_preamble + count + jbi_dr_postamble;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    JBIE_JTAG_STATE start_state = JBI_ILLEGAL_JTAG_STATE;

    switch (jbi_jtag_state)
    {
    case JBI_ILLEGAL_JTAG_STATE:
    case RESET:
    case IDLE:
        start_code = 0;
        start_state = IDLE;
        break;

    case DRSELECT:
    case DRCAPTURE:
    case DRSHIFT:
    case DREXIT1:
    case DRPAUSE:
    case DREXIT2:
    case DRUPDATE:
        start_code = 1;
        start_state = DRPAUSE;
        break;

    case IRSELECT:
    case IRCAPTURE:
    case IRSHIFT:
    case IREXIT1:
    case IRPAUSE:
    case IREXIT2:
    case IRUPDATE:
        start_code = 2;
        start_state = IRPAUSE;
        break;

    default:
        status = JBIC_INTERNAL_ERROR;
        break;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_jtag_state != start_state)
        {
            status = jbi_goto_jtag_state(start_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_workspace != NULL)
        {
            if (shift_count > JBIC_MAX_JTAG_DR_LENGTH)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
        }
        else if (shift_count > jbi_dr_length)
        {
            alloc_chars = (shift_count + 7) >> 3;
            jbi_free(jbi_dr_buffer);
            jbi_dr_buffer = (unsigned char *) jbi_malloc(alloc_chars);

            if (jbi_dr_buffer == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_dr_length = alloc_chars * 8;
            }
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Copy preamble data, DR data, and postamble data into a buffer
        */
        jbi_jtag_concatenate_data
        (
            jbi_dr_buffer,
            jbi_dr_preamble_data,
            jbi_dr_preamble,
            tdi_data,
            start_index,
            count,
            jbi_dr_postamble_data,
            jbi_dr_postamble
        );

        /*
        *   Do the DRSCAN
        */
        jbi_jtag_drscan
        (
            start_code,
            shift_count,
            jbi_dr_buffer,
            NULL
        );

        /* jbi_jtag_drscan() always ends in DRPAUSE state */
        jbi_jtag_state = DRPAUSE;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_drstop_state != DRPAUSE)
        {
            status = jbi_goto_jtag_state(jbi_drstop_state);
        }
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_swap_dr
(
    unsigned int count,
    unsigned char *in_data,
    unsigned long in_index,
    unsigned char *out_data,
    unsigned int out_index
)

/*                                                                          */
/*  Description:    Shifts data into data register, capturing output data   */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    int start_code = 0;
    unsigned int alloc_chars = 0;
    unsigned int shift_count = jbi_dr_preamble + count + jbi_dr_postamble;
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    JBIE_JTAG_STATE start_state = JBI_ILLEGAL_JTAG_STATE;

    switch (jbi_jtag_state)
    {
    case JBI_ILLEGAL_JTAG_STATE:
    case RESET:
    case IDLE:
        start_code = 0;
        start_state = IDLE;
        break;

    case DRSELECT:
    case DRCAPTURE:
    case DRSHIFT:
    case DREXIT1:
    case DRPAUSE:
    case DREXIT2:
    case DRUPDATE:
        start_code = 1;
        start_state = DRPAUSE;
        break;

    case IRSELECT:
    case IRCAPTURE:
    case IRSHIFT:
    case IREXIT1:
    case IRPAUSE:
    case IREXIT2:
    case IRUPDATE:
        start_code = 2;
        start_state = IRPAUSE;
        break;

    default:
        status = JBIC_INTERNAL_ERROR;
        break;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_jtag_state != start_state)
        {
            status = jbi_goto_jtag_state(start_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_workspace != NULL)
        {
            if (shift_count > JBIC_MAX_JTAG_DR_LENGTH)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
        }
        else if (shift_count > jbi_dr_length)
        {
            alloc_chars = (shift_count + 7) >> 3;
            jbi_free(jbi_dr_buffer);
            jbi_dr_buffer = (unsigned char *) jbi_malloc(alloc_chars);

            if (jbi_dr_buffer == NULL)
            {
                status = JBIC_OUT_OF_MEMORY;
            }
            else
            {
                jbi_dr_length = alloc_chars * 8;
            }
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Copy preamble data, DR data, and postamble data into a buffer
        */
        jbi_jtag_concatenate_data
        (
            jbi_dr_buffer,
            jbi_dr_preamble_data,
            jbi_dr_preamble,
            in_data,
            in_index,
            count,
            jbi_dr_postamble_data,
            jbi_dr_postamble
        );

        /*
        *   Do the DRSCAN
        */
        jbi_jtag_drscan
        (
            start_code,
            shift_count,
            jbi_dr_buffer,
            jbi_dr_buffer
        );

        /* jbi_jtag_drscan() always ends in DRPAUSE state */
        jbi_jtag_state = DRPAUSE;
    }

    if (status == JBIC_SUCCESS)
    {
        if (jbi_drstop_state != DRPAUSE)
        {
            status = jbi_goto_jtag_state(jbi_drstop_state);
        }
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Now extract the returned data from the buffer
        */
        jbi_jtag_extract_target_data
        (
            jbi_dr_buffer,
            out_data,
            out_index,
            jbi_dr_preamble,
            count
        );
    }

    return (status);
}

/****************************************************************************/
/*                                                                          */

void jbi_free_jtag_padding_buffers(int reset_jtag)

/*                                                                          */
/*  Description:    Frees memory allocated for JTAG IR and DR buffers       */
/*                                                                          */
/*  Returns:        nothing                                                 */
/*                                                                          */
/****************************************************************************/
{
    /*
    *   If the JTAG interface was used, reset it to TLR
    */
    if (reset_jtag && (jbi_jtag_state != JBI_ILLEGAL_JTAG_STATE))
    {
        jbi_jtag_reset_idle();
    }

    if (jbi_workspace == NULL)
    {
        if (jbi_dr_preamble_data != NULL)
        {
            jbi_free(jbi_dr_preamble_data);
            jbi_dr_preamble_data = NULL;
        }

        if (jbi_dr_postamble_data != NULL)
        {
            jbi_free(jbi_dr_postamble_data);
            jbi_dr_postamble_data = NULL;
        }

        if (jbi_dr_buffer != NULL)
        {
            jbi_free(jbi_dr_buffer);
            jbi_dr_buffer = NULL;
        }

        if (jbi_ir_preamble_data != NULL)
        {
            jbi_free(jbi_ir_preamble_data);
            jbi_ir_preamble_data = NULL;
        }

        if (jbi_ir_postamble_data != NULL)
        {
            jbi_free(jbi_ir_postamble_data);
            jbi_ir_postamble_data = NULL;
        }

        if (jbi_ir_buffer != NULL)
        {
            jbi_free(jbi_ir_buffer);
            jbi_ir_buffer = NULL;
        }
    }
}

#if PORT==DOS

/****************************************************************************/
/*                                                                          */

JBI_RETURN_TYPE jbi_do_drscan_multi_page
(
    unsigned int variable_id,
    unsigned long count,
    unsigned long start_index,
    int version
)

/*                                                                          */
/*  Description:    Shifts data into data register (ignoring output data)   */
/*                  Scan data comes from compressed Boolean array.          */
/*                                                                          */
/*  Returns:        JBIC_SUCCESS for success, else appropriate error code   */
/*                                                                          */
/****************************************************************************/
{
    JBI_RETURN_TYPE status = JBIC_SUCCESS;
    unsigned long shift_count = jbi_dr_preamble + count + jbi_dr_postamble;
    unsigned long i;
    unsigned long j;
    unsigned long k;
    unsigned int bi;

    if (status == JBIC_SUCCESS)
    {
        status = jbi_goto_jtag_state(DRSHIFT);
    }

    if (status == JBIC_SUCCESS)
    {
        /*
        *   Get preamble data, DR data, and postamble data one bit at a time
        *   and immediately scan it into the JTAG chain
        */

        for (i = 0L; i < jbi_dr_preamble; ++i)
        {
            jbi_jtag_io((i == shift_count - 1),
                (int) (jbi_dr_preamble_data[i >> 3L] & (1L << (i & 7L))), 0);
        }

        j = start_index;
        k = jbi_dr_preamble + count;

        jbi_uncompress_page(variable_id, (unsigned int) (j >> 16L), version);

        for (; i < k; ++i, ++j)
        {
            bi = (unsigned int) (j & 0x0000ffffL);

            /* check for page boundary - load next page if necessary */
            if (bi == 0)
            {
                jbi_uncompress_page(variable_id, (unsigned int) (j >> 16L), version);
            }

            jbi_jtag_io((i == shift_count - 1),
                (int) (jbi_aca_out_buffer[bi >> 3] & (1 << (bi & 7))), 0);
        }

        j = 0L;
        k = jbi_dr_preamble + count + jbi_dr_postamble;
        for (; i < k; ++i, ++j)
        {
            jbi_jtag_io((i == shift_count - 1),
                (int) (jbi_dr_postamble_data[j >> 3L] & (1L << (j & 7L))), 0);
        }

        jbi_jtag_io(0, 0, 0);   /* DRPAUSE */

        /* jbi_jtag_drscan() always ends in DRPAUSE state */
        jbi_jtag_state = DRPAUSE;

        if (jbi_drstop_state != DRPAUSE)
        {
            status = jbi_goto_jtag_state(jbi_drstop_state);
        }
    }

    return (status);
}

#endif
