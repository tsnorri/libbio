include ../local.mk
include ../common.mk

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS += -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS -I../lib/Catch2/single_include

OBJECTS	=	algorithm.o \
			array_list.o \
			bit_rle_codec.o \
			int_vector.o \
			main.o \
			matrix.o \
			packed_matrix.o \
			packed_vector.o \
			radix_sort.o \
			reverse_word.o \
			rle_bit_vector.o

all: tests


tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a $(BOOST_LIBS)


clean:
	$(RM) $(OBJECTS) tests
