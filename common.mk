# Ignore Xcode's setting since the SDK may contain older versions of Clang and libc++.
unexport SDKROOT

# Default values.
WARNING_FLAGS	?= -Wall -Werror -Wno-deprecated-declarations -Wno-unused
OPT_FLAGS		?= -O2 -g

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
MKDIR			?= mkdir
PYTHON			?= python
RAGEL			?= ragel
RM				?= rm

BOOST_ROOT		?= /usr
BOOST_INCLUDE	?= -I$(BOOST_ROOT)/include

CFLAGS			+= -std=c99   $(OPT_FLAGS) $(WARNING_FLAGS) $(SYSTEM_CFLAGS)
CXXFLAGS		+= -std=c++17 $(OPT_FLAGS) $(WARNING_FLAGS) $(SYSTEM_CXXFLAGS)
CPPFLAGS		+= $(SYSTEM_CPPFLAGS) $(BOOST_INCLUDE) -I../include -I../lib/GSL/include -I../lib/range-v3/include
LDFLAGS			+= $(SYSTEM_LDFLAGS)

# Assume that swift-corelibs-libdispatch is a submodule of the containing project (for now).
ifeq ($(shell uname -s),Linux)
	CPPFLAGS    += -I../../swift-corelibs-libdispatch
endif


%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

%.c: %.ggo
	$(GENGETOPT) --input="$<"

%.cc: %.rl
	$(RAGEL) -L -C -G2 -o $@ $<

%.dot: %.rl
	$(RAGEL) -V -p -o $@ $<

%.pdf: %.dot
	$(DOT) -Tpdf $< > $@
