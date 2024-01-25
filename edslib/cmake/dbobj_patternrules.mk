# Generic pattern rules for building EDS database objects

#
#  ******************************************************************************
#  **               Pattern rule to build a single database file               **
#  ******************************************************************************
#

EDSTOOL_ARCH ?= host

$(O)/%_impl.o: src/%_impl.c
	@echo EDS: Compiling $(<) for $(EDSTOOL_ARCH)
	$(CC) $(CFLAGS) -Iinc -D_EDSLIB_BUILD_ -MMD -c -o $@ $<


#
#  ******************************************************************************
#  **                Pattern rule to build a static archive file               **
#  ******************************************************************************
#

$(O)/%.a:
	@echo EDS: Archiving $(@) for $(EDSTOOL_ARCH)
	-rm -f $(@)
	$(AR) cr $@ $^


#
#  ******************************************************************************
#  **            Pattern rule to build a dynamic shared object file            **
#  ******************************************************************************
#

# This is a bit of a hack -
# Embedded targets like VxWorks and RTEMS use relocatable objects (i.e. .obj files) as
# loadable modules.  These are not shared objects (.so) in the Linux/POSIX sense, and
# need a different linker flag(s) to build them.  In lieu of somehow extracting the full
# linker logic that CMake uses, this just assumes a GNU-style LD, and that the file extension
# (.so or .obj) indicates the style of loadable object being used.

$(O)/%.so:
	@echo EDS: Linking shared object $(@) for $(EDSTOOL_ARCH)
	$(LD) $(SHARED_LDFLAGS) -shared -o $@ $^

$(O)/%.obj:
	@echo EDS: Linking relocatable object $(@) for $(EDSTOOL_ARCH)
	$(LD) $(SHARED_LDFLAGS) -r -o $@ $^
