#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Linux_Release_64
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/CARecord.o \
	${OBJECTDIR}/CheckSystemListener.o \
	${OBJECTDIR}/MBusAuthorityListener.o \
	${OBJECTDIR}/common.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/tinyxml2.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-m64 -Wall -MMD -Wshadow -Wno-write-strings -Wno-deprecated
CXXFLAGS=-m64 -Wall -MMD -Wshadow -Wno-write-strings -Wno-deprecated

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lpthread ../../Speedy/Release/linux/UFC_64.a

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ../Release/linux/MBusExample_64

../Release/linux/MBusExample_64: ../../Speedy/Release/linux/UFC_64.a

../Release/linux/MBusExample_64: ${OBJECTFILES}
	${MKDIR} -p ../Release/linux
	${LINK.cc} -o ../Release/linux/MBusExample_64 ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/CARecord.o: CARecord.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CARecord.o CARecord.cpp

${OBJECTDIR}/CheckSystemListener.o: CheckSystemListener.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CheckSystemListener.o CheckSystemListener.cpp

${OBJECTDIR}/MBusAuthorityListener.o: MBusAuthorityListener.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MBusAuthorityListener.o MBusAuthorityListener.cpp

${OBJECTDIR}/common.o: common.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/common.o common.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/tinyxml2.o: tinyxml2.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O3 -Wall -D__LINUX -I../../Speedy/Migo -I../../Speedy/UFC -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyxml2.o tinyxml2.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ../Release/linux/MBusExample_64

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
