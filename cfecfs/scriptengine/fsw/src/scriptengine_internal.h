/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * @file
 *  An example of an internal (private) header file for SAMPLE Lib
 */
#ifndef SCRIPTENGINE_INTERNAL_H
#define SCRIPTENGINE_INTERNAL_H

/* Include all external/public definitions */
#include "scriptengine.h"

#include <lua.h>


/*************************************************************************
** Macro Definitions
*************************************************************************/

typedef struct
{
    lua_State *lua;
} SCRIPTENGINE_Global_t;

/*************************************************************************
** Internal Data Structures
*************************************************************************/
extern SCRIPTENGINE_Global_t SCRIPTENGINE_Global;

/*************************************************************************
** Function Declarations
*************************************************************************/

#endif