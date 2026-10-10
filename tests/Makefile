-include ../local.mk
include ../common.mk

RAPIDCHECK_BUILD_DIR ?= build

CPPFLAGS_RC := \
	$(CPPFLAGS) \
	-isystem ../lib/rapidcheck/include \
	-I../rapidcheck-test-driver/include

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS +=	-DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS \
			-I../rapidcheck-test-driver/include \
			-isystem ../lib/Catch2/src \
			-isystem ../lib/Catch2/build/generated-includes \
			-isystem ../lib/rapidcheck/include \
			-isystem ../lib/rapidcheck/extras/catch/include

ifeq ($(origin NO_COVERAGE_CHECK),undefined)
TEST_LDFLAGS = -coverage $(BOOST_LIBS) $(LDFLAGS)
LIBBIO_PATH ?= ../src/libbio.coverage.a
else
TEST_LDFLAGS = $(BOOST_LIBS) $(LDFLAGS)
LIBBIO_PATH ?= ../src/libbio.a
endif

CXXFLAGS +=	-ftemplate-backtrace-limit=0


OBJECTS	=	algorithm.o \
			array_list.o \
			assert.o \
			bit_reading_stream_arbitrary.o \
			bit_writing_stream_arbitrary.o \
			buffer.o \
			disjoint_sets_arbitrary.o \
			dispatch_event_manager.o \
			encoding.o \
			encoding_arbitrary.o \
			fasta_reader.o \
			fasta_reader_arbitrary.o \
			fastq_reader_arbitrary.o \
			file_handle_buffered_writer.o \
			generic_parser.o \
			generic_parser_traits.o \
			int_matrix.o \
			int_vector.o \
			is_lt.o \
			is_lte.o \
			is_equal.o \
			matrix.o \
			merge_projected.o \
			partial_word.o \
			pearson_correlation_arbitrary.o \
			power_of_two.o \
			power_of_two_arbitrary.o \
			radix_sort.o \
			reverse_word.o \
			reverse_word_arbitrary.o \
			sam_reader_arbitrary.o \
			set_difference_inplace_arbitrary.o \
			sorted_set_union.o \
			stable_partition_left_arbitrary.o \
			subprocess_argument_parser.o \
			vcf_reader.o

OBJECTS_RC	=	bit_reading_stream_arbitrary.rc.o \
				bit_writing_stream_arbitrary.rc.o \
				disjoint_sets_arbitrary.rc.o \
				encoding_arbitrary.rc.o \
				fasta_reader_arbitrary.rc.o \
				fastq_reader_arbitrary.rc.o \
				partial_word.rc.o \
				pearson_correlation_arbitrary.rc.o \
				power_of_two_arbitrary.rc.o \
				reverse_word_arbitrary.rc.o \
				sam_reader_arbitrary.rc.o \
				set_difference_inplace_arbitrary.rc.o \
				stable_partition_left_arbitrary.rc.o

GENERATED_TEST_FILES = 	test-files/test-data-types.vcf.gz \
						test-files/test-simple.vcf.gz

TEST_DEPENDENCIES =	$(LIBBIO_PATH) \
					../lib/rapidcheck/build/librapidcheck.a \
					../lib/Catch2/build/src/libCatch2Main.a \
					../lib/Catch2/build/src/libCatch2.a

all: tests

.PHONY: rapidcheck clean run-tests

run-tests: tests
	./tests

coverage: coverage/index.html

coverage/index.html: run-tests
	$(MKDIR) -p coverage
	$(GCOVR) --gcov-executable $(GCOV) --root .. --filter ../include --filter ../src --print-summary --html --html-details -o coverage/index.html ../src

rapidcheck: rc_tests

tests: $(OBJECTS) $(GENERATED_TEST_FILES) $(TEST_DEPENDENCIES)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(TEST_DEPENDENCIES) $(TEST_LDFLAGS)

rc_tests: $(OBJECTS_RC)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS_RC) ../rapidcheck-test-driver/libbio_rc_test_driver.a $(LIBBIO_PATH) ../lib/rapidcheck/$(RAPIDCHECK_BUILD_DIR)/librapidcheck.a $(BOOST_LIBS) $(TEST_LDFLAGS)

clean:
	$(RM) $(OBJECTS) $(OBJECTS_RC) $(OBJECTS_COVERAGE) $(GENERATED_TEST_FILES) tests rc_tests
	$(RM) -r coverage

%.rc.o: %.cc
	$(CXX) -DLIBBIO_BUILD_RAPIDCHECK_TEST_DRIVER -c $(CXXFLAGS) $(CPPFLAGS_RC) -o $@ $<

%.vcf.gz: %.vcf
	$(GZIP) -k $<
