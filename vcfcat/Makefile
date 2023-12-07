-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcfcat

clean:
	$(RM) $(OBJECTS) vcfcat cmdline.h cmdline.c

vcfcat: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) ../src/libbio.a $(BOOST_LIBS) $(LDFLAGS)

main.cc: cmdline.c
