# Rules for EDS-based CFE table generation

# the basic set of flags to pass to the table tool
TBLTOOL_FLAGS += -e MISSION_DEFS=\"$(MISSION_DEFS)\"
TBLTOOL_FLAGS += -e CPUNAME=\"$(CFE_TABLE_CPUNAME)\"
TBLTOOL_FLAGS += -e APPNAME=\"$(CFE_TABLE_APPNAME)\"
TBLTOOL_FLAGS += -e TABLENAME=\"$(CFE_TABLE_BASENAME)\"

LOCAL_CFLAGS += $(addprefix -D,$(C_COMPILE_DEFS))
LOCAL_CFLAGS += $(addprefix -I,$(C_INCLUDE_DIRS))
LOCAL_CFLAGS += -DCFE_TABLE_NAME="$(CFE_TABLE_BASENAME)"

AWK ?= /usr/bin/awk
M4  ?= /usr/bin/m4

.PRECIOUS: eds/%.c

# This needs to strip comments from the original C file -
# the best way to do that is to send it through the C preprocessor (-E switch)
# NOTE: by defining CFE_TBL_FILEDEF_H here, we effectively squelch the normal CFE definition of the macro.
# this way we can use our own macro definition instead, and actually evaluate it several different ways.
eds/%.source:
	@mkdir -pv $(dir $(@))
	echo "$(<)" > "$(@)"

eds/%.filedef: SRC=$(shell cat $(@:.filedef=.source))
eds/%.filedef: eds/%.source $(TABLETOOL_SCRIPT_DIR)/extract_filedef.awk
	$(CC) -E -MMD -MF "$(notdir $(@)).d" -MT "$(@)" -DCFE_TBL_FILEDEF_H $(CFLAGS) $(LOCAL_CFLAGS) -o "$(@).tmp" "$(SRC)"
	$(AWK) -v SRC="$(SRC)" -f "$(TABLETOOL_SCRIPT_DIR)/extract_filedef.awk" "$(@).tmp" > "$(@).out"
	mv "$(@).out" "$(@)"
	rm -f "$(@).tmp"

eds/%.objname: eds/%.filedef $(TABLETOOL_SCRIPT_DIR)/extract_varname.m4
	$(M4) -P "$(TABLETOOL_SCRIPT_DIR)/extract_varname.m4" "$(<)" > "$(@)"

eds/%.c: SRC=$(shell cat $(@:.c=.source))
eds/%.c: OBJNAME=$(shell cat $(@:.c=.objname))
eds/%.c: eds/%.objname $(TABLETOOL_SCRIPT_DIR)/extract_object.awk
	$(AWK) -v SRC="$(SRC)" -v OBJNAME="$(OBJNAME)" -f "$(TABLETOOL_SCRIPT_DIR)/extract_object.awk" "$(SRC)" > "$(@).out"
	mv "$(@).out" "$(@)"

eds/%.o: eds/%.c $(TABLETOOL_SCRIPT_DIR)/eds_tbltool_filedef.h
	$(CC) -Wall -Werror -fPIC -g $(CFLAGS) $(LOCAL_CFLAGS) -include "$(TABLETOOL_SCRIPT_DIR)/eds_tbltool_filedef.h" -o "$(@)" -c "$(<)"

%.so: %.o
	$(CC) $(CFLAGS) -shared -o $(@) $(<)
