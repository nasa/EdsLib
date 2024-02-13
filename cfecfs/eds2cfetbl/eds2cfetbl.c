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
 * \file     eds2cfetbl.c
 * \ingroup  cfecfs
 * \author   joseph.p.hickey@nasa.gov
 *
 * Build-time utility to write table files from C source files using edslib
 * Replacement for the old "elf2cfetbl" utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <expat.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "scripts/eds_tbltool_filedef.h"

#include "edslib_init.h"

/* compatibility shim to support compilation with Lua5.1 */
#include "edslib_lua51_compatibility.h"

#include "cfe_tbl_filedef.h"
#include "cfe_mission_eds_parameters.h"
#include "cfe_mission_eds_interface_parameters.h"
#include "cfe_tbl_eds_datatypes.h"
#include "cfe_fs_eds_datatypes.h"
#include "cfe_sb_eds_datatypes.h"
#include "edslib_lua_objects.h"
#include "cfe_missionlib_lua_softwarebus.h"
#include "cfe_missionlib_runtime.h"


const char LUA_APPLIST_KEY = 0;

typedef struct EdsTableTool_Global
{
    uint16_t ProcessorId;
    uint16_t NumMultiple;

    char EdsTypeName[64];

} EdsTableTool_Global_t;

EdsTableTool_Global_t EdsTableTool_Global;


/*----------------------------------------------------------------
 *
 * Map CMD topic ID to MsgID
 * This calls the same impl that CFE FSW uses
 *
 *-----------------------------------------------------------------*/
EdsDataType_CFE_SB_MsgIdValue_t CFE_SB_CmdTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum)
{
    const EdsComponent_CFE_SB_Listener_t  Params = {{ .InstanceNumber = InstanceNum, .TopicId = TopicId }};
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t Output;

    CFE_MissionLib_MapListenerComponent(&Output, &Params);

    return Output.MsgId.Value;
}

/*----------------------------------------------------------------
 *
 * Map TLM topic ID to MsgID
 * This calls the same impl that CFE FSW uses
 *
 *-----------------------------------------------------------------*/
EdsDataType_CFE_SB_MsgIdValue_t CFE_SB_TlmTopicIdToMsgId(uint16_t TopicId, uint16_t InstanceNum)
{
    const EdsComponent_CFE_SB_Publisher_t  Params = {{ .InstanceNumber = InstanceNum, .TopicId = TopicId }};
    EdsInterface_CFE_SB_SoftwareBus_PubSub_t Output;

    CFE_MissionLib_MapPublisherComponent(&Output, &Params);

    return Output.MsgId.Value;
}

/*----------------------------------------------------------------
 *
 * Return the current processor ID
 * This reflects the CPU for which the table is getting built
 *
 *-----------------------------------------------------------------*/
uint16_t EdsTableTool_GetProcessorId(void)
{
    return EdsTableTool_Global.ProcessorId;
}

/*----------------------------------------------------------------
 *
 * Exports a C-defined object to the global instance
 *
 * This expects a lua_State object as the abstract argument,
 * and a table should be the topmost entry in the stack.  The
 * object(s) from C will be wrapped in a Lua object and appended
 * to the table.
 *
 *-----------------------------------------------------------------*/
void EdsTableTool_DoExport(void *arg, const void *filedefptr, const void *objptr, size_t objsize)
{
    lua_State *lua = arg;
    const uint8_t *sptr = objptr;
    void *tptr;
    size_t tsz;
    int tbl_pos;

    /* There should be a table at the top of the initial stack */
    tbl_pos = lua_gettop(lua);

    lua_getglobal(lua, "EdsDB");

    while (objsize > 0)
    {
        /* Call "New_Object" for the object type */
        lua_getfield(lua, -1, "NewObject");
        lua_pushstring(lua, EdsTableTool_Global.EdsTypeName);
        if (lua_pcall(lua, 1, 1, 1) != LUA_OK)
        {
            /* note - error handler already displayed error message */
            fprintf(stderr, "Failed to execute: EdsDB.NewObject(%s)\n", EdsTableTool_Global.EdsTypeName);
            exit(EXIT_FAILURE);
        }

        /* Fill the Lua object with the data from the C world */
        EdsLib_LuaBinding_GetNativeObject(lua, -1, &tptr, &tsz);
        memcpy(tptr, sptr, tsz);
        sptr += tsz;
        objsize -= tsz;

        /* Append to the table at the top of the initial stack */
        lua_rawseti(lua, tbl_pos, 1 + lua_rawlen(lua, tbl_pos));
    }

    /* Reset the stack to the same point as it was initially */
    lua_settop(lua, tbl_pos);
}

void LoadTemplateFile(lua_State *lua, const char *Filename)
{
    void *dlhandle;
    void *fptr;
    const char *tmpstr;
    int obj_idx;
    size_t RuntimeAppNameLen;
    CFE_TBL_FileDef_t *CFE_TBL_FileDefPtr;
    EdsDataType_BASE_TYPES_ApiName_t RuntimeAppName;
    EdsDataType_BASE_TYPES_ApiName_t TableName;
    const char *EdsAppName;
    size_t EdsAppNameLen;
    char EdsTypeName[64];
    char ModuleTempName[64];
    void (*DynamicGeneratorFuncPtr)(void *);
    EdsLib_Id_t EdsId;
    EdsLib_DataTypeDB_TypeInfo_t TypeInfo;
    uint16_t AppIdx;

    dlerror();
    dlhandle = dlopen(Filename, RTLD_LAZY | RTLD_LOCAL);
    tmpstr = dlerror();
    if (tmpstr != NULL || dlhandle == NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Unknown Error]";
        }
        fprintf(stderr,"Load Error: %s\n", tmpstr);
        exit(EXIT_FAILURE);
    }

    CFE_TBL_FileDefPtr = (CFE_TBL_FileDef_t *)dlsym(dlhandle, "CFE_TBL_FileDef");
    tmpstr = dlerror();
    if (CFE_TBL_FileDefPtr == NULL || tmpstr != NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Table object is NULL]";
        }
        fprintf(stderr,"Error looking up CFE_TBL_FileDef: %s\n", tmpstr);
        exit(EXIT_FAILURE);
    }

    snprintf(ModuleTempName, sizeof(ModuleTempName), "%s_EDS_TYPEDEF_NAME", CFE_TBL_FileDefPtr->ObjectName);
    fptr = dlsym(dlhandle, ModuleTempName);
    tmpstr = dlerror();
    if (fptr == NULL || tmpstr != NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Object pointer is NULL]";
        }
        fprintf(stderr,"Lookup Error on '%s': %s\n", ModuleTempName, tmpstr);
        exit(EXIT_FAILURE);
    }

    strncpy(EdsTypeName, fptr, sizeof(EdsTypeName) - 1);
    EdsTypeName[sizeof(EdsTypeName) - 1] = 0;

    /* If the table defines a "dynamic content" function, this needs to be executed first */
    snprintf(ModuleTempName, sizeof(ModuleTempName), "%s_GENERATOR", CFE_TBL_FileDefPtr->ObjectName);
    fptr = dlsym(dlhandle, ModuleTempName);
    tmpstr = dlerror();
    if (fptr == NULL || tmpstr != NULL)
    {
        if (tmpstr == NULL)
        {
            tmpstr = "[Object pointer is NULL]";
        }
        fprintf(stderr,"Lookup Error on '%s': %s\n", ModuleTempName, tmpstr);
        exit(EXIT_FAILURE);
    }

    *((void **)&DynamicGeneratorFuncPtr) = fptr;

    /*
     * The TableName in the FileDef object is supposed to be of the form "AppName.TableName" -
     * first split this into its constituent parts.
     *
     * Note, however, that the "AppName." part refers to the runtime app name, which often has
     * a suffix of some sort (e.g. _APP).  It should be related to the EDS app name though,
     * if naming conventions are followed.  (and if conventions are not followed, this tool
     * is not likely to work).
     */
    tmpstr = strchr(CFE_TBL_FileDefPtr->TableName, '.');
    if (tmpstr == NULL)
    {
        RuntimeAppNameLen = 0;
        tmpstr = CFE_TBL_FileDefPtr->TableName;
    }
    else
    {
        RuntimeAppNameLen = tmpstr - CFE_TBL_FileDefPtr->TableName;
        ++tmpstr;
    }

    if (RuntimeAppNameLen >= sizeof(RuntimeAppName))
    {
        RuntimeAppNameLen = sizeof(RuntimeAppName) - 1;
    }

    memcpy(RuntimeAppName, CFE_TBL_FileDefPtr->TableName, RuntimeAppNameLen);
    RuntimeAppName[RuntimeAppNameLen] = 0;
    printf("Runtime App Name: %s\n", RuntimeAppName);

    /*
     * It is common practice to have an "_APP" suffix to the runtime app name here,
     * so this is done as a prefix match.
     */
    AppIdx = EDS_MAX_DATASHEETS;
    while (true)
    {
        --AppIdx;
        if (AppIdx == 0)
        {
            /*
             * If the naming convention was not followed and no EDS entry is found for this app name,
             * then this tool is unlikely work EXCEPT if the user has overloaded the "Description"
             * field to refer to a specific EDS DB entry, so this will proceed if that is the case.
             */
            EdsAppName    = NULL;
            EdsAppNameLen = 0;
            printf("ERROR: No matching EDS package name for table: \'%s\'\n", CFE_TBL_FileDefPtr->TableName);
            break;
        }

        /* Note that this API does not return NULL, it returns "UNDEF" if the AppIdx is not registered,
         * but that should not happen with a static DB and an AppIdx within range */
        EdsId = EDSLIB_MAKE_ID(AppIdx, 1);
        EdsAppName    = EdsLib_DisplayDB_GetNamespace(&EDS_DATABASE, EdsId);
        EdsAppNameLen = strlen(EdsAppName);
        if (strncasecmp(EdsAppName, CFE_TBL_FileDefPtr->TableName, EdsAppNameLen) == 0)
        {
            if (!isalpha((unsigned char)CFE_TBL_FileDefPtr->TableName[EdsAppNameLen]))
            {
                printf("Matched EDS Package Name: %s\n", EdsAppName);
                break;
            }
        }
    }

    if (EdsAppName == NULL)
    {
        fprintf(stderr,"Aborting table build, unidentified application: %s\n", CFE_TBL_FileDefPtr->TableName);
        exit(EXIT_FAILURE);
    }

    /*
     * Sometimes the app name is repeated in the table name, so skip it again if so
     */
    if (strncmp(RuntimeAppName, tmpstr, RuntimeAppNameLen) == 0)
    {
        tmpstr += RuntimeAppNameLen;
    }
    else if (strncmp(EdsAppName, tmpstr, EdsAppNameLen) == 0)
    {
        tmpstr += EdsAppNameLen;
    }

    /* Skip punctuation/name separators */
    while (ispunct((unsigned char) *tmpstr))
    {
        ++tmpstr;
    }

    strncpy(TableName, tmpstr, sizeof(TableName) - 1);
    TableName[sizeof(TableName) - 1] = 0;
    printf("Runtime Table Name: %s\n", TableName);

    /* Now find an EDS type that might match */
    EdsId = EDSLIB_ID_INVALID;

    /*
     * Plan A:
     * The makefile scripts/tool should have extracted the type name
     */
    if (strncmp(EdsTypeName, EdsAppName, EdsAppNameLen) == 0)
    {
        EdsTypeName[EdsAppNameLen] = '/';
    }

    if (strlen(EdsTypeName) > 2 && strcmp(&EdsTypeName[strlen(EdsTypeName) - 2], "_t") == 0)
    {
        EdsTypeName[strlen(EdsTypeName) - 2] = 0;
    }

    printf("Attempting to lookup: %s\n", EdsTypeName);
    EdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, EdsTypeName);

    /*
     * Plan B:
     * If the specific EDS type was indicated in the description field, then use it.
     */
    if (!EdsLib_Is_Valid(EdsId) && strchr(CFE_TBL_FileDefPtr->Description, '/') != NULL)
    {
        strncpy(EdsTypeName, CFE_TBL_FileDefPtr->Description, sizeof(EdsTypeName));
        EdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, EdsTypeName);
    }

    /*
     * Plan C:
     * Attempt to deduce it from the AppName.TableName string
     */
    if (!EdsLib_Is_Valid(EdsId) && EdsAppName != NULL)
    {
        snprintf(EdsTypeName, sizeof(EdsTypeName), "%s/%s", EdsAppName, TableName);
        EdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, EdsTypeName);
    }

    /* as a last-ditch attempt, see if the ObjectName might be in the form of EDSAPP_EDSTYPE */
    if (!EdsLib_Is_Valid(EdsId) && EdsAppName != NULL)
    {
        tmpstr = CFE_TBL_FileDefPtr->ObjectName;
        if (strncmp(RuntimeAppName, tmpstr, RuntimeAppNameLen) == 0 && ispunct((unsigned char)tmpstr[RuntimeAppNameLen]))
        {
            tmpstr += RuntimeAppNameLen + 1;
        }
        else if (strncmp(EdsAppName, tmpstr, EdsAppNameLen) == 0 && ispunct((unsigned char)tmpstr[EdsAppNameLen]))
        {
            tmpstr += EdsAppNameLen + 1;
        }
        snprintf(EdsTypeName, sizeof(EdsTypeName), "%s/%s", EdsAppName, tmpstr);
        EdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, EdsTypeName);
    }

    if (!EdsLib_Is_Valid(EdsId) || EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, EdsId, &TypeInfo) != EDSLIB_SUCCESS)
    {
        fprintf(stderr,"Error: Cannot get info for table EDS Type Name \'%s\' (id %x)\n", EdsTypeName, (unsigned int)EdsId);
        exit(EXIT_FAILURE);
    }

    printf("--> Using EDS ID 0x%x for \'%s\' data structure, %lu bytes\n",(unsigned int)EdsId,EdsTypeName,(unsigned long)TypeInfo.Size.Bytes);

    if ((CFE_TBL_FileDefPtr->ObjectSize % TypeInfo.Size.Bytes) != 0)
    {
        fprintf(stderr,"Error: Data size mismatch for EDS Type \'%s\' (id %x): %lu(file)/%lu(EDS)\n",
            EdsTypeName, (unsigned int)EdsId, (unsigned long)CFE_TBL_FileDefPtr->ObjectSize, (unsigned long)TypeInfo.Size.Bytes);
        exit(EXIT_FAILURE);
    }

    EdsTableTool_Global.NumMultiple = CFE_TBL_FileDefPtr->ObjectSize / TypeInfo.Size.Bytes;
    strncpy(EdsTableTool_Global.EdsTypeName, EdsTypeName, sizeof(EdsTableTool_Global.EdsTypeName) - 1);

    lua_getglobal(lua, RuntimeAppName);
    if (lua_type(lua, -1) != LUA_TTABLE)
    {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_setglobal(lua, RuntimeAppName);
    }

    lua_newtable(lua);
    obj_idx = lua_gettop(lua);

    lua_pushstring(lua, CFE_TBL_FileDefPtr->TableName);
    lua_setfield(lua, obj_idx, "TableName");  /* Set "TableName" in Table object */

    lua_pushstring(lua, CFE_TBL_FileDefPtr->Description);
    lua_setfield(lua, obj_idx, "Description");  /* Set "Description" in Table object */

    lua_pushstring(lua, CFE_TBL_FileDefPtr->TgtFilename);
    lua_setfield(lua, obj_idx, "TgtFilename");  /* Set "TgtFilename" in Table object */

    printf("Loaded Template: %s/%s\n", RuntimeAppName, TableName);

    /*
     * If a dynamic generator function was defined, then it must be called now before dlclose().
     * This populates the object in memory (can still be post-processed in Lua too)
     */
    if (DynamicGeneratorFuncPtr != NULL)
    {
        /* The C object could be an array of entries, so make a table to hold them */
        lua_newtable(lua);

        DynamicGeneratorFuncPtr(lua);

        printf("--> Got %zu object(s) from C domain\n", lua_rawlen(lua, -1));

        /*
         * If only one object was obtained, then just put that object directly in the parent,
         * there is no need for the table wrapper.  (this is the normal/typical case when the
         * table is fully defined in EDS and C uses an EDS type).
         */
        if (lua_rawlen(lua, -1) == 1)
        {
            lua_rawgeti(lua, -1, 1);
        }
        lua_setfield(lua, obj_idx, "Content");  /* Set "Content" in Table object */
        lua_settop(lua, obj_idx);
    }

    dlclose(dlhandle);

    /* Append the Applist table to hold the loaded table template objects */
    lua_rawgetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);
    lua_pushvalue(lua, obj_idx);
    lua_rawseti(lua, -2, 1 + lua_rawlen(lua, -2));

    lua_setfield(lua, -2, TableName);  /* Set entry in App table */
    lua_pop(lua, 1);
}

void LoadLuaFile(lua_State *lua, const char *Filename)
{
    if (luaL_loadfile(lua, Filename) != LUA_OK)
    {
        fprintf(stderr,"Cannot load Lua file: %s: %s\n", Filename, lua_tostring(lua, -1));
        exit(EXIT_FAILURE);
    }
    printf("Executing LUA: %s\n", Filename);
    if (lua_pcall(lua, 0, 0, 1) != LUA_OK)
    {
        /* note - error handler already displayed error message */
        fprintf(stderr, "Failed to execute: %s\n", Filename);
        exit(EXIT_FAILURE);
    }
}

void PushEncodedSingleObject(lua_State *lua)
{
    luaL_checkudata(lua, -1, "EdsLib_Object");
    lua_getglobal(lua, "EdsDB");
    lua_getfield(lua, -1, "Encode");
    lua_remove(lua, -2);     /* remove the EdsDB global */
    lua_pushvalue(lua, -2);
    lua_call(lua, 1, 1);
}

void PushEncodedMultiObject(lua_State *lua)
{
    size_t num_tbl;
    size_t num_enc;
    int obj_idx;

    obj_idx = lua_gettop(lua);

    luaL_checktype(lua, -1, LUA_TTABLE);

    num_tbl = lua_rawlen(lua, obj_idx);

    num_enc = 0;
    while(num_enc < num_tbl)
    {
        ++num_enc;
        lua_rawgeti(lua, obj_idx, num_enc); /* Get the EdsLib_Object table entry */
        PushEncodedSingleObject(lua);
        lua_remove(lua, -2);     /* remove the EdsLib_Object table entry from stack */
    }

    lua_concat(lua, num_enc);
}

int Write_GenericFile(lua_State *lua)
{
    FILE *OutputFile = NULL;
    const char *OutputName = luaL_checkstring(lua, 1);

    OutputFile = fopen(OutputName, "w");
    if (OutputFile == NULL)
    {
        return luaL_error(lua, "%s: %s", OutputName, strerror(errno));
    }

    PushEncodedSingleObject(lua);
    fwrite(lua_tostring(lua, -1), lua_rawlen(lua, -1), 1, OutputFile);
    fclose(OutputFile);
    lua_pop(lua, 1);

    return 0;
}

int Write_CFE_EnacapsulationFile(lua_State *lua)
{
    union
    {
        EdsDataType_CFE_FS_Header_t FileHeader;
        EdsDataType_CFE_TBL_File_Hdr_t TblHeader;
    } Buffer;
    EdsPackedBuffer_CFE_FS_Header_t PackedFileHeader;
    EdsPackedBuffer_CFE_TBL_File_Hdr_t PackedTblHeader;
    uint32_t TblHeaderBlockSize;
    uint32_t FileHeaderBlockSize;
    EdsLib_Id_t PackedEdsId;
    FILE *OutputFile = NULL;
    const char *OutputName = luaL_checkstring(lua, 1);
    EdsLib_DataTypeDB_TypeInfo_t BlockInfo;
    const void *content_ptr;
    size_t content_sz;

    PackedEdsId = EdsLib_DisplayDB_LookupTypeName(&EDS_DATABASE, EdsTableTool_Global.EdsTypeName);
    printf("Generating file using EDS ID: %x\n", (unsigned int)PackedEdsId);

    if (lua_type(lua, -1) == LUA_TTABLE)
    {
        PushEncodedMultiObject(lua);
    }
    else
    {
        PushEncodedSingleObject(lua);
    }

    if (lua_isnoneornil(lua, -1))
    {
        return luaL_error(lua, "%s: Failed to encode object", OutputName);
    }

    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.TblHeader.EdsAppId = EdsLib_Get_AppIdx(PackedEdsId);
    Buffer.TblHeader.EdsFormatId = EdsLib_Get_FormatIdx(PackedEdsId);
    Buffer.TblHeader.NumBytes = lua_rawlen(lua, -1);
    strncpy(Buffer.TblHeader.TableName, luaL_checkstring(lua, 3), sizeof(Buffer.TblHeader.TableName));

    PackedEdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_TBL), CFE_TBL_File_Hdr_DATADICTIONARY);
    EdsLib_DataTypeDB_PackCompleteObject(&EDS_DATABASE, &PackedEdsId,
            PackedTblHeader, &Buffer.TblHeader, sizeof(PackedTblHeader) * 8, sizeof(Buffer.TblHeader));
    EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, PackedEdsId, &BlockInfo);
    TblHeaderBlockSize = (BlockInfo.Size.Bits + 7) / 8;

    memset(&Buffer, 0, sizeof(Buffer));
    Buffer.FileHeader.ContentType = CFE_FS_FILE_CONTENT_ID;
    Buffer.FileHeader.SubType = luaL_checkinteger(lua, lua_upvalueindex(1));
    Buffer.FileHeader.Length = TblHeaderBlockSize + lua_rawlen(lua, -1);
    snprintf(Buffer.FileHeader.Description, sizeof(Buffer.FileHeader.Description), "%s", luaL_checkstring(lua, 2));

    PackedEdsId = EDSLIB_MAKE_ID(EDS_INDEX(CFE_FS), CFE_FS_Header_DATADICTIONARY);
    EdsLib_DataTypeDB_PackCompleteObject(&EDS_DATABASE, &PackedEdsId,
            PackedFileHeader, &Buffer, sizeof(PackedFileHeader) * 8, sizeof(Buffer));
    EdsLib_DataTypeDB_GetTypeInfo(&EDS_DATABASE, PackedEdsId, &BlockInfo);
    FileHeaderBlockSize = (BlockInfo.Size.Bits + 7) / 8;

    OutputFile = fopen(OutputName, "w");
    if (OutputFile == NULL)
    {
        return luaL_error(lua, "%s: %s", OutputName, strerror(errno));
    }

    fwrite(PackedFileHeader, FileHeaderBlockSize, 1, OutputFile);
    fwrite(PackedTblHeader, TblHeaderBlockSize, 1, OutputFile);
    content_ptr = lua_tostring(lua, -1);
    content_sz = lua_rawlen(lua, -1);
    if (content_ptr != NULL && content_sz > 0)
    {
        fwrite(content_ptr, content_sz, 1, OutputFile);
    }
    else
    {
        fprintf(stderr, "WARNING: No content produced\n");
    }
    fclose(OutputFile);
    printf("Wrote File: %s\n", OutputName);
    lua_pop(lua, 1);

    return 0;
}

static int ErrorHandler(lua_State *lua)
{
    lua_getglobal(lua, "debug");
    lua_getfield(lua, -1, "traceback");
    lua_remove(lua, -2);
    lua_pushnil(lua);
    lua_pushinteger(lua, 3);
    lua_call(lua, 2, 1);
    fprintf(stderr, "Error: %s\n%s\n", lua_tostring(lua, 1), lua_tostring(lua, 2));
    return 0;
}

int main(int argc, char *argv[])
{
    int arg;
    size_t slen;
    lua_State *lua;

    EdsLib_Initialize();
    memset(&EdsTableTool_Global, 0, sizeof(EdsTableTool_Global));

    /* Create a Lua state and register all of our local test functions */
    lua = luaL_newstate();
    luaL_openlibs(lua);

    /* Stack index 1 will be the error handler function (used for protected calls) */
    lua_pushcfunction(lua, ErrorHandler);

    /* Create an Applist table to hold the loaded table template objects */
    lua_newtable(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);

    while ((arg = getopt (argc, argv, "e:")) != -1)
    {
       switch (arg)
       {
       case 'e':
          if (luaL_loadstring(lua, optarg) != LUA_OK)
          {
              fprintf(stderr,"ERROR: Invalid command line expression: %s\n",
                      luaL_tolstring(lua,-1, NULL));
              lua_pop(lua, 2);
              return EXIT_FAILURE;
          }

          if (lua_pcall(lua, 0, 0, 1) != LUA_OK)
          {
              fprintf(stderr,"ERROR: Cannot evaluate command line expression: %s\n",
                      luaL_tolstring(lua,-1, NULL));
              lua_pop(lua, 2);
              return EXIT_FAILURE;
          }
          break;

       case '?':
          if (isprint (optopt))
          {
             fprintf(stderr, "Unknown option `-%c'.\n", optopt);
          }
          else
          {
             fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
          }
          return EXIT_FAILURE;
          break;

       default:
          return EXIT_FAILURE;
          break;
       }
    }

    /* All additional args are input files.  If none, this should be considered an error */
    if (optind >= argc)
    {
        fprintf(stderr, "ERROR: No input files specified on command line.\n");
        return EXIT_FAILURE;
    }

    /* The CPUNAME and CPUNUMBER variables should both be defined.
     * If only one was provided then look up the other value in EDS */
    lua_getglobal(lua, "CPUNAME");
    lua_getglobal(lua, "CPUNUMBER");
    if (lua_isstring(lua, -2) && lua_isnil(lua, -1))
    {
        lua_pushinteger(lua, CFE_MissionLib_GetInstanceNumber(&CFE_SOFTWAREBUS_INTERFACE,
                lua_tostring(lua, -2)));
        lua_setglobal(lua, "CPUNUMBER");
    }
    else if (lua_isnumber(lua, -1) && lua_isnil(lua, -2))
    {
        char TempBuf[64];
        lua_pushstring(lua, CFE_MissionLib_GetInstanceName(&CFE_SOFTWAREBUS_INTERFACE,
                lua_tointeger(lua, -1), TempBuf, sizeof(TempBuf)));
        lua_setglobal(lua, "CPUNAME");
    }
    lua_pop(lua, 2);

    /*
     * if the SIMULATION compile-time directive is set, create a global
     * Lua variable with that name so Lua code can do "if SIMULATION" statements
     * as needed to support simulated environments.
     *
     * Note that a double macro is needed here, to force expansion of the macro
     * rather than stringifying the macro name itself i.e. "SIMULATION"
     */
#ifdef SIMULATION
#define EDS2CFETBL_STRINGIFY(x)     #x
#define EDS2CFETBL_SIMSTRING(x)     EDS2CFETBL_STRINGIFY(x)
    lua_pushstring(lua, EDS2CFETBL_SIMSTRING(SIMULATION));
    lua_setglobal(lua, "SIMULATION");
#endif

    lua_pushinteger(lua, CFE_FS_SubType_TBL_IMG);
    lua_pushcclosure(lua, Write_CFE_EnacapsulationFile, 1);
    lua_setglobal(lua, "Write_CFE_LoadFile");

    lua_pushcfunction(lua, Write_GenericFile);
    lua_setglobal(lua, "Write_GenericFile");

    EdsLib_Lua_Attach(lua, &EDS_DATABASE);
    CFE_MissionLib_Lua_SoftwareBus_Attach(lua, &CFE_SOFTWAREBUS_INTERFACE);
    lua_setglobal(lua, "EdsDB");

    lua_getglobal(lua, "CPUNUMBER");
    EdsTableTool_Global.ProcessorId = lua_tointeger(lua, -1);
    lua_pop(lua, 1);

    for (arg = optind; arg < argc; arg++)
    {
        /* assume the argument is a file to execute/load */
        slen = strlen(argv[arg]);
        if (slen >= 3 && strcmp(argv[arg] + slen - 3, ".so") == 0)
        {
            LoadTemplateFile(lua, argv[arg]);
        }
        else if (slen >= 4 && strcmp(argv[arg] + slen - 4, ".lua") == 0)
        {
            LoadLuaFile(lua, argv[arg]);
        }
        else
        {
            fprintf(stderr,"WARNING: Unable to handle file argument: %s\n",argv[arg]);
            return EXIT_FAILURE;
        }
    }

    lua_rawgetp(lua, LUA_REGISTRYINDEX, &LUA_APPLIST_KEY);
    lua_pushnil(lua);
    while (lua_next(lua, -2))
    {
        luaL_checktype(lua, -1, LUA_TTABLE);
        lua_getglobal(lua, "Write_CFE_LoadFile");
        assert(lua_type(lua, -1) == LUA_TFUNCTION);
        lua_pushstring(lua, "TgtFilename");
        lua_gettable(lua, -3);
        lua_pushstring(lua, "Description");
        lua_gettable(lua, -4);
        lua_pushstring(lua, "TableName");
        lua_gettable(lua, -5);
        lua_pushstring(lua, "Content");
        lua_gettable(lua, -6);
        assert(lua_type(lua, -5) == LUA_TFUNCTION);
        if (lua_pcall(lua, 4, 0, 1) != LUA_OK)
        {
            /* note - more descriptive error message already printed by error handler function */
            fprintf(stderr, "Failed to write CFE table file from template\n");
            return (EXIT_FAILURE);
        }

        lua_pop(lua, 1);
    }
    lua_pop(lua, 1); /* Applist Global */

    return EXIT_SUCCESS;
}
