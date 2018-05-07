-include local.mk
include common.mk

all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean

clean-all: clean
