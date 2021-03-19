-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcf_visualize_genotype

clean:
	$(RM) $(OBJECTS) vcf_visualize_genotype cmdline.h cmdline.c

# FIXME: Take libraries from a makefile.
vcf_visualize_genotype: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a /usr/lib/x86_64-linux-gnu/libtiff.a /usr/lib/x86_64-linux-gnu/libtiffxx.a /usr/lib/x86_64-linux-gnu/libjpeg.a /usr/lib/x86_64-linux-gnu/libjbig.a /usr/lib/x86_64-linux-gnu/liblzma.a -lz

main.cc: cmdline.c
