-include ../local.mk
include ../common.mk

.PRECIOUS: fasta_reader.cc subprocess_argument_parser.cc vcf_reader_parser.cc vcf_reader_header_parser.cc vcf_genotype_field_gt_parser.cc

OBJECTS		=	assert.o \
				bed_reader.o \
				buffered_writer_base.o \
				dispatch_io_channel_buffered_writer.o \
				dispatch_utility.o \
				fasta_reader.o \
				file_handle.o \
				file_handle_buffered_writer.o \
				file_handling.o \
				progress_bar.o \
				progress_indicator.o \
				sequence_container.o \
				sequence_reader.o \
				subprocess.o \
				subprocess_argument_parser.o \
				types.o \
				utility.o \
				vcf_genotype_field_gt_parser.o \
				vcf_input.o \
				vcf_metadata.o \
				vcf_reader_header_parser.o \
				vcf_reader_parser.o \
				vcf_reader.o \
				vcf_region_variant_validator.o \
				vcf_subfield.o \
				vcf_variant_format.o

CFLAGS		+= -fPIC
CXXFLAGS	+= -fPIC

all: libbio.a

clean:
	$(RM) $(OBJECTS) libbio.a

libbio.a: $(OBJECTS)
	$(AR) rcs $@ $^
