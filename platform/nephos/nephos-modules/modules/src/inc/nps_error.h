/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:   nps_error.h
 * PURPOSE:
 *      Define the generic error code on NPS SDK.
 * NOTES:
 */

#ifndef NPS_ERROR_H
#define NPS_ERROR_H

/* INCLUDE FILE DECLARATIONS
 */
#include <nps_types.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

typedef enum
{
    NPS_E_OK = 0,           /* Ok and no error */
    NPS_E_BAD_PARAMETER,    /* Parameter is wrong */
    NPS_E_NO_MEMORY,        /* No memory is available */
    NPS_E_TABLE_FULL,       /* Table is full */
    NPS_E_ENTRY_NOT_FOUND,  /* Entry is not found */
    NPS_E_ENTRY_EXISTS,     /* Entry already exists */
    NPS_E_NOT_SUPPORT,      /* Feature is not supported */
    NPS_E_ALREADY_INITED,   /* Module is reinitialized */
    NPS_E_NOT_INITED,       /* Module is not initialized */
    NPS_E_OTHERS,           /* Other errors */
    NPS_E_ENTRY_IN_USE,     /* Entry is in use */
    NPS_E_LAST
} NPS_ERROR_NO_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME:   nps_error_getString
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
nps_error_getString(
    const NPS_ERROR_NO_T cause );

#endif  /* NPS_ERROR_H */

