-include ../local.mk
include ../common.mk

.PRECIOUS: vcf_reader_parser.cc vcf_reader_header_parser.cc vcf_genotype_field_gt_parser.cc

OBJECTS		=	assert.o \
				fasta_reader.o \
				file_handling.o \
				progress_bar.o \
				progress_indicator.o \
				sequence_container.o \
				sequence_reader.o \
				types.o \
				utility.o \
				vcf_genotype_field_gt_parser.o \
				vcf_input.o \
				vcf_metadata.o \
				vcf_reader_header_parser.o \
				vcf_reader_parser.o \
				vcf_reader.o \
				vcf_subfield.o

all: libbio.a

clean:
	$(RM) $(OBJECTS) libbio.a

libbio.a: $(OBJECTS)
	$(AR) rcs $@ $^
