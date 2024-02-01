###########################################################
#
# EDS CFE platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# on target architecture builds, the "edstool-execute" target
# will not actually run the tool again.  Instead, it will just build all
# the generated make targets from the parent build for this processor by
# supplying the correct CC/AR tool

add_custom_target(edstool-execute
    COMMAND "\$(MAKE)"
        ARCH_BINARY_DIR=${CMAKE_BINARY_DIR}
        -f ${missionlib_MISSION_DIR}/cmake/edstool-execute-arch.mk
        all
    WORKING_DIRECTORY
        ${MISSION_BINARY_DIR}
    DEPENDS
        missionlib-runtime-install
)
