/*
 * LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
 * 
 * Copyright (c) 2020 United States Government as represented by
 * the Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file     cfe_missionlib_runtime.h
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * In typical ground/debug applications the entire EDS of the mission may be
 * statically linked into the executable using a special object named "MISSION_EDS"
 *
 * This header only provides an "extern" declaration for such an object.  It does not
 * define the object; that is up to the user application to do.
 */

#ifndef _CFE_MISSIONLIB_RUNTIME_H_
#define _CFE_MISSIONLIB_RUNTIME_H_

#include "cfe_mission_eds_parameters.h"
#include "cfe_sb_eds_typedefs.h"

#ifdef __cplusplus
extern "C"
{
#endif


void CFE_SB_MapListenerComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Listener_Component_t *Input);
void CFE_SB_UnmapListenerComponent(CFE_SB_Listener_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input);
void CFE_SB_MapPublisherComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Publisher_Component_t *Input);
void CFE_SB_UnmapPublisherComponent(CFE_SB_Publisher_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input);

uint8_t CFE_SB_PubSub_IsListenerComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params);
uint8_t CFE_SB_PubSub_IsPublisherComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params);

void CFE_SB_Get_PubSub_Parameters(CFE_SB_SoftwareBus_PubSub_Interface_t *Params, const CCSDS_SpacePacket_t *Packet);
void CFE_SB_Set_PubSub_Parameters(CCSDS_SpacePacket_t *Packet, const CFE_SB_SoftwareBus_PubSub_Interface_t *Params);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif  /* _CFE_MISSIONLIB_RUNTIME_H_ */

