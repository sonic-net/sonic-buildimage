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

/* FILE NAME:  cmlib.h
 * PURPOSE:
 *  this file is used to provide init common library operations to other users.
 * NOTES:
 *  it contains operations as below:
 *      1. init common library
 *      2. deinit common library.
 *      3. register debug commands for all sub-modules.
 *      4. deregister debug commands for all sub-modules.
 *      5. provide debug functions for sub-modules.
 *
 *
 *
 */
#ifndef CMLIB_H
#define CMLIB_H

/* INCLUDE FILE DECLARATIONS
 */

#include <clx_types.h>
#include <clx_error.h>

/* NAMING CONSTANT DECLARATIONS
 */
#define CMLIB_NAME_MAX_LEN        (32)

/* MACRO FUNCTION DECLARATIONS
 */


/* FUNCTION NAME: CMLIB_MULTI_OVERFLOW
 * PURPOSE:
 *      it is used to test if two 32-bit unsigned operand multiply will be overflow
 * INPUT:
 *      ui32a  -- operand a
 *      ui32b  -- operand b
 * OUTPUT:
 *      None.
 * RETURN:
 *      0     -- not overflow
 *      non 0 -- overflow
 * NOTES:
 *
 */
#define CMLIB_MULTI_OVERFLOW(ui32a, ui32b) \
    ((ui32a) ? ((~0U/(UI32_T)(ui32a)) < (UI32_T)(ui32b)) : 0)

/* FUNCTION NAME: CMLIB_ADD_OVERFLOW
 * PURPOSE:
 *      it is used to test if two operands add overflow.
 * INPUT:
 *      ui32a  -- operand a
 *      ui32b  -- operand b
 * OUTPUT:
 *      None.
 * RETURN:
 *      0     -- not overflow
 *      non 0 -- overflow
 * NOTES:
 *
 */
#define CMLIB_ADD_OVERFLOW(ui32a, ui32b) \
    ((UI32_T)(ui32a) > ~((UI32_T)(ui32b)))

/* DATA TYPE DECLARATIONS
 */

typedef struct CMLIB_DBG_LIST_NODE_S
{
    void                         *ptr_data;
    struct CMLIB_DBG_LIST_NODE_S *ptr_next;
} CMLIB_DBG_LIST_NODE_T;


typedef struct
{
    CMLIB_DBG_LIST_NODE_T *ptr_front;
    CLX_SEMAPHORE_ID_T    sem;
} CMLIB_DBG_HEAD_T;


/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

#endif /* End of CMLIB_H */

