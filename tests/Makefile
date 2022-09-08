-include ../local.mk
include ../common.mk

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS += -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS -I../lib/Catch2/single_include

OBJECTS	=	algorithm.o \
			array_list.o \
			assert.o \
			atomic_int_matrix.o \
			atomic_int_vector.o \
			bit_rle_codec.o \
			buffer.o \
			dispatch_io_channel_buffered_writer.o \
			fasta_reader.o \
			file_handle_buffered_writer.o \
			int_vector.o \
			is_lt.o \
			is_lte.o \
			is_equal.o \
			main.o \
			matrix.o \
			merge_projected.o \
			radix_sort.o \
			reverse_word.o \
			rle_bit_vector.o \
			subprocess_argument_parser.o \
			vcf_reader.o

GENERATED_TEST_FILES = 	test-files/test-data-types.vcf.gz \
						test-files/test-simple.vcf.gz

CATCH2_HEADERS	= $(shell find ../lib/Catch2/include)

all: tests


tests: $(OBJECTS) $(GENERATED_TEST_FILES)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)


clean:
	$(RM) $(OBJECTS) $(GENERATED_TEST_FILES) tests


../lib/Catch2/single_include/catch2/catch.hpp: $(CATCH2_HEADERS)
	cd ../lib/Catch2 && $(PYTHON) scripts/generateSingleHeader.py

%.vcf.gz: %.vcf
	$(GZIP) -k $<
