-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: msa_identity_regions

clean:
	$(RM) $(OBJECTS) msa_identity_regions cmdline.h cmdline.c

msa_identity_regions: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)

main.cc: cmdline.c
