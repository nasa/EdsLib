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
 * \file     cfe_missionlib_runtime_default.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Provides the conversions between CFE software bus abstract "MsgId" values
 * and the actual values used inside message headers.  This is actually compiled
 * a small self contained library such that it can be linked easily into both
 * CFE flight software as well as ground tools and utilities that also need to
 * interpret or generate software bus messages.
 *
 * Provided that everything links to this library and therefore uses the same
 * conversion logic, everything should stay synchronized all the time even if
 * the logic needs to change at some point.
 *
 * This also allows the logic to be customized per-mission.
 *
 * The default logic is to perform a bitwise shift and mapping, but this is
 * not required.  It can be arbitrary, e.g. a table lookup if necessary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ccsds_spacepacket_eds_typedefs.h"
#include "cfe_sb_eds_typedefs.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_missionlib_runtime.h"

#define CCSDS_GET_VER_FROM_HDR(hdr)     CCSDS_ ## hdr ## _VERSION
#define CCSDS_HDRTYPE_VERSION(x)        CCSDS_GET_VER_FROM_HDR(x)
#define CCSDS_ACTIVE_VERSION            CCSDS_HDRTYPE_VERSION(CCSDS_SPACEPACKET_HEADER_TYPE)
#define CCSDS_PriHdr_VERSION            1
#define CCSDS_APIDQHdr_VERSION          2

#define CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS   0x01
#define CFE_MISSIONLIB_MSGID_TELEMETRY_BITS     0x00

static inline uint16_t CFE_SB_Get_MsgId_Subsystem(const CFE_SB_MsgId_Atom_t *MsgId)
{
    return (*MsgId >> 8) & 0xFF;
}

static inline uint16_t CFE_SB_Get_MsgId_Apid(const CFE_SB_MsgId_Atom_t *MsgId)
{
    return (*MsgId >> 1) & 0x7F;
}

static inline uint16_t CFE_SB_Get_MsgId_InterfaceType(const CFE_SB_MsgId_Atom_t *MsgId)
{
    return (*MsgId & 0x01);
}

static inline void CFE_SB_Set_MsgId_Subsystem(CFE_SB_MsgId_Atom_t *MsgId, uint16_t Val)
{
    *MsgId |= (Val & 0xFF) << 8;
}

static inline void CFE_SB_Set_MsgId_Apid(CFE_SB_MsgId_Atom_t *MsgId, uint16_t Val)
{
    *MsgId |= (Val & 0x7F) << 1;
}

static inline void CFE_SB_Set_MsgId_InterfaceType(CFE_SB_MsgId_Atom_t *MsgId, uint16_t Val)
{
    *MsgId |= (Val & 0x01);
}

void CFE_SB_MapListenerComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Listener_Component_t *Input)
{
    Output->MsgId = 0;

    if (Input->Telecommand.InstanceNumber > 0 &&
            Input->Telecommand.InstanceNumber <=  0xFF &&
            Input->Telecommand.TopicId >= CFE_MISSION_TELECOMMAND_BASE_TOPICID &&
            Input->Telecommand.TopicId < CFE_MISSION_TELECOMMAND_MAX_TOPICID)
    {
        CFE_SB_Set_MsgId_InterfaceType(&Output->MsgId, CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS);
        CFE_SB_Set_MsgId_Apid(&Output->MsgId, Input->Telecommand.TopicId - CFE_MISSION_TELECOMMAND_BASE_TOPICID);
        CFE_SB_Set_MsgId_Subsystem(&Output->MsgId, Input->Telecommand.InstanceNumber);
    }
}

void CFE_SB_UnmapListenerComponent(CFE_SB_Listener_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    if (Input->MsgId == 0 || CFE_SB_Get_MsgId_InterfaceType(&Input->MsgId) != CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS)
    {
        memset(Output, 0, sizeof(*Output));
    }
    else
    {
        Output->Telecommand.TopicId = CFE_SB_Get_MsgId_Apid(&Input->MsgId) + CFE_MISSION_TELECOMMAND_BASE_TOPICID;
        Output->Telecommand.InstanceNumber = CFE_SB_Get_MsgId_Subsystem(&Input->MsgId);
    }
}

uint8_t CFE_SB_PubSub_IsListenerComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return (CFE_SB_Get_MsgId_InterfaceType(&Params->MsgId) == CFE_MISSIONLIB_MSGID_TELECOMMAND_BITS);
}

void CFE_SB_MapPublisherComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Publisher_Component_t *Input)
{
    Output->MsgId = 0;

    if (Input->Telemetry.InstanceNumber > 0 &&
            Input->Telemetry.InstanceNumber <=  0xFF &&
            Input->Telemetry.TopicId >= CFE_MISSION_TELEMETRY_BASE_TOPICID &&
            Input->Telemetry.TopicId < CFE_MISSION_TELEMETRY_MAX_TOPICID)
    {
        CFE_SB_Set_MsgId_InterfaceType(&Output->MsgId, CFE_MISSIONLIB_MSGID_TELEMETRY_BITS);
        CFE_SB_Set_MsgId_Apid(&Output->MsgId, Input->Telemetry.TopicId - CFE_MISSION_TELEMETRY_BASE_TOPICID);
        CFE_SB_Set_MsgId_Subsystem(&Output->MsgId, Input->Telemetry.InstanceNumber);
    }
}

void CFE_SB_UnmapPublisherComponent(CFE_SB_Publisher_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    if (Input->MsgId == 0 || CFE_SB_Get_MsgId_InterfaceType(&Input->MsgId) != CFE_MISSIONLIB_MSGID_TELEMETRY_BITS)
    {
        memset(Output, 0, sizeof(*Output));
    }
    else
    {
        Output->Telemetry.TopicId = CFE_SB_Get_MsgId_Apid(&Input->MsgId) + CFE_MISSION_TELEMETRY_BASE_TOPICID;
        Output->Telemetry.InstanceNumber = CFE_SB_Get_MsgId_Subsystem(&Input->MsgId);
    }
}

uint8_t CFE_SB_PubSub_IsPublisherComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return (Params->MsgId != 0 && CFE_SB_Get_MsgId_InterfaceType(&Params->MsgId) == CFE_MISSIONLIB_MSGID_TELEMETRY_BITS);
}

void CFE_SB_Get_PubSub_Parameters(CFE_SB_SoftwareBus_PubSub_Interface_t *Params, const CCSDS_SpacePacket_t *Packet)
{
    Params->MsgId = 0;

    if (Packet->Hdr.BaseHdr.SecHdrFlags != 0)
    {
        CFE_SB_Set_MsgId_InterfaceType(&Params->MsgId, Packet->Hdr.BaseHdr.SecHdrFlags >> 1);
#if CCSDS_ACTIVE_VERSION == CCSDS_PriHdr_VERSION
        CFE_SB_Set_MsgId_Apid(&Params->MsgId, Packet->Hdr.BaseHdr.AppId & 0x3F);
        CFE_SB_Set_MsgId_Subsystem(&Params->MsgId, Packet->Hdr.BaseHdr.AppId >> 6);
#elif CCSDS_ACTIVE_VERSION == CCSDS_APIDQHdr_VERSION
        CFE_SB_Set_MsgId_Apid(&Params->MsgId, Packet->Hdr.BaseHdr.AppId);
        CFE_SB_Set_MsgId_Subsystem(&Params->MsgId, Packet->Hdr.ApidQ.SubsystemId);
#else
#error "MsgId Mappings not defined for this header style"
#endif
    }
}

void CFE_SB_Set_PubSub_Parameters(CCSDS_SpacePacket_t *Packet, const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    Packet->Hdr.BaseHdr.SecHdrFlags = (CFE_SB_Get_MsgId_InterfaceType(&Params->MsgId) << 1) | 0x01;
#if CCSDS_ACTIVE_VERSION == CCSDS_PriHdr_VERSION
    Packet->Hdr.BaseHdr.AppId =
            (CFE_SB_Get_MsgId_Apid(&Params->MsgId) & 0x3F) |
            (CFE_SB_Get_MsgId_Subsystem(&Params->MsgId) << 6);
#elif CCSDS_ACTIVE_VERSION == CCSDS_APIDQHdr_VERSION
    Packet->Hdr.BaseHdr.AppId = CFE_SB_Get_MsgId_Apid(&Params->MsgId);
    Packet->Hdr.ApidQ.SubsystemId = CFE_SB_Get_MsgId_Subsystem(&Params->MsgId);
    Packet->Hdr.ApidQ.SystemId = 0x1234; /* not used yet */
#else
#error "MsgId Mappings not defined for this header style"
#endif

}

