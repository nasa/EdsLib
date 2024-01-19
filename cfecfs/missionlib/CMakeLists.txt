#
# LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
# LEW-20211-1, Python Bindings for the Core Flight Executive Mission Library
#
# Copyright (c) 2020 United States Government as represented by
# the Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# CFE/CFS EDS Mission Library Build Script
# -----------------------------------------
#
# This is a component of the larger CFE/CFS Electronic Data Sheet (EDS) integration and
# it contains the runtime database objects.  It is intended to be a component within a
# larger mission or CPU build.  This Cmake script supports both configuration as part
# of the parent build via the add_subdirectory() command, or as an external build
# that references back to the CFE/CFS build.
#
# The objective of this project is to generate the "interface" database which specifies
# how the CFE software bus components interconnect.  Primarily this includes how message
# ID values (The MsgId parameter on CFE Software Bus interfaces) map back to the data
# types defined in EDS and vice versa.
#
# To do this, additional lookup tables are generated from the EDS information using the
# EDS toolchain.  The generators of these lookup tables are contained in the "scripts"
# subdirectory.  The contents of this directory should be included on the sedstool command
# line which is handled outside of this script.
#
cmake_minimum_required(VERSION 3.5)
project(EDS_CFECFS_MISSIONLIB C)


# When the EDS toolchain executes it produces a number of files based on the mission name
# By convention, the filenames will be all lowercase, and global symbols within those files
# will be all UPPERCASE.  The mission name is used to generate a unique prefix.
string(TOLOWER ${MISSION_NAME}_eds EDS_FILE_PREFIX)
string(TOUPPER ${MISSION_NAME} EDS_SYMBOL_PREFIX)
file(RELATIVE_PATH ARCH_BINARY_SUBDIR ${MISSION_BINARY_DIR} ${CMAKE_BINARY_DIR})
if ("${ARCH_BINARY_SUBDIR}" STREQUAL "")
    # Directory ref should not be empty, use local dir if they are the same
    set(ARCH_BINARY_SUBDIR .)
endif()

# Capture critical cache variables from the build environment so they can be
# used in custom makefiles that are invoked from here as custom targets
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/edstool-buildenv.d.in
    ${CMAKE_BINARY_DIR}/obj/edstool-buildenv.d
)

# The EDS toolchain generates several outputs, C headers
# as well as C libraries.  Scripts are included to not only
# generate the sources but compile those into .a / .so files as well,
# at least for the local machine.
#
# This is because the components that go into these libs are variable based
# off the EDS datasheets in use.  So it is best if the EDS tool handles the
# entire process here from generating to compiling and linking as well.
#
# However these generated libraries will need to be linked to executables
# built by these cmake scripts.  This is where the "IMPORTED" targets become
# useful, as we can tell cmake about these libraries that were built externally.
add_library(cfe_edsdb_static STATIC IMPORTED GLOBAL)
set_target_properties(cfe_edsdb_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_db${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

# Import Interface DB objects
# The process is similar to the other DB objects in that the set of generated files
# depends on the EDS datasheets in use, so as a result the scripts also generate a
# makefile fragment which has the appropriate list of files to build.  Back in this
# cmake world this is treated as an imported target so it can be referenced like any other.
add_library(cfe_missionlib_interfacedb_static STATIC IMPORTED GLOBAL)
set_target_properties(cfe_missionlib_interfacedb_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_interfacedb${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

# The missionlib runtime contains the code that converts TopicID (functional level identifier)
# to MsgId (access level identifier) and vice versa.  It is in a separate module so it can be
# more easily customized if necessary.
add_library(cfe_missionlib_runtime_static STATIC IMPORTED GLOBAL)
set_target_properties(cfe_missionlib_runtime_static PROPERTIES
    IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_missionlib_runtime${CMAKE_STATIC_LIBRARY_SUFFIX}"
)

# All of the imported libraries are products of the EDS tool
add_dependencies(cfe_edsdb_static edstool-execute)
add_dependencies(cfe_missionlib_interfacedb_static edstool-execute)
add_dependencies(cfe_missionlib_runtime_static edstool-execute)



# The same set of libraries, but a shared version
# These targets are only registered for machines that support shared libraries in some form
# Check if this target supports shared libraries
# Most CFE targets will, but small RTOS deployments may not
get_property(SUPPORTS_SHARED_LIBS GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
if (SUPPORTS_SHARED_LIBS)

    add_library(cfe_edsdb_shared SHARED IMPORTED GLOBAL)
    set_target_properties(cfe_edsdb_shared PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_db${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
    add_library(cfe_missionlib_interfacedb_shared STATIC IMPORTED GLOBAL)
    set_target_properties(cfe_missionlib_interfacedb_shared PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_interfacedb${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
    add_library(cfe_missionlib_runtime_shared SHARED IMPORTED GLOBAL)
    set_target_properties(cfe_missionlib_runtime_shared PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_missionlib_runtime${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

    add_dependencies(cfe_edsdb_shared edstool-execute)
    add_dependencies(cfe_missionlib_interfacedb_shared edstool-execute)
    add_dependencies(cfe_missionlib_runtime_shared edstool-execute)

    # Install the EDS missionlib library objects --
    # This is generally for host-side tools.
    # For CFE itself it is statically linked if necessary.
    if(IS_CFS_MISSION_BUILD)
        install(FILES
                "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_interfacedb${CMAKE_SHARED_LIBRARY_SUFFIX}"
                "${CMAKE_BINARY_DIR}/obj/${EDS_FILE_PREFIX}_db${CMAKE_SHARED_LIBRARY_SUFFIX}"
            DESTINATION
                lib)
    endif(IS_CFS_MISSION_BUILD)

endif (SUPPORTS_SHARED_LIBS)


add_subdirectory(fsw)
add_subdirectory(lua)
add_subdirectory(python)

# EDS aliases - due to the generic nature of EdsLib the
# target names do not exactly match CFE expectations.
add_library(edslib ALIAS edslib_minimal)
add_library(missionlib ALIAS cfe_missionlib)
