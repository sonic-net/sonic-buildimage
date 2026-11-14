/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef NIM_UTIL_H
#define NIM_UTIL_H

#include "datatypes.h"
#include "commdefs.h"
#include "osapi.h"
#include "log.h"
#include "nim_data.h"
#include "nim_trace.h"
#include "platform_config.h"

#define NIM_LOG_MSG(format, args...) \
       {   \
          syslog( LOG_SEVERITY_INFO, format, ## args); \
       }

#define NIM_LOG_ERROR(format, args...) \
       {   \
          syslog(LOG_ERR, format, ## args); \
       }

#define IS_INTIFNUM_PRESENT(_intIfNum,_rc)                                  \
   {                                                                        \
      if ( ( _intIfNum < 1 ) || ( _intIfNum > platIntfTotalMaxCountGet()) ) \
      {                                                                     \
         _rc =  (  FAILURE );                                             \
      } else                                                                \
      if ( nimCtlBlk_g->nimPorts[_intIfNum].present !=  TRUE )            \
      {                                                                     \
         _rc =  (  ERROR );                                               \
      }                                                                     \
      else                                                                  \
      {                                                                     \
        _rc =  SUCCESS;                                                   \
      }                                                                     \
   }

#define NIM_CRIT_SEC_READ_ENTER()                            \
  {                                                          \
    nimTraceCriticalSection(0, 1);                           \
    osapiReadLockTake(nimCtlBlk_g->rwLock,  WAIT_FOREVER); \
  }

#define NIM_CRIT_SEC_READ_EXIT()            \
  {                                         \
    nimTraceCriticalSection(0, 0);          \
    osapiReadLockGive(nimCtlBlk_g->rwLock); \
  }

#define NIM_CRIT_SEC_WRITE_ENTER()                            \
  {                                                           \
    nimTraceCriticalSection(1, 1);                            \
    osapiWriteLockTake(nimCtlBlk_g->rwLock,  WAIT_FOREVER); \
  }

#define NIM_CRIT_SEC_WRITE_EXIT()            \
  {                                          \
    nimTraceCriticalSection(1, 0);           \
    osapiWriteLockGive(nimCtlBlk_g->rwLock); \
  }

/* Trace messages are logged and printed only if the nimPrintLog is set to true */
#define NIM_TRACE_MSG(format, args...) \
          nimTraceMsg (__FILE__, __LINE__, format, ## args)


/*
  Expandable Port Macros for PortModeFlags 16 bits field.
  MSB is l7_portmode_t. LSB has port mode status and enable bits
 */
/* Port Status Bit position 5th LSB */
#define NIM_EXP_STATUS_BIT_POS 1


  /* Port mode status Helper Masks */
  /* To enable need 1 in STATUS position bit */
#define NIM_EXP_STATUS_ENABLE_MASK ( ushort16)(0x1)

  /* To disable 0 in STATUS position */
#define NIM_EXP_STATUS_DISABLE_MASK ( ushort16)(~NIM_EXP_STATUS_ENABLE_MASK)


  /* This Macros below are for internal NIM Usage only */
#define NIM_EXP_PORT_MODE_STATUS_ENABLE(_portModeFlags) \
  do { (_portModeFlags) |= (NIM_EXP_STATUS_ENABLE_MASK); } while(0)

#define NIM_EXP_PORT_MODE_STATUS_DISABLE(_portModeFlags)        \
  do { (_portModeFlags) &= (NIM_EXP_STATUS_DISABLE_MASK); } while(0)

#define NIM_EXP_PORT_MODE_STATUS_GET(_portModeFlags)    \
  ((uint32)((_portModeFlags) & NIM_EXP_STATUS_ENABLE_MASK))

#define NIM_EXP_IS_PORT_MODE_STATUS_ENABLED(_portModeFlags)             \
  (NIM_EXP_PORT_MODE_STATUS_GET(_portModeFlags) & (uint32)0x1)

#define NIM_EXP_PORT_MODE_STATUS_SET(_portModeFlags, _val)       \
  if (( BOOL)(_val)) {                                         \
    NIM_EXP_PORT_MODE_STATUS_ENABLE(_portModeFlags);             \
  } else {                                                       \
    NIM_EXP_PORT_MODE_STATUS_DISABLE(_portModeFlags);            \
  }

/* Port mode helper macros */
#define NIM_EXP_QUAD_100G_ENABLE_MASK  ( ushort16)( EXP_MODE_QUAD_100G << 0x8)
#define NIM_EXP_QUAD_40G_ENABLE_MASK   ( ushort16)( EXP_MODE_QUAD_40G << 0x8)
#define NIM_EXP_SINGLE_10G_ENABLE_MASK ( ushort16)( EXP_MODE_SINGLE_10G << 0x8)
#define NIM_EXP_SINGLE_25G_ENABLE_MASK ( ushort16)( EXP_MODE_SINGLE_25G << 0x8)
#define NIM_EXP_DUAL_50G_ENABLE_MASK   ( ushort16)( EXP_MODE_DUAL_50G << 0x8)
#define NIM_EXP_ROOT_DUAL_50G ENABLE_MASK ( ushort16)( EXP_MODE_ROOT_DUAL_50G << 0x8)
#define NIM_EXP_DUAL_25G_ENABLE_MASK   ( ushort16)( EXP_MODE_DUAL_25G << 0x8)

#define NIM_EXP_MODE_ENABLE_MASK ( ushort16)(0xFF00)

#define NIM_EXP_MODE_DISABLE_MASK (~NIM_EXP_MODE_ENABLE_MASK)

#define NIM_EXP_PORT_MODE_GET(_portModeFlags)           \
  ((uint32)(( ushort16)(_portModeFlags) >> 8))

#define NIM_EXP_IS_PORT_MODE_DUAL_50G(_portModeFlags)                   \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_DUAL_50G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_SINGLE_10G(_portModeFlags)                 \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_SINGLE_10G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_SINGLE_25G(_portModeFlags)                 \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_SINGLE_25G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_QUAD_100G(_portModeFlags)                  \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_QUAD_100G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_QUAD_40G(_portModeFlags)                   \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_QUAD_40G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_DUAL_25G(_portModeFlags)                   \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_DUAL_25G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_SINGLE_50G(_portModeFlags)                 \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == NIM_EXP_SINGLE_50G_ENABLE_MASK)

#define NIM_EXP_IS_PORT_MODE_NONE(_portModeFlags)                \
  (NIM_EXP_PORT_MODE_GET(_portModeFlags) == ( portmode_t) PORT_PORTMODE_NONE)

#define NIM_EXP_PORT_MODE_SET(_portModeFlags, _val)                     \
  do {                                                                  \
    ((_portModeFlags) &= ( ushort16) NIM_EXP_MODE_DISABLE_MASK);      \
    ((_portModeFlags) |= (( ushort16)(_val) << 0x8));                 \
  } while (0)


#define NIM_EXP_PORT_MODE_SET_1x400G(_portModeFlags)                 \
  NIM_EXP_PORT_MODE_SET ((_portModeFlags),  EXP_MODE_1x400G)

#define NIM_EXP_PORT_MODE_SET_QUAD_100G(_portModeFlags)                 \
  NIM_EXP_PORT_MODE_SET ((_portModeFlags),  EXP_MODE_QUAD_100G)

#define NIM_EXP_PORT_MODE_SET_QUAD_40G(_portModeFlags)                  \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_QUAD_40G)

#define NIM_EXP_PORT_MODE_SET_SINGLE_10G(_portModeFlags)                \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_SINGLE_10G)

#define NIM_EXP_PORT_MODE_SET_SINGLE_25G(_portModeFlags)                \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_SINGLE_25G)

#define NIM_EXP_PORT_MODE_SET_DUAL_50G(_portModeFlags)                  \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_DUAL_50G)

#define NIM_EXP_PORT_MODE_SET_SINGLE_50G(_portModeFlags)                \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_ROOT_DUAL_50G)

#define NIM_EXP_PORT_MODE_SET_DUAL_25G(_portModeFlags)                \
  NIM_EXP_PORT_MODE_SET((_portModeFlags),  EXP_MODE_DUAL_25G)

#define NIM_EXP_PORT_MODE_SET_NONE(_portModeFlags)      \
  do { ((_portModeFlags) &= ( ushort16) NIM_EXP_MODE_DISABLE_MASK); } while (0)


#define NIM_EXP_PORT_FLAGS_BY_INTF_TYPE(__type, __flags)                \
  do {                                                                  \
    if ( IANA_400G_ETHERNET == (__type))                              \
    {                                                                   \
     NIM_EXP_PORT_MODE_SET_1x400G ((__flags));                          \
    }                                                                   \
    else if ( IANA_100G_ETHERNET == (__type))                         \
    {                                                                   \
      NIM_EXP_PORT_MODE_SET_QUAD_100G((__flags));                       \
    }                                                                   \
    else if ( IANA_40G_ETHERNET == (__type))                          \
    {                                                                   \
      NIM_EXP_PORT_MODE_SET_QUAD_40G((__flags));                        \
    }                                                                   \
    else if ( IANA_50G_ETHERNET == (__type))                          \
    {                                                                   \
      NIM_EXP_PORT_MODE_SET_DUAL_50G((__flags));                        \
    }                                                                   \
    else if ( IANA_25G_ETHERNET == (__type))                          \
    {                                                                   \
      NIM_EXP_PORT_MODE_SET_SINGLE_25G((__flags));                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
      NIM_EXP_PORT_MODE_SET_SINGLE_10G((__flags));                      \
    }                                                                   \
  } while(0)

/*********************************************************************
* @purpose  create a unit slot port mapping to interface number
*
* @param    usp         Pointer to nimUSP_t structure
* @param    intIntfNum    interface number
*
* @returns   SUCCESS or  FALIURE
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimUnitSlotPortToIntfNumSet(nimUSP_t *usp, uint32 intIntfNum);

/*********************************************************************
* @purpose  Deletes the specified interface
*
* @param    intIfNum Internal Interface Number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimDeleteInterface(uint32 intIfNum);

/*********************************************************************
* @purpose  Create the internal interface number for the interface
*
* @param    configId  @b{(input)}  The configuration ID to be searched
* @param    intIfNum  @b{(output)} Internal Interface Number created
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero
*
* @end
*********************************************************************/
RC_t nimIntIfNumCreate(nimConfigID_t configId, uint32 *intIfNum);

/*********************************************************************
* @purpose  Create the internal interface number for the interface
*
* @param    intIfNum  @b{(input)} Internal Interface Number deleted
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero
*
* @end
*********************************************************************/
RC_t nimIntIfNumDelete(uint32 intIfNum);

/*********************************************************************
* @purpose  Determine the next State to transition to
*
* @param    currState    @b{(input)}    The current state
* @param    event        @b{(input)}    The event being processed
* @param    nextState    @b{(output)}   The next state to transition to
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimUtilIntfNextStateGet( INTF_STATES_t currState, PORT_EVENTS_t event, INTF_STATES_t *nextState);

/*********************************************************************
* @purpose  Add an entry to the nim nimConfigIdTreeData AVL tree
*
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryAdd(  nimConfigIdTreeData_t  *pConfigIdInfo);

/*********************************************************************
* @purpose  Delete an entry from the nim nimConfigIdTreeData AVL tree
*
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryDelete(  nimConfigIdTreeData_t  *pConfigIdInfo);

/*********************************************************************
* @purpose  Get an entry from the nim nimConfigIdTreeData AVL tree
*
* @param    *pConfigId    @b{(input)} pointer to a nimConfigID_t structure
* @param    *pConfigIdInfo    @b{(input)} pointer to a nimConfigIdTreeData_t structure
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimConfigIdTreeEntryGet(nimConfigID_t  *pConfigId,
                                nimConfigIdTreeData_t  *pConfigIdInfo );

/*********************************************************************
* @purpose  Set either the saved config or the default config in the interface
*
* @param    port        @b{(input)}   The interface being manipulated
* @param    cfgVersion  @b{(input)}   version of the config
*
* @returns   SUCCESS or  FALIURE
*
* @notes
*
* @end
*********************************************************************/
void nimConfigIdTreePopulate(void);

/*********************************************************************
* @purpose  Check to see if the number of interfaces per type are exceeded
*
* @param    intfType  @b{(input)} one of  INTF_TYPES_t
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero
*
* @end
*********************************************************************/
 BOOL nimNumberOfInterfaceExceeded( INTF_TYPES_t intfType);

/*********************************************************************
* @purpose  Get the maximum number of interfaces for an interface type
*
* @param    intfType  @b{(input)} one of  INTF_TYPES_t
*
* @returns  the maximum number of interfaces for an interface type, 0 or greater
*
* @notes    Returns zero if an interface type is unrecognized
*
* @end
*********************************************************************/
 int32 nimMaxIntfForIntfTypeGet( INTF_TYPES_t intfType);

/*********************************************************************
* @purpose  Delete the internal interface number for the interface
*
* @param    configId  @b{(input)}  The configuration ID to be removed
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    If the action fails, the intIfNum will be set to zero
*
* @end
*********************************************************************/
RC_t nimConfigInterfaceDelete(nimConfigID_t configId);

#define NIM_EXP_PORT_DBG(__fmt,__args...)                               \
  do {                                                                  \
    /*if (nimExpDebugFlagGet() & 0x1)*/                          {          \
      sysapiPrintf("%s:%d " __fmt, __func__, __LINE__,##__args);        \
    }                                                                   \
  } while (0)

/*********************************************************************
* @purpose  return the configuration phase
*
* @param     none
*
* @returns  the phase number
*
* @notes
*
* @end
*********************************************************************/
 CNFGR_STATE_t nimConfigPhaseGet(void);

/*********************************************************************
* @purpose  Get the state of an interface
*
* @param    intIfNum    The internal interface number
*
* @returns  The state of the interface
*
* @notes    none
*
* @end
*********************************************************************/
 INTF_STATES_t nimUtilIntfStateGet(uint32 intIfNum);

/*********************************************************************
* @purpose  Set the state of an interface
*
* @param    intIfNum    The internal interface number
* @param    state       The state of the interface from the  INTF_STATES_t
*
* @returns   SUCCESS or  FALIURE
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimUtilIntfStateSet(uint32 intIfNum,  INTF_STATES_t state);


/*********************************************************************
* @purpose  Log NIM Message at ERROR Severity
*
* @param     fileName - file
* @param     lineNum - Line
* @param     format - Format of the output.
* @param     ... - Variable output list.
*
* @returns   nothing
*
* @notes
*
* @end
*********************************************************************/
void nimLogErrorMsg ( BOOL logError,  char8 * fileName, uint32 lineNum,  char8 * format, ...) PRINTF_FMT(4,5);
#endif /* NIM_UTIL_H */
