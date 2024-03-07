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


local write_cosmos_tlm_lineitem
local write_cosmos_cmd_lineitem
local write_cosmos_array_members
local write_cosmos_container_members
local endianness

if (SEDS.get_define("DATA_BYTE_ORDER") == "littleEndian") then
  endianness = "LITTLE"
  ccsds_append = " BIG_ENDIAN" -- The CCSDS header elements are always big-endian
else
  endianness = "BIG"
  ccsds_append = "" -- This must _not_ put a redundant BIG_ENDIAN flag on the ccsds header, it confuses cosmos
end


-- -------------------------------------------------------------------------
-- Helper function: assemble the combined element name
-- -------------------------------------------------------------------------
local append_qual_prefix = function(old_prefix,add_prefix)
  local merged_prefix

  if (old_prefix ~= nil) then
    merged_prefix = tostring(old_prefix) .. "_"
  else
    merged_prefix = ""
  end

  if (add_prefix ~= nil) then
    merged_prefix = merged_prefix .. tostring(add_prefix)
  else
    merged_prefix = merged_prefix .. "?"
  end

  return merged_prefix
end

-- -------------------------------------------------------------------------
-- Helper function: Write a single line-item to a TLM definition (APPEND_ITEM)
-- -------------------------------------------------------------------------
write_cosmos_tlm_lineitem = function(output,attribs)

  output:write(string.format("APPEND_ITEM %s %d %s \"%s\"", attribs.name, attribs.bitsize, attribs.ctype, attribs.descr))

end

-- -------------------------------------------------------------------------
-- Helper function: Write a single line-item to a CMD definition (APPEND_PARAMETER)
-- -------------------------------------------------------------------------
write_cosmos_cmd_lineitem = function(output,attribs)

  -- The APPEND_PARAMETER format is a bit different for strings vs. integer values
  -- the integer items have a min/max value and the default value is quoted on strings

  if (attribs.ctype == "STRING") then

    -- String lineitem
    output:write(string.format("APPEND_PARAMETER %s %d STRING \"%s\" \"%s\"",
      attribs.name, attribs.bitsize, attribs.defaultval or "", attribs.descr or ""))

  elseif (attribs.ctype) then

    -- Numeric lineitem

    local min = attribs.min
    local max = attribs.max

    if (min == nil) then
      min = "MIN_" .. attribs.ctype .. tostring(attribs.bitsize)
    else
      min = math.ceil(min)
    end

    if (max == nil) then
      max = "MAX_" .. attribs.ctype .. tostring(attribs.bitsize)
    else
      max = math.floor(max)
    end

    output:write(string.format("APPEND_PARAMETER %s %d %s %s %s %d \"%s\"",
      attribs.name, attribs.bitsize, attribs.ctype, min, max, attribs.defaultval or 0, attribs.descr or ""))

  end

end

-- -------------------------------------------------------------------------
-- Helper function: Invoke the line_writer function for the given entry
-- This can be CMD or TLM depending on the value of line_writer
-- -------------------------------------------------------------------------
write_cosmos_lineitem = function(output,line_writer,entry,qual_prefix,descr)

  local attribs = {}

  attribs.name = qual_prefix and SEDS.to_macro_name(qual_prefix)
  attribs.bitsize = entry.resolved_size and entry.resolved_size.bits or 0

  if (not descr) then
    descr = entry.attributes.shortdescription or "Value"
  end

  -- The description should not have newlines or other big chunks of whitespace in it
  descr = descr:gsub("[\t\n ]+", " ")
  descr = descr:gsub("\"", "\'")
  attribs.descr = descr

  if (entry.entity_type == "STRING_DATATYPE" or entry.entity_type == "BINARY_DATATYPE") then
    attribs.ctype = "STRING"
  elseif (SEDS.index_datatype_filter(entry)) then
    attribs.ctype = (entry.is_signed and "INT") or "UINT"

    -- This only handles integers
    if (entry.resolved_range) then
      attribs.min = entry.resolved_range.min.value
      if (not entry.resolved_range.min.inclusive) then
        attribs.min = attribs.min + 1
      end
      attribs.max = entry.resolved_range.max.value
      if (not entry.resolved_range.max.inclusive) then
        attribs.max = attribs.max - 1
      end
    end
  end

  line_writer(output,attribs)

end

-- -------------------------------------------------------------------------
-- Helper function: Convert the given entry to a block of COSMOS DB line items
-- This can be CMD or TLM depending on the value of line_writer
-- -------------------------------------------------------------------------
write_cosmos_block = function(output,line_writer,entry,qual_prefix,descr)

  -- flatten all subcontainers
  if (entry.entity_type == "ARRAY_DATATYPE") then
    write_cosmos_array_members(output,line_writer,entry,qual_prefix)
  elseif (entry.entity_type == "CONTAINER_DATATYPE") then
    write_cosmos_container_members(output,line_writer,entry,qual_prefix)
  elseif (entry.name) then
    write_cosmos_lineitem(output,line_writer,entry,qual_prefix,descr)
  end

end


-- -------------------------------------------------------------------------
-- Helper function: write a flattened array, adding a digit to the name prefix
-- -------------------------------------------------------------------------
write_cosmos_array_members = function(output,line_writer,arr,qual_prefix)

  local min, max

  -- Arrays can be dimensioned by an index type, and that type may have a better string representation
  -- This also means that the range could be something other than the typical limits
  local dimension_typename

  for dimension in arr:iterate_subtree("DIMENSION") do
    if (dimension.indextyperef) then
      dimension_typename = dimension.indextyperef:get_qualified_name()
      min = dimension.indextyperef.resolved_range.min
      max = dimension.indextyperef.resolved_range.max
    end
  end

  -- As a fallback use a generic 32-bit integer as the index type
  local dimension_obj = SEDS.edslib.NewObject(dimension_typename or "BASE_TYPES/int32")

  -- Determine the usable min/max -- this means adjusting for inclusiveness of the limits
  local minv
  if (min) then
    minv = min.value + (min.inclusive and 0 or 1)
  else
    minv = 0
  end

  local maxv
  if (max) then
    maxv = max.value - (max.inclusive and 0 or 1)
  else
    maxv = arr.total_elements - 1
  end

  for i=minv,maxv do

    dimension_obj(i)
    write_cosmos_block(output,line_writer,arr.datatyperef,append_qual_prefix(qual_prefix, dimension_obj))

  end

end


-- -------------------------------------------------------------------------
-- Helper function: write a flattened container, adding member name to the prefix
-- -------------------------------------------------------------------------
write_cosmos_container_members = function(output,line_writer,cont,qual_prefix)

  for _,ntype,refnode in cont:iterate_members() do
    -- By checking refnode this includes only direct entries, not basetypes
    if (ntype) then
      write_cosmos_block(output,line_writer,ntype, append_qual_prefix(qual_prefix, refnode and refnode.name), refnode and refnode.attributes.shortdescription)
    end
  end

end

-- -------------------------------------------------------------------------
-- Helper function: write a complete TLM message container to the output file
-- -------------------------------------------------------------------------
local write_tlm_intf_items = function (output,ds,reqintf,msgid,argtype)

  local tlmname = SEDS.to_macro_name(ds.name .. "_" .. reqintf.name)

  if (string.sub(tlmname, -4, -1) == "_TLM") then
    tlmname = string.sub(tlmname, 1, -5)
  end

  output:write(string.format("TELEMETRY %s %s_ENDIAN \"%s\"", tlmname,
    endianness, reqintf.attributes.shortdescription or "Telemetry Message"))

  output:start_group("")


  -- BACKWARD COMPATIBILITY: A proper identification of the packet involves multiple fields.
  -- Historically these were merged into a single 16-bit "StreamID" value and that's what
  -- appears to be done in existing/old cosmos DB files.  This mimics that for now.  There
  -- should be a better/more correct way to do this.  But for now, this just duplicates the
  -- simplified (3x 16-bit UINT) view of the CCSDS v1 header.  These should always be big-endian.
  output:write(string.format("APPEND_ID_ITEM CCSDS_STREAMID 16 UINT 0x%04X \"CCSDS Packet Identification\" FORMAT_STRING \"0x%%04X\"%s", msgid.Value(), ccsds_append))
  output:write(string.format("APPEND_ITEM CCSDS_SEQUENCE 16 UINT \"CCSDS Packet Sequence Control\" FORMAT_STRING \"0x%%04X\"%s", ccsds_append))
  output:write(string.format("APPEND_ITEM CCSDS_LENGTH 16 UINT \"CCSDS Packet Data Length\"%s", ccsds_append))

  -- Write the definition of the payload
  -- Do not output the basetype, only direct members (Payload)
  -- This is done by checking that refnode is non-nil
  for _,ntype,refnode in argtype:iterate_members() do
    if (ntype and refnode) then
      write_cosmos_container_members(output, write_cosmos_tlm_lineitem, ntype)
    end
  end

  output:end_group("")

end

-- -------------------------------------------------------------------------
-- Helper function: write a complete CMD message container to the output file
-- -------------------------------------------------------------------------
local write_cmd_intf_params = function (output,ds,reqintf,msgid,cc)

  local argtype = cc.argtype
  local cmdname = SEDS.to_macro_name(ds.name .. "_" .. reqintf.name .. "_" .. argtype.name)

  if (string.sub(cmdname, -4, -1) == "_CMD") then
    cmdname = string.sub(cmdname, 1, -5)
  end

  output:write(string.format("COMMAND %s %s_ENDIAN \"%s\"",  cmdname,
      endianness, argtype.attributes.shortdescription or "Telecommand Message"))

  output:start_group("")

  -- BACKWARD COMPATIBILITY: A proper identification of the packet involves multiple fields.
  -- Historically these were merged into a single 16-bit "StreamID" value and that's what
  -- appears to be done in existing/old cosmos DB files.  This mimics that for now.  There
  -- should be a better/more correct way to do this.  But for now, this just duplicates the
  -- simplified (3x 16-bit UINT) view of the CCSDS v1 header.  These should always be big-endian.
  output:write(string.format("APPEND_ID_PARAMETER CCSDS_STREAMID 16 UINT MIN MAX 0x%04X \"CCSDS Packet Identification\" FORMAT_STRING \"0x%%04X\"%s", msgid.Value(), ccsds_append))
  output:write(string.format("APPEND_PARAMETER CCSDS_SEQUENCE 16 UINT MIN MAX 0xC000 \"CCSDS Packet Sequence Control\" FORMAT_STRING \"0x%%04X\"%s", ccsds_append))
  output:write(string.format("APPEND_PARAMETER CCSDS_LENGTH 16 UINT MIN MAX %d \"CCSDS Packet Data Length\"%s", math.ceil(argtype.resolved_size.bits / 8) - 7, ccsds_append))
  output:write(string.format("APPEND_PARAMETER CCSDS_FC 8 UINT MIN MAX %d \"CCSDS Command Function Code\"", cc.value))
  output:write(string.format("APPEND_PARAMETER CCSDS_CHECKSUM 8 UINT MIN MIN 0 \"Checksum\""))

  -- Write the definition of the payload
  -- Do not output the basetype, only direct members (Payload)
  -- This is done by checking that refnode is non-nil
  for _,ntype,refnode in argtype:iterate_members() do
    if (ntype and refnode) then
      write_cosmos_container_members(output, write_cosmos_cmd_lineitem, ntype)
    end
  end

  output:end_group("")
end


-- -------------------------
-- MAIN ROUTINE
-- -------------------------

SEDS.output_mkdir("cosmos")

for _,instance in ipairs(SEDS.highlevel_interfaces) do
  local ds = instance.component:find_parent(SEDS.basenode_filter)

  -- The various interfaces should be attached under required_links
  for _,binding in ipairs(instance.required_links) do
    local reqintf = binding.reqintf
    local cmd = reqintf.intf_commands and reqintf.intf_commands[1]

    -- SB interfaces should only have one "command" of the indication type (i.e. message appears on the bus)
    -- Note "command" here is the EDS terminology, not CFE - the CFE intf is referred to as "telecommand"
    -- The indication has one argument, which is the data associated with the message
    if (cmd.refnode.name == "indication") then
      local sb_params = binding.provinst:find_param_of_type("CFE_SB/SoftwareBusRouting")
      local argtype = cmd.args[1].type

      if (sb_params) then
        local file = "cosmos/" .. SEDS.to_filename(reqintf.name .. "_cosmos_intf.txt", ds.name)
        local msgid = sb_params.PubSub.MsgId -- This is the actual hex msgid value
        local output

        if (reqintf.type.name == "Telecommand") then
          if (cmd.cc_list) then
            output = SEDS.output_open(file)
            for _,cc in ipairs(cmd.cc_list) do
              write_cmd_intf_params(output,ds,reqintf,msgid,cc)
              output:add_whitespace(1)
            end
            SEDS.output_close(output)
          end
        elseif (reqintf.type.name == "Telemetry") then
          output = SEDS.output_open(file)
          write_tlm_intf_items(output,ds,reqintf,msgid,argtype)
          SEDS.output_close(output)
        end

      end
    end
  end
end

-- SEDS.error("stop")
