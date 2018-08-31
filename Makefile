-include local.mk
include common.mk

all:
	$(MAKE) -C src all

tests: all
	$(MAKE) -C tests all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

clean-all: clean
