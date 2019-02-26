-include ../local.mk
include ../common.mk

.PRECIOUS: vcf_reader.cc

OBJECTS		=	assert.o \
				fasta_reader.o \
				file_handling.o \
				progress_bar.o \
				progress_indicator.o \
				sequence_container.o \
				sequence_reader.o \
				types.o \
				utility.o \
				variant.o \
				vcf_input.o \
				vcf_reader.o

all: libbio.a

clean:
	$(RM) $(OBJECTS) libbio.a

libbio.a: $(OBJECTS)
	$(AR) rcs $@ $^
