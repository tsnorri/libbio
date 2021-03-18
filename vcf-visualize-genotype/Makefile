-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	cmdline.o \
				main.o

all: vcf_visualize_genotype

clean:
	$(RM) $(OBJECTS) vcf_visualize_genotype cmdline.h cmdline.c

vcf_visualize_genotype: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS) -lpng

main.cc: cmdline.c
