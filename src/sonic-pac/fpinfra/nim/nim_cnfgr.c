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


#include "pacinfra_common.h"
#include "nim_data.h"
#include "nim_cnfgr.h"
#include "nim_util.h"
#include "nim_events.h"
#include "nimapi.h"
//#include "nim.h"
//#include "sysapi.h"
#include "platform_config.h"
#include "nim_config.h"
#include "avl_api.h"
#include "tree_api.h"
#include "osapi.h"
#include "osapi_sem.h"
#include "nim_ifindex.h"
#include "nim_startup.h"



/*********************************************************************
*
* @purpose  Compare two USP AVL keys and determine Greater, Less, or Equal
*
* @param    a    a void pointer to an AVL key comprised of a USP structure
* @param    b    a void pointer to an AVL key comprised of a USP structure
* @param    len  the length of the key in bytes (unused)
*                            
* @returns  1 if a > b
* @returns  -1 if a < b
* @returns  0 if a == b
*
* @notes    This function is used as a custom comparator in the
*           nimUspTreeData AVL tree.  It is also used by the
*           custom comparator used by the custom comparator
*           used in the nimConfigIdTreeData AVL tree.  The 'len'
*           parameter is unused because the length of a USP is
*           known, however, it is required for compatibility with
*           the comparator type definition.
*
* @end
*********************************************************************/
static int nimUspCompare(const void *a, const void *b, size_t len)
{
  nimUSP_t *ua = (nimUSP_t *)a;
  nimUSP_t *ub = (nimUSP_t *)b;

  if (ua->unit < ub->unit)
  {
    return -1;
  }
  if (ua->unit > ub->unit)
  {
    return 1;
  }
  if (ua->slot < ub->slot)
  {
    return -1;
  }
  if (ua->slot > ub->slot)
  {
    return 1;
  }
  if (ua->port < ub->port)
  {
    return -1;
  }
  if (ua->port > ub->port)
  {
    return 1;
  }
  return 0;
}

/*********************************************************************
*
* @purpose  Compare two nimConfigId_t AVL keys and determine Greater,
*           Less, or Equal
*
* @param    a    a void pointer to an AVL key comprised of a nimConfigId_t
* @param    b    a void pointer to an AVL key comprised of a nimConfigId_t
* @param    len  the length of the key in bytes (unused)
*                            
* @returns  1 if a > b
* @returns  -1 if a < b
* @returns  0 if a == b
*
* @notes    This function is used as a custom comparator in the
*           in the nimConfigIdTreeData AVL tree.  The 'len'
*           parameter is unused because the length of a nimConfigId_t
*           is known, however, it is required for compatibility with
*           the comparator type definition.
*
* @end
*********************************************************************/
static int nimConfigIdCompare(const void *a, const void *b, size_t len)
{
  nimConfigID_t *id_a = (nimConfigID_t *)a;
  nimConfigID_t *id_b = (nimConfigID_t *)b;

  if (id_a->type < id_b->type)
  {
    return -1;
  }
  if (id_a->type > id_b->type)
  {
    return 1;
  }
  /*
   * The types are the same, so compare them based on type...
   */
  switch (id_a->type)
  {
    case  PHYSICAL_INTF:
    case  STACK_INTF:
    case  CPU_INTF:
      return nimUspCompare(&id_a->configSpecifier.usp,
                        &id_b->configSpecifier.usp,
                        sizeof(nimUSP_t));
    case  LAG_INTF:
      return avlCompareULong32(&id_a->configSpecifier.dot3adIntf,
                             &id_b->configSpecifier.dot3adIntf,
                             sizeof(uint32));
    case  LOGICAL_VLAN_INTF:
      return avlCompareULong32(&id_a->configSpecifier.vlanId,
                             &id_b->configSpecifier.vlanId,
                             sizeof(uint32));
    case  LOOPBACK_INTF:
      return avlCompareULong32(&id_a->configSpecifier.loopbackId,
                             &id_b->configSpecifier.loopbackId,
                             sizeof(uint32));
    case  TUNNEL_INTF:
      return avlCompareULong32(&id_a->configSpecifier.tunnelId,
                             &id_b->configSpecifier.tunnelId,
                             sizeof(uint32));

    case  SERVICE_PORT_INTF:
      return avlCompareULong32(&id_a->configSpecifier.servicePortId,
                           &id_b->configSpecifier.servicePortId,
                           sizeof(uint32));
    default:
      /*
       * A node with an invalid type is a critical error.
       */
      NIM_LOG_ERROR("Bad config id type");
      return 0;
  }
}

/*********************************************************************
* @purpose  phase 1 to Initialize Network Interface Manager component   
*
* @param    void
*
* @returns  void
*
* @notes    none
*       
* @end
*********************************************************************/
RC_t   nimPhaseOneInit()
{
  RC_t rc =  SUCCESS;

  do
  {

    /* Not to be freed during operation */
    nimCtlBlk_g = osapiMalloc( NIM_COMPONENT_ID, sizeof(nimSystemData_t));

    if (nimCtlBlk_g ==  NULL)
    {
      NIM_LOG_ERROR("NIM Control block not created\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      /* all pointers and data with nimCtlBlk_g are to be set to 0 */
      memset((void *) nimCtlBlk_g, 0, sizeof (nimSystemData_t));
      //nimLoggingSet(0);
    }

    if (nimStartTask() !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Tasks and queues not created\n");
      rc =  FAILURE;
      break;
    }

    /* Allocate and initialize the necessary data elements of nimCtlBlk_g for 
       for phase 1 */
    nimCtlBlk_g->maxNumOfUnits                  = /*platUnitTotalMaxPerStackGet()*/  MAX_UNITS_PER_STACK;
    nimCtlBlk_g->maxNumOfSlotsPerUnit           = /*platSlotTotalMaxPerUnitGet()*/  MAX_SLOTS_PER_UNIT;
    nimCtlBlk_g->maxNumOfPhysicalPortsPerSlot   = /*platSlotMaxPhysicalPortsPerSlotGet()*/  MAX_PHYSICAL_PORTS_PER_SLOT;

    nimCtlBlk_g->nimNotifyList = osapiMalloc( NIM_COMPONENT_ID, sizeof(nimNotifyList_t) *  LAST_COMPONENT_ID);

    /* Not to be freed during operation */
    if (nimCtlBlk_g->nimNotifyList ==  NULL)
    {
      NIM_LOG_ERROR("NIM registrant notification list not created\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      memset(( void * )nimCtlBlk_g->nimNotifyList, 0, sizeof( nimNotifyList_t ) *  LAST_COMPONENT_ID ); 
    }

    /* Create NIM syncronization semaphore for the entire control block, should be used
       during all accesses to the control block in order to prevent contention.  NIM can
       be called at any time by multiple tasks */
    if (osapiRWLockCreate(&nimCtlBlk_g->rwLock,  OSAPI_RWLOCK_Q_PRIORITY) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM syncronization read write lock not created\n");
      rc =  FAILURE;
      break;
    }

    (void)nimIfIndexPhaseOneInit();
    (void)nimStartUpPhaseOneInit();

    /* Create the nimUSP AVL Tree */
    if (avlAllocAndCreateAvlTree(&nimCtlBlk_g->nimUspTreeData,
                                  NIM_COMPONENT_ID,
                                 platIntfTotalMaxCountGet(),
                                 sizeof(nimUspIntIfNumTreeData_t), 0x10,
                                 nimUspCompare, sizeof(nimUSP_t)) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Unable to allocate resources\n");
      break; /* goto while */
    }

    /* Create the nimConfigId AVL Tree */
    if (avlAllocAndCreateAvlTree(&nimCtlBlk_g->nimConfigIdTreeData,
                                  NIM_COMPONENT_ID,
                                 platIntfTotalMaxCountGet(),
                                 sizeof(nimConfigIdTreeData_t), 0x10,
                                 nimConfigIdCompare,
                                 sizeof(nimConfigID_t)) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Unable to allocate resources\n");
      break; /* goto while */
    }

    /* enable the trace utility for NIM */
    //nimTraceInit(NIM_TRACE_ENTRY_MAX, NIM_TRACE_ENTRY_SIZE_MAX);
    //nimProfileInit();

    /* Set the phase */
    nimCtlBlk_g->nimPhaseStatus =  CNFGR_STATE_P1;

  } while ( 0 );

  return(rc) ;
}

/*********************************************************************
* @purpose  phase 2 to Initialize Network Interface Manager component   
*
* @param    void
*
* @returns  void
*
* @notes    none
*       
* @end
*********************************************************************/
RC_t   nimPhaseTwoInit()
{

  RC_t   rc =  SUCCESS;

  do
  {
    /* Initialize the remote interface support module */
    /*rc = nimRifPhase2Init();

    if (rc !=  SUCCESS)
      break;*/

    if ((rc = nimIntfDataAlloc()) !=  SUCCESS)
    {
      rc =  FAILURE;
      break;
    }

    if (nimEventHdlrInit() !=  SUCCESS)
    {
      rc =  FAILURE;
      break;
    }

    /* allocate the memory for the memory copy of the config file */
    if (nimConfigPhase2Init() !=  SUCCESS)
    {
      rc =  FAILURE;
      break;
    }

 
    nimCtlBlk_g->nimPhaseStatus =  CNFGR_STATE_P2;

  } while (0);

  return(rc);
}


/*********************************************************************
* @purpose  phase 3 to Initialize Network Interface Manager component   
*
* @param    void
*
* @returns  void
*
* @notes    none
*       
* @end
*********************************************************************/
RC_t   nimPhaseThreeInit()
{
  RC_t           rc =  SUCCESS;  

  nimCtlBlk_g->ifNumber = 0;
  nimCtlBlk_g->ifTableLastChange = 0;
  nimCtlBlk_g->nimHighestIntfNumber = 0;

  /* read in the config file and set the datachanged flag appropriately */
  //nimConfigInit();

  //nimConfigIdTreePopulate();

  nimCtlBlk_g->nimPhaseStatus =  CNFGR_STATE_P3;

  return(rc);
}

/*********************************************************************
* @purpose  phase 5(exec) to Initialize Network Interface Manager component
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
RC_t   nimPhaseExecInit()
{
  RC_t           rc =  SUCCESS;

  nimCtlBlk_g->nimPhaseStatus =  CNFGR_STATE_E;

  return(rc);
}


/*********************************************************************
* @purpose  Allocate all of the interface related data
*
* @param    none
*
* @returns  RC_t  Returns  SUCCESS or  ERROR
*
* @notes    none
*       
* @end
*********************************************************************/
RC_t nimIntfDataAlloc()
{
  RC_t               rc =  SUCCESS;  

  do
  {
    /* Not freed during operation */
    nimCtlBlk_g->nimPorts = osapiMalloc( NIM_COMPONENT_ID, sizeof(nimIntf_t) * (platIntfTotalMaxCountGet() + 1));

    if (nimCtlBlk_g->nimPorts ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to alloc memory for nimPorts\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      /* reset the global data to known state */
      memset(( void * )nimCtlBlk_g->nimPorts, 
             0, 
             sizeof( nimIntf_t ) * (platIntfTotalMaxCountGet() + 1)); 
    }

    /* allocate/initialize the memory to store the number of ports per unit that have been created 
        Not freed during operation */
    nimCtlBlk_g->nimNumberOfPortsPerUnit = (uint32*)osapiMalloc( NIM_COMPONENT_ID, sizeof(uint32)*(nimCtlBlk_g->maxNumOfUnits+1));

    if (nimCtlBlk_g->nimNumberOfPortsPerUnit ==  NULL)
    {
      NIM_LOG_ERROR("NIM:Couldn't allocate memory for nimNumberOfPortPerUnit\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      memset((void*)nimCtlBlk_g->nimNumberOfPortsPerUnit, 0 , (sizeof(uint32)*(nimCtlBlk_g->maxNumOfUnits+1)));
    }

    /* Not freed during operation */
    nimCtlBlk_g->nimVlanSlotsInfo = osapiMalloc( NIM_COMPONENT_ID, sizeof(nimUSP_t)*platIntfVlanIntfMaxCountGet());

    if (nimCtlBlk_g->nimVlanSlotsInfo ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to alloc nimVlanSlotsInfo\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      memset((void *)nimCtlBlk_g->nimVlanSlotsInfo, 0, sizeof(nimUSP_t)*platIntfVlanIntfMaxCountGet());
    }

    /* Not freed during operation */
    nimCtlBlk_g->numberOfInterfacesByType = osapiMalloc( NIM_COMPONENT_ID, sizeof(uint32)* MAX_INTF_TYPE_VALUE);

    if (nimCtlBlk_g->numberOfInterfacesByType ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to alloc numberOfInterfacesByType\n");
      rc =  FAILURE;
      break;
    }
    else
    {
      memset((void *)nimCtlBlk_g->numberOfInterfacesByType, 0, sizeof(uint32)* MAX_INTF_TYPE_VALUE);
    }

    nimCtlBlk_g->intfTypeData = osapiMalloc ( NIM_COMPONENT_ID, sizeof(nimIntfTypeData_t) *  MAX_INTF_TYPE_VALUE);
    if (nimCtlBlk_g->intfTypeData ==  NULLPTR)
    {
      LOG_ERROR (0);
    }

  }while (0);

  return(rc);
}

/*********************************************************************
* @purpose  Initialize and start Card Manager Task function.
*
* @param    none
*
* @returns  RC_t  Returns  SUCCESS or  ERROR
*
* @notes    none
*       
* @end
*********************************************************************/
RC_t nimStartTask()
{

  RC_t rc =  SUCCESS;
   LOGF( LOG_SEVERITY_NOTICE, "nimStartTask started");

  do
  {

    nimCtlBlk_g->nimMsgQueue = (void *)osapiMsgQueueCreate("NIM-Q",/*nimSidMsgCountGet()*/ 16000,
                                                           (uint32)sizeof(nimPdu_t));
    if (nimCtlBlk_g->nimMsgQueue ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM queue not created\n");
      rc =  FAILURE;
      break;
    }

    nimCtlBlk_g->taskId = osapiTaskCreate("nim_t",nimTask,0, 0, 
                                          /* nimSidTaskStackSizeGet()*/  DEFAULT_STACK_SIZE, 
                                          /*nimSidTaskPriorityGet()*/  DEFAULT_TASK_PRIORITY, 
                                          /*nimSidTaskSliceGet()*/  DEFAULT_TASK_SLICE);    
    if (nimCtlBlk_g->taskId == 0)
    {
      NIM_LOG_ERROR("NIM task not created.\n");
      rc =  FAILURE;
      break;
    }

    if (osapiWaitForTaskInit ( NIM_TASK_SYNC,  WAIT_FOREVER) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Could not sync the TASK\n");
      rc =  FAILURE;
      break;
    }

  } while ( 0 );
  return(rc);
}

