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
 * \file     ut_edslib_stubs.c
 * \ingroup  fsw
 * \author   joseph.p.hickey@nasa.gov
 *
 * UT-Assert compatible stub functions for EdsLib
 */

/*
** Include section
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "edslib_id.h"
#include "edslib_api_types.h"
#include "edslib_datatypedb.h"
#include "edslib_displaydb.h"

#include "utstubs.h"

/*
 * A macro for a generic stub function that has no return code
 * (note this still invokes the UT call, so that user-specified hooks will work)
 */
#define EDSLIB_VOID_STUB(Func,Args)       \
    void Func Args { UT_DefaultStubImpl(#Func, UT_KEY(Func), 0); }

/*
 * A macro for a generic stub function that has no outputs aside from return code
 */
#define EDSLIB_SIMPLE_STUB(Func,Args)       \
    int32_t Func Args { return EdsLib_UT_GenericStub(#Func, UT_KEY(Func), NULL, 0); }

/*
 * A macro for a generic stub function that has an output parameter which is a
 * pointer to fixed-type blob (common paradigm in EdsLib lookup functions)
 */
#define EDSLIB_OUTPUT_STUB(Func,Obj,Args)   \
    int32_t Func Args { return EdsLib_UT_GenericStub(#Func, UT_KEY(Func), Obj, sizeof(*Obj)); }

/*
 * A macro for a generic stub that has an output parameter which is a user-supplied
 * buffer of user-specified size (common paradigm for pack/unpack functions)
 */
#define EDSLIB_USERBUFFER_STUB(Func,Buf,BufSz,Args)    \
    int32_t Func Args { return EdsLib_UT_GenericStub(#Func, UT_KEY(Func), Buf, BufSz); }

static int32_t EdsLib_UT_GenericStub(const char *FuncName, UT_EntryKey_t FuncKey, void *OutputObj, uint32_t OutputSize)
{
    int32_t Result;
    uint32_t CopySize;

    Result = UT_DefaultStubImpl(FuncName, FuncKey, EDSLIB_SUCCESS);

    if (OutputSize > 0)
    {
        if (Result < EDSLIB_SUCCESS)
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
                /*
                 * Fill out the _remainder_ of the output object with a pattern.
                 * Same logic as above, to catch potential issues with user code that
                 * are potentially looking at data beyond what was really filled
                 */
                memset((uint8_t*)OutputObj + CopySize, 0x5A, OutputSize - CopySize);
            }
        }
    }

    return Result;
}


EDSLIB_VOID_STUB(EdsLib_DataTypeDB_Initialize,
        (void)
)

uint16_t EdsLib_DataTypeDB_GetAppIdx(const EdsLib_DataTypeDB_t AppDict)
{
    return UT_DEFAULT_IMPL(EdsLib_DataTypeDB_GetAppIdx);
}

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_Register,
        (EdsLib_DatabaseObject_t *GD, EdsLib_DataTypeDB_t AppDict)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_Unregister,
        (EdsLib_DatabaseObject_t *GD, uint16_t AppIdx)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetTypeInfo,TypeInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_TypeInfo_t *TypeInfo)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetDerivedTypeById,DerivedEdsId,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t DerivId, EdsLib_Id_t *DerivedEdsId)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetDerivedInfo,DerivInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_DataTypeDB_DerivedTypeInfo_t *DerivInfo)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetMemberByIndex, MemberInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t SubIndex, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetMemberByNativeOffset, MemberInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint32_t ByteOffset, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_GetConstraintEntity, MemberInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, uint16_t ConstraintIdx, EdsLib_DataTypeDB_EntityInfo_t *MemberInfo)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_ConstraintIterator,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId, EdsLib_ConstraintCallback_t Callback, void *CbArg)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_BaseCheck,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t BaseId, EdsLib_Id_t DerivedId)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_FinalizePackedObject,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *PackedData)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_VerifyUnpackedObject,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj, const void *PackedObj, uint32_t RecomputeFields)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_InitializeNativeObject,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *UnpackedObj)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_LoadValue, DestBuffer,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, EdsLib_GenericValueBuffer_t *DestBuffer, const void *SrcPtr)
)

EDSLIB_SIMPLE_STUB(EdsLib_DataTypeDB_StoreValue,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, void *DestPtr, EdsLib_GenericValueBuffer_t *SrcBuffer)
)

EDSLIB_OUTPUT_STUB(EdsLib_DataTypeDB_IdentifyBuffer, DerivObjInfo,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t EdsId, const void *MessageBuffer, EdsLib_DataTypeDB_DerivativeObjectInfo_t *DerivObjInfo)
)

EDSLIB_USERBUFFER_STUB(EdsLib_DataTypeDB_PackCompleteObject, DestBuffer, ((MaxPackedBitSize + 7) / 8),
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
                void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize)
)

EDSLIB_USERBUFFER_STUB(EdsLib_DataTypeDB_PackPartialObject, DestBuffer, ((MaxPackedBitSize + 7) / 8),
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
                void *DestBuffer, const void *SourceBuffer, uint32_t MaxPackedBitSize, uint32_t SourceByteSize, uint32_t StartingBit)
)

EDSLIB_USERBUFFER_STUB(EdsLib_DataTypeDB_UnpackCompleteObject,DestBuffer, MaxNativeByteSize,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
                void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize)
)

EDSLIB_USERBUFFER_STUB(EdsLib_DataTypeDB_UnpackPartialObject,DestBuffer, MaxNativeByteSize,
        (const EdsLib_DatabaseObject_t *GD, EdsLib_Id_t *EdsId,
                void *DestBuffer, const void *SourceBuffer, uint32_t MaxNativeByteSize, uint32_t SourceBitSize, uint32_t StartingByte)
)

