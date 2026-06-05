/** @file api_driver164_init.h
 *
 * @brief Security-IP-164 MACsec Driver Initialization API.
 *
 * This API specifies the driver entry and exit points.
 *
 * Note: one driver instance can support multiple Security-IP-164 MACsec
 *       hardware acceleration devices.
 */

/*****************************************************************************
* Copyright (c) 2015-2021 by Rambus, Inc. and/or its subsidiaries.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#ifndef DRIVER164_INIT_H_
#define DRIVER164_INIT_H_

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/** Driver initialization callbacks */
typedef int (*Driver164_Device_Read)(void* PlatformContext_p, const volatile void *addr, unsigned int* data);
typedef int (*Driver164_Device_Write)(void* PlatformContext_p, volatile void *addr, unsigned int data);
typedef int (*Driver164_Device_Logger)(int loglevel, const char* message);

/** Driver initialization structure */
typedef struct
{
   Driver164_Device_Read Device_Read;
   Driver164_Device_Write Device_Write;
   Driver164_Device_Logger Device_Logger;
} Driver164_Init_t;

/** Device administration structure */
typedef struct
{
    /** Device name */
    char * DeviceName_p;

    /** User specified platform context */
    void* PlatformContext_p;

    /** Device start offset in the device register range in system memory map */
    unsigned int StartByteOffset;

    /** Device end offset in the device register range in system memory map */
    unsigned int LastByteOffset;

    /** Implementation specific device flags */
    char Flags;

} Driver164_Device_t;


/** Data path administration structure */
typedef struct
{
    /** User specified platform context */
    void* PlatformContext_p;

    /**  Device start offset  inside system memory map of first sub-device
         chain.*/
    unsigned int StartByteOffset1;
    /**  Device last offset  inside system memory map of second sub-device
         chain.*/
    unsigned int LastByteOffset1;

    /**  Device start offset  inside system memory map of first sub-device
         chain.*/
    unsigned int StartByteOffset2;
    /**  Device last offset  inside system memory map of second sub-device
         chain.*/
    unsigned int LastByteOffset2;

    /** Implementation specific device flags for first chain */
    char Flags1;

    /** Implementation specific device flags for second chain */
    char Flags2;

} Driver164_DataPath_t;


/**---------------------------------------------------------------------------
 * @fn Driver164_Init(void)
 *
 * Initialize the driver. This function must be called before any other
 * driver API function can be called.
 *
 * @param [in] Init_p
 *     Pointer to memory location where driver initialization data is stored, may not be NULL
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_Init(
        const Driver164_Init_t * const Init_p);


/**---------------------------------------------------------------------------
 * @fn Driver164_Exit(void)
 *
 * Initialize the driver. After this function is called no other driver API
 * function can be called except Driver164_Init().
 */
void
Driver164_Exit(void);


/**---------------------------------------------------------------------------
 * @fn Driver164_Device_Add(
 *     const unsigned int Index,
 *     const Driver164_Device_t * const Device_p)
 *
 * Adds a new device to the driver device list.
 * It can be used as an alternative or in combination with static device list
 * configuration in the driver.
 *
 * @pre This function must be called before any other driver function can
 *      reference this device.
 *
 * @param [in] Index
 *     Device index where the device must be added in the device list
 *
 * @param [in] Device_p
 *     Pointer to memory location where device data is stored, may not be NULL
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_Device_Add(
        const unsigned int Index,
        const Driver164_Device_t * const Device_p);


/**---------------------------------------------------------------------------
 * @fn Driver164_Device_Remove(
 *     const unsigned int Index)
 *
 * Removes device from the driver device list at the requested index,
 * the device must be previously added either statically or via a call
 * to the Driver164_Device_Add() function.
 *
 * @pre This function must be called when no other driver function can reference
 *      this device.
 *
 * @param [in] Index
 *     Device index where the device must be added in the device list
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_Device_Remove(
        const unsigned int Index);


/**----------------------------------------------------------------------------
 * @fn Driver164_Device_GetCount(void)
 *
 * This function returns the number of devices present in the device list.
 *
 * @return : device count.
 */
unsigned int
Driver164_Device_GetCount(void);


/**----------------------------------------------------------------------------
 * @fn Driver164_Device_Get(
 *        const unsigned int Index,
 *        Driver164_Device_t * const Device_p)
 *
 * This function returns the data path properties for a given data path index .
 *
 * @param [in] Index
 *     Device index where the device must be added in the device list
 *
 * @param [in] Device_p
 *     Pointer to memory location where device data will be stored, may not be NULL
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_Device_GetProperties(
        const unsigned int Index,
        Driver164_Device_t * const Device_p);


/**---------------------------------------------------------------------------
 * @fn Driver164_DataPath_Add(
 *       const unsigned int Index,
 *       const Driver164_DataPath_t * const DataPath_p);
 *
 * Adds all devices for a complete data path to the driver device list.
 *
 * This function must be called before any other driver function can reference
 * this data path. It can be used as an alternative or in combination with
 * static device list configuration in the driver.
 *
 * @param [in] Index
 *     Data path index of the data path that must be added in the device list
 *
 * @param [in] DataPath_p
 *     Pointer to memory location where data path properties are stored, may
 *     not be NULL
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_DataPath_Add(
        const unsigned int Index,
        const Driver164_DataPath_t * const DataPath_p);


/**---------------------------------------------------------------------------
 * @fn Driver164_DataPath_Remove(
 *        const unsigned int Index);
 *
 * Removes all devices of a data path from the driver device list at
 * the requested index, the data path must be previously added either
 * statically or via a call to the Driver164_DataPath_Add() function.
 *
 * This function must be called when no other driver function can reference
 * this device.
 *
 * @param [in] Index
 *     Data path index of the data path that must be removed from the device
 *     list
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_DataPath_Remove(
        const unsigned int Index);


/**----------------------------------------------------------------------------
 * @fn Driver164_DataPath_GetCount(void)
 *
 * This function returns the number of data paths supported by the Driver.
 *
 * @return: Data path count.
 */
unsigned int
Driver164_DataPath_GetCount(void);


/**----------------------------------------------------------------------------
 * @fn Driver164_DataPath_GetProperties(
 *        const unsigned int Index,
 *        Driver164_DataPath_t * const DataPath_p)
 *
 * This function returns the data path properties for a given data path index .
 *
 * @param [in] Index
 *     Data path index of the data path that must be retrieved from the device
 *     list
 *
 * @param [out] DataPath_p
 *     Pointer to memory location where data path properties will be stored, may
 *     not be NULL
 *
 * @return 0 : success
 * @return -1: failure
 */
int
Driver164_DataPath_GetProperties(
        const unsigned int Index,
        Driver164_DataPath_t * const DataPath_p);


#endif /* DRIVER164_INIT_H_ */


/* end of file api_driver164_init.h */
