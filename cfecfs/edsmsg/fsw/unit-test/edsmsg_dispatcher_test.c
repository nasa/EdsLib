/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#include <string.h>
#include "cfe_error.h"
#include "cfe_msg.h"
#include "edsmsg_dispatcher.h"
#include "edslib_datatypedb.h"
#include "edslib_intfdb.h"
#include "cfe_missionlib_api.h"
#include "cfe_missionlib_runtime.h"
#include "utassert.h"
#include "utstubs.h"
#include "uttest.h"

#define TEST_COMPONENT    EDSLIB_INTF_ID(1, 1)
#define TEST_BASE_TYPE    EDSLIB_MAKE_ID(1, 1)
#define TEST_DERIVED_TYPE EDSLIB_MAKE_ID(1, 2)

static CFE_SB_Buffer_t                          Buffer;
static CFE_MSG_Size_t                           MessageSize;
static EdsLib_DataTypeDB_DerivedTypeInfo_t      DerivedInfo;
static EdsLib_DataTypeDB_DerivativeObjectInfo_t IdentifiedType;
static EdsLib_DataTypeDB_TypeInfo_t             TypeInfo;
static EdsLib_Id_t                              LookedUpType;
static unsigned int                             HandlerCalls[2];

static CFE_Status_t Handler0(const CFE_SB_Buffer_t *Message)
{
    UtAssert_True(Message == &Buffer, "Original buffer passed to handler zero");
    ++HandlerCalls[0];
    return 10;
}

static CFE_Status_t Handler1(const CFE_SB_Buffer_t *Message)
{
    UtAssert_True(Message == &Buffer, "Original buffer passed to handler one");
    ++HandlerCalls[1];
    return 11;
}

static void GetInterface(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    EdsLib_IntfDB_InterfaceInfo_t *Info =
        UT_Hook_GetArgValueByName(Context, "IntfInfoBuffer", EdsLib_IntfDB_InterfaceInfo_t *);
    memset(Info, 0, sizeof(*Info));
    Info->ParentCompEdsId = TEST_COMPONENT;
}

static void GetId(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    *UT_Hook_GetArgValueByName(Context, "IdBuffer", EdsLib_Id_t *) = TEST_BASE_TYPE;
}

static void GetDerived(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    *UT_Hook_GetArgValueByName(Context, "DerivInfo", EdsLib_DataTypeDB_DerivedTypeInfo_t *) = DerivedInfo;
}

static void Identify(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    *UT_Hook_GetArgValueByName(Context, "DerivObjInfo", EdsLib_DataTypeDB_DerivativeObjectInfo_t *) = IdentifiedType;
}

static void GetType(void *UserObj, UT_EntryKey_t FuncKey, const UT_StubContext_t *Context)
{
    LookedUpType = UT_Hook_GetArgValueByName(Context, "EdsId", EdsLib_Id_t);
    *UT_Hook_GetArgValueByName(Context, "TypeInfo", EdsLib_DataTypeDB_TypeInfo_t *) = TypeInfo;
}

static void Setup(void)
{
    UT_ResetState(0);
    memset(&Buffer, 0, sizeof(Buffer));
    memset(&DerivedInfo, 0, sizeof(DerivedInfo));
    memset(&IdentifiedType, 0, sizeof(IdentifiedType));
    memset(&TypeInfo, 0, sizeof(TypeInfo));
    memset(HandlerCalls, 0, sizeof(HandlerCalls));
    MessageSize                         = sizeof(CFE_MSG_CommandHeader_t);
    TypeInfo.Size.Bytes                 = MessageSize;
    DerivedInfo.NumDerivatives          = 2;
    IdentifiedType.EdsId                = TEST_DERIVED_TYPE;
    IdentifiedType.DerivativeTableIndex = 1;
    LookedUpType                        = EDSLIB_ID_INVALID;
    UT_SetDefaultReturnValue(UT_KEY(CFE_MissionLib_PubSub_IsListenerComponent), 1);
    UT_SetDataBuffer(UT_KEY(CFE_MSG_GetSize), &MessageSize, sizeof(MessageSize), false);
    UT_SetHandlerFunction(UT_KEY(EdsLib_IntfDB_GetComponentInterfaceInfo), GetInterface, NULL);
    UT_SetHandlerFunction(UT_KEY(EdsLib_IntfDB_FindAllCommands), GetId, NULL);
    UT_SetHandlerFunction(UT_KEY(EdsLib_IntfDB_FindAllArgumentTypes), GetId, NULL);
    UT_SetHandlerFunction(UT_KEY(EdsLib_DataTypeDB_GetDerivedInfo), GetDerived, NULL);
    UT_SetHandlerFunction(UT_KEY(EdsLib_DataTypeDB_IdentifyBufferWithSize), Identify, NULL);
    UT_SetHandlerFunction(UT_KEY(EdsLib_DataTypeDB_GetTypeInfo), GetType, NULL);
}

static CFE_Status_t Dispatch(void)
{
    CFE_Status_t (*const Handlers[])(const CFE_SB_Buffer_t *) = { Handler0, Handler1 };
    return CFE_EDSMSG_Dispatch(TEST_COMPONENT, TEST_COMPONENT, &Buffer, Handlers);
}

static void AssertNoHandler(void)
{
    UtAssert_UINT32_EQ(HandlerCalls[0], 0);
    UtAssert_UINT32_EQ(HandlerCalls[1], 0);
}

static void TestMatchingDerivatives(void)
{
    IdentifiedType.DerivativeTableIndex = 0;
    UtAssert_INT32_EQ(Dispatch(), 10);
    UtAssert_UINT32_EQ(LookedUpType, TEST_DERIVED_TYPE);
    UtAssert_UINT32_EQ(HandlerCalls[0], 1);
    UtAssert_UINT32_EQ(HandlerCalls[1], 0);
    Setup();
    UtAssert_INT32_EQ(Dispatch(), 11);
    UtAssert_UINT32_EQ(LookedUpType, TEST_DERIVED_TYPE);
    UtAssert_UINT32_EQ(HandlerCalls[0], 0);
    UtAssert_UINT32_EQ(HandlerCalls[1], 1);
}

static void TestUnmatchedDerivative(void)
{
    UT_SetDefaultReturnValue(UT_KEY(EdsLib_DataTypeDB_IdentifyBufferWithSize), EDSLIB_NO_MATCHING_VALUE);
    UtAssert_INT32_EQ(Dispatch(), CFE_STATUS_VALIDATION_FAILURE);
    AssertNoHandler();
    UtAssert_STUB_COUNT(EdsLib_DataTypeDB_GetTypeInfo, 0);
}

static void TestIdentificationError(void)
{
    UT_SetDefaultReturnValue(UT_KEY(EdsLib_DataTypeDB_IdentifyBufferWithSize), EDSLIB_FAILURE);
    UtAssert_INT32_EQ(Dispatch(), CFE_STATUS_VALIDATION_FAILURE);
    AssertNoHandler();
}

static void TestNonDerivedType(void)
{
    DerivedInfo.NumDerivatives = 0;
    UT_SetDefaultReturnValue(UT_KEY(EdsLib_DataTypeDB_IdentifyBufferWithSize), EDSLIB_NO_MATCHING_VALUE);
    UtAssert_INT32_EQ(Dispatch(), 10);
    UtAssert_UINT32_EQ(LookedUpType, TEST_BASE_TYPE);
    UtAssert_STUB_COUNT(EdsLib_DataTypeDB_IdentifyBufferWithSize, 0);
    UtAssert_UINT32_EQ(HandlerCalls[0], 1);
    UtAssert_UINT32_EQ(HandlerCalls[1], 0);
}

static void TestDerivedInfoError(void)
{
    UT_SetDefaultReturnValue(UT_KEY(EdsLib_DataTypeDB_GetDerivedInfo), EDSLIB_FAILURE);
    UtAssert_INT32_EQ(Dispatch(), CFE_SB_INTERNAL_ERR);
    AssertNoHandler();
    UtAssert_STUB_COUNT(EdsLib_DataTypeDB_IdentifyBufferWithSize, 0);
}

static void TestTypeInfoError(void)
{
    UT_SetDefaultReturnValue(UT_KEY(EdsLib_DataTypeDB_GetTypeInfo), EDSLIB_FAILURE);
    UtAssert_INT32_EQ(Dispatch(), CFE_SB_INTERNAL_ERR);
    AssertNoHandler();
}

static void TestMessageSizeError(void)
{
    UT_SetDefaultReturnValue(UT_KEY(CFE_MSG_GetSize), CFE_STATUS_WRONG_MSG_LENGTH);
    UtAssert_INT32_EQ(Dispatch(), CFE_STATUS_WRONG_MSG_LENGTH);
    AssertNoHandler();
    UtAssert_STUB_COUNT(EdsLib_DataTypeDB_GetDerivedInfo, 0);
    UtAssert_STUB_COUNT(EdsLib_DataTypeDB_IdentifyBufferWithSize, 0);
}

static void TestWrongMessageLength(void)
{
    ++TypeInfo.Size.Bytes;
    UtAssert_INT32_EQ(Dispatch(), CFE_STATUS_WRONG_MSG_LENGTH);
    AssertNoHandler();
    Setup();
    DerivedInfo.NumDerivatives = 0;
    --TypeInfo.Size.Bytes;
    UtAssert_INT32_EQ(Dispatch(), CFE_STATUS_WRONG_MSG_LENGTH);
    AssertNoHandler();
}

void UtTest_Setup(void)
{
    UtTest_Add(TestMatchingDerivatives, Setup, NULL, "Matching derivatives keep their handlers");
    UtTest_Add(TestUnmatchedDerivative, Setup, NULL, "Unknown derivative never invokes handler zero");
    UtTest_Add(TestIdentificationError, Setup, NULL, "Identification errors do not fall back");
    UtTest_Add(TestNonDerivedType, Setup, NULL, "Non-derived type retains handler zero");
    UtTest_Add(TestDerivedInfoError, Setup, NULL, "Database metadata errors are rejected");
    UtTest_Add(TestTypeInfoError, Setup, NULL, "Type information errors are rejected");
    UtTest_Add(TestMessageSizeError, Setup, NULL, "Unreadable message size is rejected");
    UtTest_Add(TestWrongMessageLength, Setup, NULL, "Derived and base size checks remain enforced");
}
