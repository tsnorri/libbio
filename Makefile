-include local.mk
include common.mk

all:
	$(MAKE) -C src all

tests: all
	cd lib/Catch2 && $(PYTHON) scripts/generateSingleHeader.py
	$(MAKE) -C tests all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

clean-all: clean
