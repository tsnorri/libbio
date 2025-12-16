# Ignore Xcode's setting since the SDK may contain older versions of Clang and libc++.
unexport SDKROOT

SHELL			?= /bin/bash

# Default values.
WARNING_FLAGS_		?=
WARNING_FLAGS		?= -Wall -Werror -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-unused -Wno-misleading-indentation $(WARNING_FLAGS_)
WARNING_CXXFLAGS_	?=
WARNING_CXXFLAGS	?= $(WARNING_CXXFLAGS_)
OPT_FLAGS			?= -O2 -g

CFLAGS			?=
CXXFLAGS		?=
CPPFLAGS		?=
LDFLAGS			?=
SYSTEM_CFLAGS	?=
SYSTEM_CXXFLAGS	?=
SYSTEM_CPPFLAGS	?=
SYSTEM_LDFLAGS	?=

AR				?= ar
CC				?= cc
CMAKE			?= cmake
CP				?= cp
CXX				?= c++
DOT				?= dot
GENGETOPT		?= gengetopt
GZIP			?= gzip
MKDIR			?= mkdir
PATCH			?= patch
PYTHON			?= python
RAGEL			?= ragel
RM				?= rm
GCOV			?= gcov
GCOVR			?= gcovr

BOOST_ROOT		?=

ifeq ($(BOOST_ROOT),)
BOOST_INCLUDE	?=
else
BOOST_INCLUDE	?= -isystem $(BOOST_ROOT)/include
endif

IQUOTE			=
ifneq ($(strip $(VPATH)),)
	IQUOTE += -iquote $(VPATH)
endif

CFLAGS			+= -std=c99   $(OPT_FLAGS) $(WARNING_FLAGS) $(SYSTEM_CFLAGS)
CXXFLAGS		+= -std=c++2b $(OPT_FLAGS) $(WARNING_FLAGS) $(WARNING_CXXFLAGS) $(SYSTEM_CXXFLAGS)
CPPFLAGS    	+= -I../include -isystem ../lib/range-v3/include $(BOOST_INCLUDE) $(SYSTEM_CPPFLAGS) $(IQUOTE)
LDFLAGS			+= $(SYSTEM_LDFLAGS) -lz

%.cov.o: %.cc
	$(CXX) -c --coverage $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.cov.o: %.c
	$(CC) -c --coverage $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.c: %.ggo
	$(GENGETOPT) -i $< -F $(basename $@)

%.cc: %.rl
	$(RAGEL) -L -C -G2 -o $@ $<

%.dot: %.rl
	$(RAGEL) -V -p -o $@ $<

%.pdf: %.dot
	$(DOT) -Tpdf $< > $@
