-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcf_private_variants

clean:
	$(RM) $(OBJECTS) vcf_private_variants cmdline.h cmdline.c

# FIXME: Take libraries from a makefile.
vcf_private_variants: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a

main.cc: cmdline.c
