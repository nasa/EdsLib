include edstool-buildenv.d
include edstool-sources.d

LOCAL_STAMPFILE := edstool-complete.stamp
MISSION_PARAM_HEADER := inc/cfe_mission_eds_parameters.h
INTFDB_PARAM_HEADER := inc/cfe_mission_eds_interface_parameters.h
O := $(OBJDIR)


# NOTE: At the mission scope, this runs the full EDS tool to generate all artifacts
# As part of this, it invokes the db_objects make scripts as well, so there is noting
# extra that needs to be invoked from here.

.PHONY: all
all: $(LOCAL_STAMPFILE)
	@echo All host side global EDS objects built

# The file lists here came from CMake, and so they have the semicolon separator,
# which needs to be changed to a space separator for interpretation here.
ALL_EDS_FILES += $(subst ;, ,$(MISSION_EDS_FILELIST))
ALL_EDS_FILES += $(subst ;, ,$(MISSION_EDS_SCRIPTLIST))

$(LOCAL_STAMPFILE): $(EDSTOOL) $(MISSION_PARAM_HEADER) $(INTFDB_PARAM_HEADER) $(ALL_EDS_FILES)
	+$(EDSTOOL) -DMAKE_PROGRAM="$(MAKE)" -DOBJDIR=$(OBJDIR) $(ALL_EDS_FILES)
	$(CMAKE) -E touch "$(@)"

$(MISSION_PARAM_HEADER): $(EDS_CFECFS_MISSIONLIB_SOURCE_DIR)/cmake/cfe_mission_eds_parameters.h.in obj/edstool-buildenv.d
	@echo "configure_file($(<) $(@))" > src/generate_eds_parameters_h.cmake
	$(CMAKE) -D MISSION_NAME=$(MISSION_NAME) -D EDS_FILE_PREFIX=$(EDS_FILE_PREFIX) -D EDS_SYMBOL_PREFIX=$(EDS_SYMBOL_PREFIX) -P src/generate_eds_parameters_h.cmake

$(INTFDB_PARAM_HEADER): $(EDS_CFECFS_MISSIONLIB_SOURCE_DIR)/cmake/cfe_mission_eds_interface_parameters.h.in obj/edstool-buildenv.d
	@echo "configure_file($(<) $(@))" > src/cfe_mission_eds_interface_parameters.cmake
	$(CMAKE) -D MISSION_NAME=$(MISSION_NAME) -D EDS_FILE_PREFIX=$(EDS_FILE_PREFIX) -D EDS_SYMBOL_PREFIX=$(EDS_SYMBOL_PREFIX) -P src/cfe_mission_eds_interface_parameters.cmake
