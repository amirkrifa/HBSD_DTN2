#
# System.make: settings extracted from the oasys configuration
#
# System.make.  Generated from System.make.in by configure.

#
# Programs
#
AR		= ar
RANLIB		= ranlib
DEPFLAGS	= -MMD -MP -MT "$*.o $*.E"
INSTALL 	= /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA 	= ${INSTALL} -m 644
PYTHON		= /usr/local/bin/python
PYTHON_BUILD_EXT= yes
XSD_TOOL	= 

#
# System-wide compilation flags including the oasys-dependent external
# libraries.
#
SYS_CFLAGS          = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
SYS_EXTLIB_CFLAGS   =  -I/user/akrifa/home/Xerces-C_2_8_0/include
SYS_EXTLIB_LDFLAGS  =  -lpthread  -ldl -lm  -ltcl8.4 -lexpat -L/user/akrifa/home/Xerces-C_2_8_0/lib -lxerces-c -lreadline  -lz  -ldb-4.5

#
# Library-specific compilation flags
TCL_LDFLAGS     = -ltcl8.4

