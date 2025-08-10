/****************************************************************************/
/*                                                                          */
/*  Module:         jbicomp.h                                               */
/*                                                                          */
/*                  Copyright (C) Altera Corporation 1997-2001              */
/*                                                                          */
/*  Description:    Contains the function prototypes for compressing        */
/*                  and uncompressing Boolean array data.                   */
/*                                                                          */
/****************************************************************************/

#ifndef INC_JBICOMP_H
#define INC_JBICOMP_H

#if PORT==DOS

void jbi_uncompress_page
(
    int variable_id,
    int page,
    int version
);

#else

unsigned long jbi_uncompress
(
    unsigned char *in,
    unsigned long in_length,
    unsigned char *out,
    unsigned long out_length,
    int version
);

#endif /* PORT==DOS */

#endif /* INC_JBICOMP_H */
