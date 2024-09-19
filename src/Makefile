-include ../local.mk
include ../common.mk

.PRECIOUS: fasta_reader.cc subprocess_argument_parser.cc vcf_reader_parser.cc vcf_reader_header_parser.cc vcf_genotype_field_gt_parser.cc

OBJECTS		=	bed_reader.o \
				buffered_writer_base.o \
				dispatch_io_channel_buffered_writer.o \
				dispatch_utility.o \
				fasta_reader.o \
				file_handle.o \
				file_handle_buffered_writer.o \
				file_handling.o \
				log_memory_usage_support.o \
				progress_bar.o \
				progress_indicator.o \
				sam_reader.o \
				sam_reader_header_parser.o \
				sam_reader_input_range.o \
				sam_reader_optional_field_parser.o \
				sequence_container.o \
				sequence_reader.o \
				subprocess.o \
				subprocess_argument_parser.o \
				types.o \
				utility.o \
				vcf_genotype_field_gt_parser.o \
				vcf_input.o \
				vcf_metadata.o \
				vcf_reader_default_delegate.o \
				vcf_reader_header_parser.o \
				vcf_reader_parser.o \
				vcf_reader.o \
				vcf_region_variant_validator.o \
				vcf_subfield.o \
				vcf_variant_format.o

OBJECTS_COVERAGE    = $(OBJECTS:.o=.cov.o)
GCDA                = $(OBJECTS_COVERAGE:.o=.gcda)
GCNO                = $(OBJECTS_COVERAGE:.o=.gcno)

CFLAGS		+= -fPIC
CXXFLAGS	+= -fPIC

all: libbio.a

clean:
	$(RM) $(OBJECTS) $(OBJECTS_COVERAGE) $(GCDA) $(GCNO) libbio.a libbio.coverage.a

libbio.a: $(OBJECTS)
	$(AR) rcs $@ $^

libbio.coverage.a: $(OBJECTS_COVERAGE)
	$(AR) rcs $@ $^
