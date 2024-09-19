-include ../local.mk
include ../common.mk

.PRECIOUS: 

OBJECTS		=	main.o

all: read_allocation_log

clean:
	$(RM) $(OBJECTS) read_allocation_log cmdline.h cmdline.c

read_allocation_log: $(OBJECTS) ../src/libbio.a
	$(CXX) -o $@ $(OBJECTS) ../src/libbio.a $(BOOST_LIBS) $(LDFLAGS)
