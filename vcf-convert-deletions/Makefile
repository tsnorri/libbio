-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcf_convert_deletions

clean:
	$(RM) $(OBJECTS) vcf_convert_deletions cmdline.h cmdline.c

vcf_convert_deletions: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)

main.cc: cmdline.c
