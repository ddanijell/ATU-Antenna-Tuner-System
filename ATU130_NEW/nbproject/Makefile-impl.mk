#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile).
#
# NOCDDL

SUB_no=NO
SUBPROJECTS=${SUB_${SUB}}
BUILD_SUBPROJECTS_=.build-subprojects
BUILD_SUBPROJECTS_NO=
BUILD_SUBPROJECTS=${BUILD_SUBPROJECTS_${SUBPROJECTS}}
CLEAN_SUBPROJECTS_=.clean-subprojects
CLEAN_SUBPROJECTS_NO=
CLEAN_SUBPROJECTS=${CLEAN_SUBPROJECTS_${SUBPROJECTS}}

# Project Name
PROJECTNAME=ATU130

# Active Configuration
DEFAULTCONF=default
CONF=${DEFAULTCONF}

# All Configurations
ALLCONFS=default


# build
.build-impl: .build-pre
	${MAKE} -f nbproject/Makefile-${CONF}.mk SUBPROJECTS=${SUBPROJECTS} .build-conf


# clean
.clean-impl: .clean-pre
	${MAKE} -f nbproject/Makefile-${CONF}.mk SUBPROJECTS=${SUBPROJECTS} .clean-conf

# clobber
.clobber-impl: .clobber-pre .depcheck-impl
	    ${MAKE} SUBPROJECTS=${SUBPROJECTS} CONF=default clean



# all
.all-impl: .all-pre .depcheck-impl
	    ${MAKE} SUBPROJECTS=${SUBPROJECTS} CONF=default build



# dependency checking support
.depcheck-impl:
#	@echo "# This code depends on make tool being used" >.dep.inc
#	@if [ -n "${IGNORE_LOCAL}" ] ; then \
#	    echo "# World is not local" >>.dep.inc \
#	>> .dep.inc ; \
#	else \
#	    echo "# World is local" >>.dep.inc ; \
#	fi



# help
.help-impl: .help-pre
	@echo "This makefile implements the following targets:"
	@echo ""
	@echo "  build           build a specific configuration"
	@echo "  clean           remove built files from a configuration"
	@echo "  clobber         remove all built files"
	@echo "  all             build all configurations"
	@echo "  help            print this help message"
	@echo ""
	@echo "Available configurations are:"
	@echo ""
	@echo "  default"
	@echo ""
	@echo "Targets can be combined with configurations like this:"
	@echo "make CONF=default clean"
	@echo ""
	@echo "For more information, go to http://www.sun.com/developers/sunstudio/downloads/ssx/netbeans/"
	@echo ""

# include project implementation makefile
include nbproject/Makefile-default.mk
