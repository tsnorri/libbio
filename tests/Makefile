include ../local.mk
include ../common.mk

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS += -I../lib/Catch2/single_include -DCATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS

OBJECTS	=	main.o \
			bit_rle_codec.o

all: tests


tests: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) ../src/libbio.a


clean:
	$(RM) $(OBJECTS) tests
