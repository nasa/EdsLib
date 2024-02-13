--
-- LEW-19710-1, CCSDS SOIS Electronic Data Sheet Implementation
--
-- Copyright (c) 2020 United States Government as represented by
-- the Administrator of the National Aeronautics and Space Administration.
-- All Rights Reserved.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--    http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--


-- -------------------------------------------------------------------------
-- Lua implementation of the "dispatch tables" as part of the CFS/MissionLib
--
-- In this area the EDS processing becomes very specific to CFE/CFS
-- and how this application will actually use it.  The objective is
-- to generate the appropriate data structures for "dispatching"
-- messages from the software bus to the local application.
-- -------------------------------------------------------------------------

local global_sym_prefix = SEDS.get_define("MISSION_NAME")
global_sym_prefix = global_sym_prefix and string.upper(global_sym_prefix) or "EDS"

local components = SEDS.mts_info.component_set
local topicid_table = SEDS.mts_info.mts_component.topicid_table

local total_intfs = SEDS.mts_info.total_intfs
local interface_list = {}


-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------

-- -----------------------------------------------------
-- helper function to generate software bus component/interface definitions
-- -----------------------------------------------------


-- -----------------------------------------------------
-- Main routine starts here
-- -----------------------------------------------------
--[[
--]]
local hdrout = SEDS.output_open(SEDS.to_filename("interfacedb.h"))

hdrout:write("enum")
hdrout:start_group("{")
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  for intftype in ds:iterate_subtree("DECLARED_INTERFACE") do
    interface_list[1 + #interface_list] = intftype
    local command_list = {}
    for cmd in intftype:iterate_subtree("COMMAND") do
      command_list[1 + #command_list] = cmd
    end
    if (#command_list > 0) then
      hdrout:write(string.format("%s_%s_BASE,", global_sym_prefix, intftype:get_ctype_basename("IndicationId")))
      for _,cmd in ipairs(command_list) do
        hdrout:write(string.format("%s_%s_VALUE,", global_sym_prefix, cmd:get_ctype_basename("IndicationId")))
      end
      hdrout:add_whitespace(1)
    end
  end
end
hdrout:write(string.format("%s_%s_MAX,", global_sym_prefix, "IndicationId"))
hdrout:end_group("};")
hdrout:add_whitespace(1)

hdrout:write("enum")
hdrout:start_group("{")
hdrout:write(string.format("%s_DispatchTableId_RESERVED = 0,", global_sym_prefix))
for _,entry in ipairs(total_intfs) do
  hdrout:write(string.format("%s_DispatchTableId_%s,", global_sym_prefix, entry.dispatchid))
end
hdrout:write(string.format("%s_DispatchTableId_MAX", global_sym_prefix))
hdrout:end_group("};")
hdrout:add_whitespace(1)

hdrout:write("enum")
hdrout:start_group("{")
hdrout:write(string.format("%s_InterfaceId_RESERVED = 0,", global_sym_prefix))
for _,intftype in ipairs(interface_list) do
  hdrout:write(string.format("%s_InterfaceId_%s,", global_sym_prefix, intftype:get_ctype_basename()))
end
hdrout:write(string.format("%s_InterfaceId_MAX", global_sym_prefix))
hdrout:end_group("};")
hdrout:add_whitespace(1)

hdrout:write(string.format("extern const struct CFE_MissionLib_SoftwareBus_Interface %s_SOFTWAREBUS_INTERFACE;", global_sym_prefix))


SEDS.output_close(hdrout)


local dbout = SEDS.output_open(SEDS.to_filename("interfacedb_impl.c"))

dbout:write("#include \"edslib_database_types.h\"")
dbout:write("#include \"cfe_missionlib_database_types.h\"")
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("interfacedb.h")))
dbout:write(string.format("#include \"%s\"",SEDS.to_filename("master_index.h")))
for ds in SEDS.root:iterate_children(SEDS.basenode_filter) do
  dbout:write(string.format("#include \"%s\"",SEDS.to_filename("interface.h", ds.name)))
end
dbout:add_whitespace(1)

for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    local intf = chain.binding.reqintf
    local cmdlist = intf.intf_commands or {}
    local arg_list = {}
    for j,cmd in ipairs(cmdlist) do
      if (#cmd.args > 0) then
        arg_list[j] = string.format("%s_%s_ARGUMENT_LIST",intf:get_flattened_name(), cmd.refnode.name)
        dbout:write(string.format("static const CFE_MissionLib_Argument_Entry_t %s[] =", arg_list[j]))
        dbout:start_group("{")
        for _,arg in ipairs(cmd.args) do
          dbout:append_previous(",")
          dbout:start_group("{")
          local ds = arg.type:find_parent(SEDS.basenode_filter)
          dbout:write(string.format(".AppIndex = %s,", ds.edslib_refobj_global_index))
          dbout:write(string.format(".TypeIndex = %s", arg.type.edslib_refobj_local_index))
          dbout:end_group("}")
        end
        dbout:end_group("};")
        dbout:add_whitespace(1)
      end
    end

    for j,cmd in ipairs(cmdlist) do
      if (cmd.subcommand_arg) then
        local derivlist = cmd.args[cmd.subcommand_arg].type.edslib_derivtable_list
        dbout:write(string.format("static const CFE_MissionLib_Subcommand_Entry_t %s_%s_SUBCOMMAND_LIST[] =",
          intf:get_flattened_name(), cmd.refnode.name))
        dbout:start_group("{")
        for _,deriv in ipairs(derivlist) do
          dbout:append_previous(",")
          dbout:start_group("{")
          dbout:write(string.format(".DispatchOffset = offsetof(struct %s, %s)",
            intf:get_ctype_basename("Commands"), deriv.name .. "_" .. cmd.refnode.name))
          dbout:end_group("}")
        end
        dbout:end_group("};")
        dbout:add_whitespace(1)
      end
    end

    local cmd_def = string.format("%s_COMMANDS",intf:get_flattened_name())
    dbout:write(string.format("static const CFE_MissionLib_Command_Definition_Entry_t %s[] =",cmd_def))
    dbout:start_group("{")
    for j,cmd in ipairs(cmdlist) do
      dbout:append_previous(",")
      dbout:start_group("{")
      if (cmd.subcommand_arg) then
        dbout:write(string.format(".SubcommandArg = %d,", cmd.subcommand_arg))
        dbout:write(string.format(".SubcommandCount = %d,",
          #cmd.args[cmd.subcommand_arg].type.edslib_derivtable_list))
        dbout:write(string.format(".SubcommandList = %s_%s_SUBCOMMAND_LIST",
          intf:get_flattened_name(), cmd.refnode.name))
      end
      if (arg_list[j]) then
        dbout:append_previous(",")
        dbout:write(string.format(".ArgumentList = %s", arg_list[j]))
      end
      dbout:end_group("}")
    end
    dbout:end_group("};")
    dbout:add_whitespace(1)
  end
end

local objname = string.format("%s_TOPICID_LOOKUP", global_sym_prefix)
local base_topicid = 1
dbout:write(string.format("static const CFE_MissionLib_TopicId_Entry_t %s[%d] =", objname, SEDS.get_define("CFE_MISSION/MAX_TOPICID") - base_topicid))
dbout:start_group("{")
for tid = 1,SEDS.get_define("CFE_MISSION/MAX_TOPICID") do
  local chain = topicid_table[tid]
  if (chain) then
    dbout:append_previous(",")
    dbout:write(string.format("[%d] =", chain.tctm.TopicId - base_topicid))
    dbout:start_group("{")
    dbout:write(string.format(".DispatchTableId = %s_DispatchTableId_%s,", global_sym_prefix, chain.binding.reqintf.mapping_info.dispatchid))
    dbout:write(string.format(".DispatchStartOffset = offsetof(struct %s,%s),",
      chain.binding.reqintf.mapping_info.dispatchtype,
      chain.binding.reqintf.name))
    dbout:write(string.format(".InterfaceId = %s_InterfaceId_%s,", global_sym_prefix, chain.binding.reqintf.type:get_ctype_basename()))
    dbout:write(string.format(".TopicName = \"%s\",", chain.binding.reqintf:get_qualified_name()))
    dbout:write(string.format(".CommandList = %s_COMMANDS",chain.binding.reqintf:get_flattened_name()))
    dbout:end_group("}")
  end
end
dbout:end_group("};")
dbout:add_whitespace(1)

dbout:section_marker("Interface Definitions")
for _,intftype in ipairs(interface_list) do
  if (intftype:find_first("COMMAND")) then
    dbout:write(string.format("static const CFE_MissionLib_Command_Prototype_Entry_t %s_BASE_COMMAND_LIST[] =", intftype:get_flattened_name()))
    dbout:start_group("{")
    for cmd in intftype:iterate_subtree("COMMAND") do
      dbout:append_previous(",")
      dbout:start_group("{")
      dbout:write(string.format(".CommandName = \"%s\",", cmd.name))
      dbout:write(string.format(".NumArguments = %d", SEDS.count_results(cmd:iterate_subtree("ARGUMENT"))))
      dbout:end_group("}")
    end
    dbout:end_group("};")
    dbout:add_whitespace(1)
  end
end

dbout:write(string.format("static const CFE_MissionLib_InterfaceId_Entry_t %s_INTERFACEID_LOOKUP[] =", global_sym_prefix))
dbout:start_group("{")
for _,intftype in ipairs(interface_list) do
  local command_count = SEDS.count_results(intftype:iterate_subtree("COMMAND"))
  local intfname = intftype:get_qualified_name()
  dbout:append_previous(",")
  dbout:start_group("{")
  dbout:write(string.format(".NumCommands = %d,", command_count))
  dbout:write(string.format(".InterfaceName = \"%s\",", intfname))
  if (command_count > 0) then
    dbout:write(string.format(".CommandList = %s,", intftype:get_flattened_name() .. "_BASE_COMMAND_LIST"))
  end
  -- Only include topicid lookup info on the TC/TM interfaces
  -- this is a bit of a hack as this doesn't really belong here.
  if (intfname == "CFE_SB/Telecommand" or intfname == "CFE_SB/Telemetry") then
    dbout:write(string.format(".NumTopics = %d,", SEDS.get_define("CFE_MISSION/MAX_TOPICID") - base_topicid))
    dbout:write(string.format(".TopicList = %s_TOPICID_LOOKUP,", global_sym_prefix))
  end
  dbout:end_group("}")
end
dbout:end_group("};")
dbout:add_whitespace(1)

dbout:section_marker("Instance Name Table")
dbout:write("#define DEFINE_TGTNAME(x)       #x,")
dbout:write(string.format("static const char * const %s_INSTANCE_LIST[] =", global_sym_prefix))
dbout:write("{")
dbout:write("#include \"cfe_mission_tgtnames.inc\"")
dbout:write("    NULL")
dbout:write("};")

dbout:section_marker("Summary Object")
dbout:write(string.format("const struct CFE_MissionLib_SoftwareBus_Interface %s_SOFTWAREBUS_INTERFACE =", global_sym_prefix))
dbout:start_group("{")
    dbout:write(string.format(".NumInterfaces = %d,", #interface_list))
    dbout:write(string.format(".InterfaceList = %s_INTERFACEID_LOOKUP,", global_sym_prefix))
    dbout:write(string.format(".InstanceList = %s_INSTANCE_LIST", global_sym_prefix))
dbout:end_group("};")
dbout:add_whitespace(1)

SEDS.output_close(dbout)
