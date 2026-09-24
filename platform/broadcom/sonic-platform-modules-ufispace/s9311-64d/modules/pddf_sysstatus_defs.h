/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 * Description
 *  Platform system status module structures
 */

#ifndef __PDDF_SYSSTATUS_DEFS_H__
#define __PDDF_SYSSTATUS_DEFS_H__


#define ATTR_NAME_LEN 33
#define MAX_ATTRS 34


/* SYSSTATUS CLIENT DATA - PLATFORM DATA FOR SYSSTATUS CLIENT */
typedef struct SYSSTATUS_ADDR_ATTR
{
    char aname[ATTR_NAME_LEN];  // attr name
    uint32_t devaddr;
    uint32_t offset;
    uint32_t mask;
    uint32_t len;

}SYSSTATUS_ADDR_ATTR;


typedef struct SYSSTATUS_DATA
{
  int len;
  SYSSTATUS_ADDR_ATTR sysstatus_addr_attr;
  SYSSTATUS_ADDR_ATTR sysstatus_addr_attrs[MAX_ATTRS]; 
  
}SYSSTATUS_DATA;




#endif
