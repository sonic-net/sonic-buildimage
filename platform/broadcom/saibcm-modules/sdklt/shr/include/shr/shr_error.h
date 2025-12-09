/*! \file shr_error.h
 *
 * Shared error codes.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef SHR_ERROR_H
#define SHR_ERROR_H

/*!
 * \brief Standard SDK error codes.
 *
 * IMPORTANT: These error codes must match the corresponding text
 * messages in shr_error.c.
 */
typedef enum {

    /*!
     * The operation completed successfully.
     */
    SHR_E_NONE                 =  0,

    /*!
     * This usually indicates that software encountered an internal
     * data inconsistency or an unanticipated hardware state.
     */
    SHR_E_INTERNAL             = -1,

    /*!
     * An operation failed due to insufficient dynamically allocated
     * memory.
     */
    SHR_E_MEMORY               = -2,

    /*!
     * The first argument of many API routines is a unit number. This
     * error occurs if that number refers to a non-existent unit.
     */
    SHR_E_UNIT                 = -3,

    /*!
     * A parameter to an API routine was invalid. A null pointer value
     * may have been passed to the routine, or an integer parameter
     * may be outside of its allowed range.
     */
    SHR_E_PARAM                = -4,

    /*!
     * The operation encountered a pooled resource (e.g. a table or a
     * list) with no valid elements.
     */
    SHR_E_EMPTY                = -5,

    /*!
     * The operation encountered a pooled resource (e.g. a table or a
     * list) with no room for new elements.
     */
    SHR_E_FULL                 = -6,

    /*!
     * The specified entry in a pooled resource (e.g. a table or a
     * list) could not be found.
     */
    SHR_E_NOT_FOUND            = -7,

    /*!
     * The specified entry of a pooled resource (e.g. a table or a
     * list) already exists.
     */
    SHR_E_EXISTS               = -8,

    /*!
     * The operation did not complete within the maximum allowed time frame.
     */
    SHR_E_TIMEOUT              = -9,

    /*!
     * An operation was attempted before the previous operation had
     * completed.
     */
    SHR_E_BUSY                 = -10,

    /*!
     * An operation could not be completed. This may be due to a
     * hardware or configuration problem.
     */
    SHR_E_FAIL                 = -11,

    /*!
     * The operation could not be completed because a required feature
     * was disabled.
     */
    SHR_E_DISABLED             = -12,

    /*!
     * The specified identifier was not valid. Note that this error
     * code will normally take precedence over \ref SHR_E_PARAM.
     */
    SHR_E_BADID                = -13,

    /*!
     * The operation could not be completed due to lack of hardware
     * resources.
     */
    SHR_E_RESOURCE             = -14,

    /*!
     * The operation could not be completed due to incomplete or
     * incorrect configuration.
     */
    SHR_E_CONFIG               = -15,

    /*!
     * The hardware does not support the requested operation.
     */
    SHR_E_UNAVAIL              = -16,

    /*!
     * An operation was attempted before initialization was complete.
     */
    SHR_E_INIT                 = -17,

    /*!
     * The specified port value was not valid. Note that this error
     * code will normally take precedence over \ref SHR_E_PARAM.
     */
    SHR_E_PORT                 = -18,

    /*!
     * A low-level register or memory access failed.
     */
    SHR_E_IO                   = -19,

    /*!
     * Access method not permitted. Typically returned if attempting
     * to write to a read-only object.
     */
    SHR_E_ACCESS               = -20,

    /*!
     * No handler exists to perform the hardware access associated
     * with a an operation on a software object.
     */
    SHR_E_NO_HANDLER           = -21,

    /*!
     * The operation was only partially completed, and this could
     * potentially leave the system in an unexpected state.
     */
    SHR_E_PARTIAL              = -22,

    /*!
     * The operation failed because of a hash collision.
     */
    SHR_E_COLL                 = -23,

    SHR_E_LIMIT                = -24           /* Must come last */

} shr_error_t;

/*! Check for successful return value. */
#define SHR_SUCCESS(_expr) ((_expr) >= 0)

/*! Check for error return value. */
#define SHR_FAILURE(_expr) ((_expr) < 0)

/*!
 * \brief Get standard error message
 *
 * Returns a text message corresponding to the error code passed in.
 *
 * \param [in] rv Error code
 *
 * \return Pointer to error message
 */
extern const char *
shr_errmsg(int rv);

#endif /* SHR_ERROR_H */
