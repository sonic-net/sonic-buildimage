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

#ifndef OSAPI_SEM_H
#define OSAPI_SEM_H

/*********************************************************************
*
* @structures OSAPI_SEM_B_STATE
*
* @purpose
*
* @comments
*
*********************************************************************/
typedef enum
{
  OSAPI_SEM_EMPTY,              /* 0: semaphore not available */
  OSAPI_SEM_FULL                /* 1: semaphore available */
} OSAPI_SEM_B_STATE;

/**************************************************************************
 *
 * @purpose  Create a binary Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options         @b{(input)}  queueing style - either Priority style or FIFO style
 * @param    initialState    @b{(input)}  FULL(available) or EMPTY (not available)
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API void *osapiSemaBCreateTrack ( char8 * name,  int32 inst,  int32 options,
                             OSAPI_SEM_B_STATE initial_state);
#define osapiSemaBCreate(options,initial_state) \
  osapiSemaBCreateTrack(__FILE__,__LINE__,options,initial_state)

/**************************************************************************
 *
 * @purpose  create a Mutual Exclusion Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options  @b{(input)}  queueing style - either Priority style or FIFO style
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API void *osapiSemaMCreateTrack ( char8 * name,  int32 inst,  int32 options);
#define osapiSemaMCreate(options) \
  osapiSemaMCreateTrack(__FILE__,__LINE__,options)

/**************************************************************************
 *
 * @purpose  Take a Semaphore
 *
 * @param    SemaID    @b{(input)}  ID of the requested semaphore returned from the create
 * @param    timeout   @b{(input)}  time to wait in milliseconds, forever (-1), or no wait (0)
 *
 * @returns   SUCCESS
 * @returns   FAILURE if timeout or if semaphore does not exist
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiSemaTake (void *SemaID,  int32 timeout);

/**************************************************************************
 *
 * @purpose  Give a Semaphore
 *
 * @param    SemaID @b{(input)}  ID of the semaphore, returned from the create, to release
 *
 * @returns   SUCCESS
 * @returns   FAILURE if semaphore is invalid
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiSemaGive (void *SemaID);

/**************************************************************************
 *
 * @purpose  Unblock any and all tasks waiting on the semaphore
 *
 * @param    SemaID    @b{(input)}  ID of the semaphore returned from the create
 *
 * @returns   SUCCESS
 * @returns   FAILURE if semaphore is invalid
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiSemaFlush (void *SemaID);

/**************************************************************************
 *
 * @purpose  Delete a Semaphore
 *
 * @param    SemaID    @b{(input)}  ID of the semaphore to delete
 *
 * @returns   SUCCESS
 * @returns   FAILURE if semaphore is invalid
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiSemaDelete (void *SemaID);

/**************************************************************************
 *
 * @purpose  create a Counting Semaphore
 *
 * @param    name @b{(input)}           name of the semaphore
 * @param    inst @b{(input)}           optional instance of semaphore
 *                                      value = 0 to ignore
 * @param    options         @b{(input)}  queueing style - either Priority style or FIFO style
 * @param    initialCount    @b{(input)}  initialized to the specified initial count
 *
 * @returns  ptr to the semaphore or NULL if memory cannot be allocated
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API void *osapiSemaCCreateTrack ( char8 * name,  int32 inst,  int32 options,
                              int32 initialCount);
#define osapiSemaCCreate(options,initialCount) \
  osapiSemaCCreateTrack(__FILE__,__LINE__,options,initialCount)

/**************************************************************************
 *
 * @purpose  Give the IO Semaphore
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE if semaphore is invalid
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiIOSemaGive (void);

/**************************************************************************
 *
 * @purpose  Take the IO Semaphore
 *
 * @param    timeout   @b{(input)}  time to wait in ticks, forever (-1), or no wait (0)
 *
 * @returns   SUCCESS
 * @returns   FAILURE if timeout or if semaphore does not exist
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiIOSemaTake ( int32 timeout);

#endif /* OSAPI_SEM_H */
