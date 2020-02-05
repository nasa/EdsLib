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
 * \file     ut_missionlib_stubs.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * Implementation of unit testing stubs for the CFE-EDS mission integration
 */

/*
** Include section
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "utstubs.h"

/*
 * A macro for a generic stub function that has no return code
 * (note this still invokes the UT call, so that user-specified hooks will work)
 */
#define MISSIONLIB_VOID_STUB(Func,Args)       \
    void Func Args { UT_DefaultStubImpl(#Func, UT_KEY(Func), 0); }

/*
 * A macro for a generic stub function that has an output parameter which is a
 * pointer to fixed-type blob (common paradigm in EdsLib lookup functions)
 */
#define MISSIONLIB_OUTPUT_STUB(Func,Obj,Default,Args)   \
    int32_t Func Args { return CFE_MissionLib_UT_GenericStub(#Func, UT_KEY(Func), Obj, Default, sizeof(*Obj)); }

/*
 * A macro for a generic stub function that has an output parameter which is a
 * pointer to fixed-type blob (common paradigm in EdsLib lookup functions)
 */
#define MISSIONLIB_NAME_STUB(Func,Args)   \
    const char * Func Args { return CFE_MissionLib_UT_NameStub(#Func, UT_KEY(Func)); }

static const char * CFE_MissionLib_UT_NameStub(const char *FuncName, UT_EntryKey_t FuncKey)
{
    const char *Result;
    static const char *DEFAULT_NAME = "UT";

    if (UT_DefaultStubImpl(FuncName, FuncKey, CFE_MISSIONLIB_SUCCESS) != CFE_MISSIONLIB_SUCCESS)
    {
        Result = NULL;
    }
    else if (UT_Stub_CopyToLocal(FuncKey, (uint8_t*)&Result, sizeof(Result)) < sizeof(Result))
    {
        Result = DEFAULT_NAME;
    }

    return Result;
}

static int32_t CFE_MissionLib_UT_GenericStub(const char *FuncName, UT_EntryKey_t FuncKey, void *OutputObj, const void *Default, uint32_t OutputSize)
{
    int32_t Result;
    uint32_t CopySize;

    Result = UT_DefaultStubImpl(FuncName, FuncKey, CFE_MISSIONLIB_SUCCESS);

    if (OutputSize > 0)
    {
        if (Result < CFE_MISSIONLIB_SUCCESS)
        {
            /*
             * Fill the output object with some pattern.
             * The result was failure so the user code should _not_ be looking at this.
             * Putting nonzero garbage data in here may catch violators that _do_ look at it.
             */
            memset(OutputObj, 0xA5, OutputSize);
        }
        else
        {
            CopySize = UT_Stub_CopyToLocal(FuncKey, OutputObj, OutputSize);
            if (CopySize < OutputSize)
            {
                if (Default == NULL)
                {
                    /*
                     * Fill out the _remainder_ of the output object with a pattern.
                     * Same logic as above, to catch potential issues with user code that
                     * are potentially looking at data beyond what was really filled
                     */
                    memset((uint8_t*)OutputObj + CopySize, 0x5A, OutputSize - CopySize);
                }
                else
                {
                    memcpy((uint8_t*)OutputObj + CopySize, (uint8_t*)Default + CopySize, OutputSize - CopySize);
                }
            }
        }
    }

    return Result;
}


MISSIONLIB_NAME_STUB(CFE_MissionLib_GetInstanceName,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InstanceNum, char *Buffer, uint32_t BufferSize))

uint16_t CFE_MissionLib_GetInstanceNumber(const CFE_MissionLib_SoftwareBus_Interface_t *Intf, const char *String)
{
    int32_t Retcode;
    uint16_t InstanceNumber;

    Retcode = UT_DEFAULT_IMPL(CFE_MissionLib_GetInstanceNumber);
    if (Retcode > 0 && Retcode < 0xFFFF)
    {
        InstanceNumber = Retcode;
    }
    else
    {
        InstanceNumber = 1;
    }

    return InstanceNumber;
}

static const CFE_MissionLib_IndicationInfo_t UT_DEFAULT_INDINFO =
{
        .NumArguments = 1,
        .SubcommandArgumentId = 1,
        .NumSubcommands = 1
};

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_GetIndicationInfo, IndInfo, &UT_DEFAULT_INDINFO,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, uint16_t TopicId,
                uint16_t IndicationId, CFE_MissionLib_IndicationInfo_t *IndInfo))

static const CFE_MissionLib_InterfaceInfo_t UT_DEFAULT_INTFINFO =
{
        .NumCommands = 1,
        .NumTopics = 1
};

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_GetInterfaceInfo, IntfInfo, &UT_DEFAULT_INTFINFO,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, CFE_MissionLib_InterfaceInfo_t *IntfInfo))

static const CFE_MissionLib_TopicInfo_t UT_DEFAULT_TOPICINFO =
{
        .DispatchTableId = 1,
        .DispatchStartOffset = 0
};

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_GetTopicInfo, TopicInfo, &UT_DEFAULT_TOPICINFO,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf, uint16_t InterfaceType,
                uint16_t TopicId, CFE_MissionLib_TopicInfo_t *TopicInfo))

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_GetArgumentType, Id, NULL,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, uint16_t TopicId,
                uint16_t IndicationId, uint16_t ArgumentId, EdsLib_Id_t *Id))

static const uint16 UT_DEFAULT_NUMBER = 1;

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_GetSubcommandOffset, Offset, &UT_DEFAULT_NUMBER,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, uint16_t TopicId,
                uint16_t IndicationId, uint16_t SubcommandId, uint16_t *Offset))


MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_FindInterfaceByName, InterfaceIdBuffer, &UT_DEFAULT_NUMBER,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                const char *IntfName, uint16_t *InterfaceIdBuffer))

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_FindTopicByName, TopicIdBuffer, &UT_DEFAULT_NUMBER,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, const char *TopicName, uint16_t *TopicIdBuffer))

MISSIONLIB_OUTPUT_STUB(CFE_MissionLib_FindCommandByName, CommandIdBuffer, &UT_DEFAULT_NUMBER,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, const char *CommandName, uint16_t *CommandIdBuffer))

MISSIONLIB_NAME_STUB(CFE_MissionLib_GetTopicName,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, uint16_t TopicId))

MISSIONLIB_NAME_STUB(CFE_MissionLib_GetInterfaceName,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType))

MISSIONLIB_NAME_STUB(CFE_MissionLib_GetCommandName,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, uint16_t CommandId))

MISSIONLIB_VOID_STUB(CFE_MissionLib_EnumerateTopics,
        (const CFE_MissionLib_SoftwareBus_Interface_t *Intf,
                uint16_t InterfaceType, CFE_MissionLib_TopicInfo_Callback_t Callback, void *OpaqueArg))

uint8_t CFE_SB_PubSub_IsListenerComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return UT_DEFAULT_IMPL(CFE_SB_PubSub_IsListenerComponent);
}

uint8_t CFE_SB_PubSub_IsPublisherComponent(const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    return UT_DEFAULT_IMPL(CFE_SB_PubSub_IsPublisherComponent);
}

void CFE_SB_MapListenerComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Listener_Component_t *Input)
{
    UT_DEFAULT_IMPL(CFE_SB_MapListenerComponent);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_MapListenerComponent), (uint8_t*)Output, sizeof(*Output)) < sizeof(*Output))
    {
        Output->MsgId = Input->Telecommand.TopicId;
    }
}

void CFE_SB_UnmapListenerComponent(CFE_SB_Listener_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    UT_DEFAULT_IMPL(CFE_SB_UnmapListenerComponent);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_UnmapListenerComponent), (uint8_t*)Output, sizeof(*Output)) < sizeof(*Output))
    {
        Output->Telecommand.TopicId = Input->MsgId;
    }
}

void CFE_SB_MapPublisherComponent(CFE_SB_SoftwareBus_PubSub_Interface_t *Output, const CFE_SB_Publisher_Component_t *Input)
{
    UT_DEFAULT_IMPL(CFE_SB_MapPublisherComponent);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_MapPublisherComponent), (uint8_t*)Output, sizeof(*Output)) < sizeof(*Output))
    {
        Output->MsgId = Input->Telemetry.TopicId;
    }
}

void CFE_SB_UnmapPublisherComponent(CFE_SB_Publisher_Component_t *Output, const CFE_SB_SoftwareBus_PubSub_Interface_t *Input)
{
    UT_DEFAULT_IMPL(CFE_SB_UnmapPublisherComponent);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_UnmapPublisherComponent), (uint8_t*)Output, sizeof(*Output)) < sizeof(*Output))
    {
        Output->Telemetry.TopicId = Input->MsgId;
    }
}

void CFE_SB_Get_PubSub_Parameters(CFE_SB_SoftwareBus_PubSub_Interface_t *Params, const CCSDS_SpacePacket_t *Packet)
{
    UT_DEFAULT_IMPL(CFE_SB_Get_PubSub_Parameters);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_Get_PubSub_Parameters), (uint8_t*)Params, sizeof(*Params)) < sizeof(*Params))
    {
        Params->MsgId = Packet->Hdr.BaseHdr.AppId | (Packet->Hdr.BaseHdr.SecHdrFlags << 11);
    }
}


void CFE_SB_Set_PubSub_Parameters(CCSDS_SpacePacket_t *Packet, const CFE_SB_SoftwareBus_PubSub_Interface_t *Params)
{
    UT_DEFAULT_IMPL(CFE_SB_Set_PubSub_Parameters);

    if (UT_Stub_CopyToLocal(UT_KEY(CFE_SB_Set_PubSub_Parameters), (uint8_t*)Packet, sizeof(*Packet)) < sizeof(*Packet))
    {
        Packet->Hdr.BaseHdr.SecHdrFlags = Params->MsgId >> 11;
        Packet->Hdr.BaseHdr.AppId = Params->MsgId & 0x7FF;
    }
}
