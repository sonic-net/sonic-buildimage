/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:   clx_error.h
 * PURPOSE:
 *      Define the generic error code on CLX SDK.
 * NOTES:
 */

#ifndef CLX_ERROR_H
#define CLX_ERROR_H

/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

typedef enum
{
    CLX_E_OK = 0,           /* Ok and no error */
    CLX_E_BAD_PARAMETER,    /* Parameter is wrong */
    CLX_E_NO_MEMORY,        /* No memory is available */
    CLX_E_TABLE_FULL,       /* Table is full */
    CLX_E_ENTRY_NOT_FOUND,  /* Entry is not found */
    CLX_E_ENTRY_EXISTS,     /* Entry already exists */
    CLX_E_NOT_SUPPORT,      /* Feature is not supported */
    CLX_E_ALREADY_INITED,   /* Module is reinitialized */
    CLX_E_NOT_INITED,       /* Module is not initialized */
    CLX_E_OTHERS,           /* Other errors */
    CLX_E_ENTRY_IN_USE,     /* Entry is in use */
    CLX_E_TIMEOUT,          /* Time out error */
    CLX_E_OP_INVALID,       /* Operation is invalid */
    CLX_E_OP_STOPPED,       /* Operation is stopped by user callback */
    CLX_E_OP_INCOMPLETE,    /* Operation is incomplete */
    CLX_E_LAST
} CLX_ERROR_NO_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME:   clx_error_getString
 * PURPOSE:
 *      To obtain the error string of the specified error code
 *
 * INPUT:
 *      cause  -- The specified error code
 * OUTPUT:
 *      None
 * RETURN:
 *      Pointer to the target error string
 *
 * NOTES:
 *
 *
 */
C8_T *
clx_error_getString(
    const CLX_ERROR_NO_T cause );

#endif  /* CLX_ERROR_H */

