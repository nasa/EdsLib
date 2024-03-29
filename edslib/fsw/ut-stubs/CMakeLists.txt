#
# LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
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

#
# Create the generic stubs library
#
add_library(ut_edslib_stubs STATIC
  edslib_binding_objects_stubs.c
  edslib_datatypedb_stubs.c
  edslib_displaydb_stubs.c
  edslib_init_stubs.c
)

target_link_libraries(ut_edslib_stubs
  edslib_api
  ut_assert
)
