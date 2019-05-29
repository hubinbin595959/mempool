
# set debug mode ifdef DEBUG_BUILD
ifdef DEBUG_BUILD
export DEBUGMODE := 1
endif

# toolchain
ifeq ($(PLATFORM),arm)
$(info =====build arm======)
CC                      := $(CROSS_COMPILE)gcc
CXX                     := $(CROSS_COMPILE)g++
AS                      := $(CROSS_COMPILE)as
LD                      := $(CROSS_COMPILE)ld
AR                      := $(CROSS_COMPILE)ar
STRIP                   := $(CROSS_COMPILE)strip
else
CC			:= gcc
CXX			:= g++
GDC			:= gdc
AS			:= nasm
LD			:= g++
AR			:= ar
endif
MAKE		:= make

# debug/release build flags
# CPPFLAGS for c and c++ flags
CPPFLAGS	:= $(if $(DEBUGMODE),\
        -g3 -DDEBUG -Wall -Wextra,-DNDEBUG -O2) $(CPPFLAGS)

# CXXFLAGS only for cpp or cc flags
CXXFLAGS	:= $(if $(DEBUGMODE),-Woverloaded-virtual -Wreorder \
	-Wctor-dtor-privacy) $(CXXFLAGS)

ifdef CXX11
CXXFLAGS := -std=c++0x $(CXXFLAGS)
endif

# setup options for shared/static libs
CPPFLAGS	:= $(if $(or $(MKSHAREDLIB),$(MKSTATICLIB)),-fPIC) $(CPPFLAGS)
##LDFLAGS		:= $(if $(LINKSTATIC),-static) $(LDFLAGS)

LINK_A := -Wl,-Bstatic $(LINK_A)

LINK_A += -Wl,-Bdynamic

LINK_BASE_SO := -lpthread -lm -lrt -ldl

LINK_SO := -Wl,-Bdynamic $(LINK_BASE_SO) $(LINK_SO)

LDPOSTFLAGS += $(LINK_A) $(LINK_SO)

# build flags for libraries add -l
##LDPOSTFLAGS := $(addprefix -l,$(LIBRARIES)) $(LDPOSTFLAGS)

# object debug/profile suffix
BUILDSUFFIX	:= $(if $(DEBUGMODE),_d)

# work out object and dependency files
OBJECTS		:= $(addsuffix $(BUILDSUFFIX).o,$(basename $(SOURCES)))

# fixup target name
ifdef TARGET
TARGET		:= $(basename $(TARGET))$(BUILDSUFFIX)$(suffix $(TARGET))
TARGET		:= $(patsubst %.so,%,$(patsubst %.a,%,$(TARGET)))
ifneq ($(strip $(MKSHAREDLIB) $(MKSTATICLIB)),)
TARGET		:= $(TARGET)$(if $(MKSHAREDLIB),.so,$(if $(MKSTATICLIB),.a))
ifndef NOLIBPREFIX
TARGET		:= lib$(patsubst lib%,%,$(TARGET))
endif
endif
endif


ifneq "$(MAKECMDGOALS)" "clean"
ifneq "$(MAKECMDGOALS)" "clean_all"
endif
endif

# default rule
.DEFAULT_GOAL := all

#_______________________________________________________________________________
#                                                                          RULES

.PHONY:	all subdirs subprojs target clean clean_all \
	$(SUBDIRS) $(SUBPROJS)

all: subdirs subprojs target

subdirs: $(SUBDIRS)

subprojs: $(SUBPROJS)

target: $(TARGET)

clean:
ifneq ($(or $(SUBDIRS),$(SUBPROJS)),)
ifneq "$(MAKECMDGOALS)" "clean_all"
	@echo "NOT RECURSING - use 'make clean_all' to clean subdirs and " \
		"subprojs as well"
endif
endif
	rm -f $(OBJECTS) $(TARGET)

clean_all: subdirs subprojs clean


$(SUBDIRS) $(SUBPROJS):
	@if [ "$@" = "$(firstword $(SUBDIRS) $(SUBPROJS))" ]; then echo; fi
	@$(MAKE) $(if $(filter $@,$(SUBPROJS)), -f $@.mk, \
		-C $@ $(if $(wildcard $@/emake.mk),-f emake.mk,)) \
		$(filter-out $(SUBDIRS) $(SUBPROJS) subdirs subprojs,$(MAKECMDGOALS))
	@echo

$(TARGET): $(OBJECTS)
ifdef MKSTATICLIB
        $(AR) rcs $(TARGET) $(OBJECTS)
else
ifdef BUILD_IN_C
	$(CC) $(if $(MKSHAREDLIB),-shared) -o $(TARGET) $(OBJECTS) $(LDPOSTFLAGS) $(LDFLAGS)
else
	$(CXX) $(if $(MKSHAREDLIB),-shared) -o $(TARGET) $(OBJECTS) $(LDPOSTFLAGS) $(LDFLAGS)
endif
endif

%.o %_d.o %_p.o: %.c
	$(CC) -c $(CPPFLAGS) $(DEPFLAGS) $(CFLAGS) -o $@ $<

%.o %_d.o %_p.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(DEPFLAGS) $(CXXFLAGS) -o $@ $<
%.o %_d.o %_p.o: %.C
	$(CXX) -c $(CPPFLAGS) $(DEPFLAGS) $(CXXFLAGS) -o $@ $<
%.o %_d.o %_p.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(DEPFLAGS) $(CXXFLAGS) -o $@ $<

#_______________________________________________________________________________

