-include ../local.mk
include ../common.mk

RAPIDCHECK_BUILD_DIR ?= build

# macOS 10.11 does not have std::uncaught_exceptions. We could use a custom-installed libc++
# but set a configuration option instead for now.
CPPFLAGS +=	-Iinclude \
			-I../lib/rapidcheck/include \
			-I../lib/rapidcheck/extras/catch/include \

ifeq ($(origin NO_COVERAGE_CHECK),undefined)
TEST_LDFLAGS = -lgcov $(BOOST_LIBS) $(LDFLAGS)
LIBBIO_PATH ?= ../src/libbio.coverage.a
else
TEST_LDFLAGS = $(BOOST_LIBS) $(LDFLAGS)
LIBBIO_PATH ?= ../src/libbio.a
endif

CXXFLAGS +=	-ftemplate-backtrace-limit=0


OBJECTS	=	cmdline.o \
			main.o

TEST_DEPENDENCIES =	$(LIBBIO_PATH) \
					../lib/rapidcheck/build/librapidcheck.a

all: libbio_rc_test_driver.a

.PHONY: all clean

clean:
	$(RM) $(OBJECTS) libbio_rc_test_driver.a

libbio_rc_test_driver.a: $(OBJECTS)
	$(AR) rcs $@ $^

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<
