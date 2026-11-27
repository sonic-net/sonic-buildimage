/*
 * Copyright 2022 Accton Technology Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Description:
 *  PSU driver related api declarations
 */

#ifndef ACCTON_PSU_API_H
#define ACCTON_PSU_API_H

#include "accton_psu_defs.h"

/** Description:
 *     Register psu status entry, set entry as NULL to unregister
 */
extern int register_psu_status_entry(PSU_STATUS_ENTRY *entry);

#endif /* ACCTON_PSU_API_H */
