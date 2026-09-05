/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

//#include "single.h"
#include "datatypes.h"
#include "packet.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_common.h"

typedef struct pac_global_oper_table_s
{
  /* number of authorized clients */
  uint32         authCount;       
  /* number of authorized clients in monitor mode */
  uint32         authCountMonMode;       
}pac_global_oper_table_t;

typedef struct pac_port_oper_table_s
{
  /* Authentication methods */
   AUTHMGR_METHOD_t           enabledMethods[AUTHMGR_METHOD_LAST];
  /* Authentication priority */
   AUTHMGR_METHOD_t           enabledPriority[AUTHMGR_METHOD_LAST];
}pac_port_oper_table_t;

typedef struct pac_authenticated_clients_oper_table_s
{
   uchar8                 currentIdL;       /* ID of current auth session (0-255) */
   AUTHMGR_PORT_STATUS_t  auth_status;
   AUTHMGR_METHOD_t       authenticatedMethod;
   uchar8                 serverState[AUTHMGR_SERVER_STATE_LEN];
  uint32                 serverStateLen;
   uchar8                 serverClass[AUTHMGR_SERVER_CLASS_LEN];
  uint32                 serverClassLen;
  uint32                 sessionTimeoutRcvdFromRadius;
  uint32                 sessionTimeoutOper;
   char8                  userName[AUTHMGR_USER_NAME_LEN];
  uint32                 userNameLen;
  uint32                 terminationAction;
  authmgrVlanType_t         vlanType;     /* assigned vlan category */
  uint32                 vlanId;
  uint32                 sessionTime;
  uint32                 lastAuthTime;
   USER_MGR_AUTH_METHOD_t backend_auth_method;

}pac_authenticated_clients_oper_table_t;


void PacAuthClientOperTblSet(uint32 intIfNum,  enetMacAddr_t macAddr, 
                            pac_authenticated_clients_oper_table_t *client_info);
void PacAuthClientOperTblDel(uint32 intIfNum,  enetMacAddr_t macAddr);

void PacGlobalOperTblSet(pac_global_oper_table_t *info);

void PacPortOperTblSet(uint32 intIfNum,  AUTHMGR_METHOD_t *enabledMethods, 
                        AUTHMGR_METHOD_t *enabledPriority);

void PacOperTblCleanup(void);

#ifdef __cplusplus
}
#endif
