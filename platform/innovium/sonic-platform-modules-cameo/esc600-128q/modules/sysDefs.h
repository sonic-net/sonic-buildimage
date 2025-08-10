/*
 *  COPYRIGHT (c) 2008 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

 
/** @file
 * Custom definitions to support a particular OS/platform.
 * Common data type definitions for usability on a particular platform.
 * Customized per platform/project depending on the OS and system environment
 * that the code will be built with and run on.
 * If the platform/build environment doesn't support certain POSIX standard
 * types, defines, etc. specify them here.  This file is customized per build
 * platform (ie. one version for building with Linux, another for VxWorks, etc.)
 * 
 */
 

#ifndef LATTICE_SEMI_SYSDEFS_H
#define LATTICE_SEMI_SYSDEFS_H


/*
 * Include specific header files for the OS that the code will run on.
 */
#include <linux/types.h>


typedef int status_t;


// Check if Linux 2.6 header files already define these types
#ifndef  __BIT_TYPES_DEFINED__
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#endif


/* These are Windows types (used in some shared files) so define them
 * when not on a Windows platform.
 */
typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;

#ifndef __cplusplus 
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
//typedef int bool;
#endif
#endif

#ifndef OK
#define OK 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif



/*============ Unsupported Functions in Linux =============*/

// Windows Sleep() is in milliseconds
// Linux has a usleep() which is microseconds
#define Sleep(a) usleep(a * 1000)



// Port some Windows concepts to Linux
typedef int HANDLE;
#define GUID char 


#endif
