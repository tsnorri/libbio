-include ../local.mk
include ../common.mk

.PRECIOUS: fasta_reader.cc subprocess_argument_parser.cc vcf_reader_parser.cc vcf_reader_header_parser.cc vcf_genotype_field_gt_parser.cc


OBJECTS		=	bam_fields.o \
				bam_header_parser.o \
				bam_in_order_streaming_reader.o \
				bam_record_parser.o \
				bam_unordered_streaming_reader.o \
				bed_reader.o \
				bgzf_deflate_decompressor.o \
				bgzf_parser.o \
				bgzf_streaming_reader.o \
				buffered_writer_base.o \
				circular_buffer.o \
				dispatch_event.o \
				dispatch_group.o \
				dispatch_parallel_queue.o \
				dispatch_serial_queue.o \
				dispatch_thread_local_queue.o \
				dispatch_thread_pool.o \
				fasta_reader.o \
				file_handle.o \
				file_handle_buffered_writer.o \
				file_handling.o \
				gzip_read_handle.o \
				log_memory_usage_support.o \
				memfd_handle.o \
				progress_bar.o \
				progress_indicator.o \
				sam_reader.o \
				sam_reader_header_parser.o \
				sam_reader_input_range.o \
				sam_reader_optional_field_parser.o \
				size_calculator.o \
				subprocess.o \
				subprocess_argument_parser.o \
				utility.o \
				vcf_constants.o \
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

ifeq ($(shell uname -s),Linux)
	OBJECTS	+=	dispatch_event_linux.o
else
	OBJECTS	+=	dispatch_event_kqueue.o
endif

OBJECTS_COVERAGE	= $(OBJECTS:.o=.cov.o)
GCDA				= $(OBJECTS_COVERAGE:.o=.gcda)
GCNO				= $(OBJECTS_COVERAGE:.o=.gcno)

CFLAGS		+= -fPIC
CXXFLAGS	+= -fPIC

all: libbio.a

clean:
	$(RM) $(OBJECTS) $(OBJECTS_COVERAGE) $(GCDA) $(GCNO) libbio.a libbio.coverage.a

libbio.a: $(OBJECTS)
	$(AR) rcs $@ $^

libbio.coverage.a: $(OBJECTS_COVERAGE)
	$(AR) rcs $@ $^
