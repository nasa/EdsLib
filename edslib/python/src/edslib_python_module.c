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
 * \file     edslib_python_module.c
 * \ingroup  python
 * \author   joseph.p.hickey@nasa.gov
 *
**   A module that implements Python bindings for EDS objects
**
**   This file is just a thin wrapper that contains a Python-compatible init
**   function for the module.  This is expected to be used when the EdsLib
**   module is imported directly in a Python program.  No built-in database
**   object will be added.  The user is on their own to load a database
**   separately from the EdsLib module.
**
**   Note that other applications which embed Python would typically
**   _not_ use this entry point.  Instead, the EdsLib would be registered
**   as a built-in module using its own customized init, and typically
**   would add an actual database object to the module during that process.
 */

#include "edslib_python.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

/*
 * Init Function / Entry point
 *
 * A wrapper function is necessary to handle the
 * difference in module init between Python 2 (void) and
 * Python 3 (PyObject*).  The symbol also has a different
 * naming convention.  The Python2 wrapper just calls the
 * common init function and discards the return value.
 *
 * This entry point also should be declared using the
 * PyMODINIT_FUNC macro to ensure it is externally visible
 * and has the right calling convention.
 */
#if (PY_MAJOR_VERSION >= 3)

/*
 * Python 3+ init function variant:
 * Returns the new module object to the interpreter.
 * Python3 should always define PyMODINIT_FUNC
 */
PyMODINIT_FUNC PyInit_EdsLib(void)
{
    /* python3 init should return the module object */
    return EdsLib_Python_CreateModule();
}

#else

/*
 * In case the Python headers didn't supply PyMODINIT_FUNC,
 * (which is possible in older versions) then just use void.
 */
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif


PyMODINIT_FUNC initEdsLib(void)
{
    /* python2 does not want module object */
    EdsLib_Python_CreateModule();
}

#endif

