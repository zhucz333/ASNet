A=@
SRC_ROOT=$(shell pwd)
X86	=x86
X64	=x64
DEBUG	=Debug
RELEASE =Release

##----------------------------------------------------------
CC    = gcc
CXX   = g++
AR    = ar
RM    = rm
ECHO  = echo
MV    = mv
MAKE  = make
CD    = cd
CP    = cp
RANLIB= ranlib

##-----------install path-----------------------
TARGETS_ARCH =
TARGETS_PATH = 

##-----------OS/COMPILE SPECIAL FLAGS------------
COSFLAGS = -fpermissive -finline-functions -fPIC
AROSFLAGS= 
OFLAG    = -O0 -std=c++2a 

ifeq ($(X86),$(ARCH))
OFLAG += -m32
else ifeq ($(X64),$(ARCH))
OFLAG += -m64
endif

##----------------------------------------------------------

ifeq ($(DEBUG),$(VERSION))
CFLAGS   = -g $(COSFLAGS) $(OFLAG)
CXXFLAGS = -g $(COSFLAGS) $(OFLAG)
else ifeq ($(RELEASE),$(VERSION))
CFLAGS   = $(COSFLAGS) $(OFLAG)
CXXFLAGS = $(COSFLAGS) $(OFLAG)
endif


DLLFLAGS = -shared
ARFLAGS  = $(AROSFLAGS) -ruc
LIBS	 = -lpthread
##----------include lib path--------------------------------
INCPATH  = 
LIBPATH  =

INCPATH  += 

ifeq ($(X86),$(ARCH))
LIBPATH  += 
else ifeq ($(X64),$(ARCH))
LIBPATH  += 
endif

##----------------------------------------------------------
.SUFFIXES: .c .cpp .C .o

.cpp.o   :
	$(A)$(ECHO) "Compiling [cpp] file:[$@] ..."
	$(A)$(CXX) $(CXXFLAGS) $(INCPATH) -c $<

.C.o   :
	$(A)$(ECHO) "Compiling [cpp] file:[$@] ..."
	$(A)$(CXX) $(CXXFLAGS) $(INCPATH) -c $<
                                      
.c.o     :                            
	$(A)$(ECHO) "Compiling [ c ] file:[$@] ..."
	$(A)$(CC)  $(CFLAGS)   $(INCPATH) -c $<

