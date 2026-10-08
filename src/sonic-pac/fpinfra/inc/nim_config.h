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

#ifndef NIM_CONFIG_H
#define NIM_CONFIG_H

#include "pacinfra_common.h"
#include "nim_data.h"

#define  NIM_FILE_NAME_LENGTH       20

#define NIM_CFG_VER_1  0x1
#define NIM_CFG_VER_2  0x2
#define NIM_CFG_VER_3  0x3
#define NIM_CFG_VER_4  0x4
#define NIM_CFG_VER_5  0x5
#define NIM_CFG_VER_CURRENT  NIM_CFG_VER_5
#define NIM_CFG_FILE_NAME  "nimStack.cfg"

/*********************************************************************
* @purpose  initialize the config structures during phase 3 init
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
void nimConfigInit(void);

/*********************************************************************
* @purpose  Setup the list of config file names
*
* @param    void
*
* @returns   SUCCESS  Success or sysapiRegistrGet error code
*
* @notes
*
*
* @end
*
*********************************************************************/
RC_t nimFileSetup(void);


/*********************************************************************
* @purpose  Saves all nim user config file to NVStore
*
* @param    void
*
* @returns   SUCCESS  if success
* @returns   ERROR    if error from osapiFsWrite
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimSave(void);

/*********************************************************************
* @purpose  Checks if nim user config data has changed
*
* @param    void
*
* @returns   TRUE or  FALSE
*
* @notes    none
*
* @end
*********************************************************************/
 BOOL nimHasDataChanged(void);
void nimResetDataChanged(void);

/*********************************************************************
* @purpose  Creates an unique interface id
*
* @param    usp         @b{(input)} Pointer to nimUSP_t structure
* @param    configId    @b{(output)} Pointer to the new config ID
*
* @returns   SUCCESS or  FALIURE
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimConfigIdCreate(nimUSP_t *usp, nimConfigID_t *configId);

/*********************************************************************
* @purpose  Allocate the memory for the memory copy of the config file
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimConfigPhase2Init(void);

/*********************************************************************
* @purpose  Migrate the config data
*
* @param    savedVersion  @b{(input)} version of the present config file
* @param    newVersion    @b{(input)} version to convert to
* @param    buffer        @b{(input)} the buffer of the cfg file
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
void nimConfigConvert(uint32 savedVersion,uint32 newVersion, char8 *buffer);

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
RC_t nimConfigSet(nimIntf_t *port, uint32 cfgVersion);

/*********************************************************************
* @purpose  apply the config to the system
*
* @param    intIfNum    @b{(input)}   The intenal interface to apply
*
* @returns   SUCCESS or  FALIURE
*
* @notes
*
* @end
*********************************************************************/
RC_t nimIntfConfigApply(uint32 intIfNum);

/*********************************************************************
* @purpose  Get the default config for the interface
*
* @param    intfDescr   @b{(input)}   A description of the interface being created
* @param    defaultCfg  @b{(output)}  The created config
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigDefaultGet(nimIntfDescr_t *intfDescr, nimIntfConfig_t *defaultCfg);

/*********************************************************************
* @purpose  Get the default config for the interface
*
* @param    version   @b{(input)} The version to create a default for
*
* @returns  void
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
void nimConfigFileDefaultCreate(uint32 version);

/*********************************************************************
* @purpose  Sets the config file header
*
* @param    none
*
* @returns   SUCCESS or  FALIURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigFileHdrCreate();

/*********************************************************************
* @purpose  Sets the offsets for ports and crc in the config struct
*
* @param    none
*
* @returns   SUCCESS or  FAILURE
*
* @notes    Since this is the first release of Stacking, no migration to do
*
* @end
*********************************************************************/
RC_t nimConfigFileOffsetsSet();

#endif /* NIM_CONFIG_H */
