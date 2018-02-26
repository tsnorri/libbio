-include local.mk
include common.mk

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
