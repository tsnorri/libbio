-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcf_concatenate_records

clean:
	$(RM) $(OBJECTS) vcf_concatenate_records cmdline.h cmdline.c

vcf_concatenate_records: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)

main.cc: cmdline.c
