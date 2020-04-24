-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcfmergerecords

clean:
	$(RM) $(OBJECTS) vcfmergerecords cmdline.h cmdline.c

vcfmergerecords: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)

main.cc: cmdline.c
