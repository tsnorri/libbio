include ../local.mk
include ../common.mk

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS += -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS -I../lib/Catch2/single_include

OBJECTS	=	algorithm.o \
			bit_rle_codec.o \
			main.o \
			radix_sort.o

all: tests


tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a


clean:
	$(RM) $(OBJECTS) tests
