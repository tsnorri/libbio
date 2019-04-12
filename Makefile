-include local.mk
include common.mk

.PHONY: smoke-test


all:
	$(MAKE) -C src all

tests: all
	cd lib/Catch2 && $(PYTHON) scripts/generateSingleHeader.py
	$(MAKE) -C tests all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(RM) -rf smoke-test

clean-all: clean

smoke-test:
	$(RM) -rf smoke-test
	$(MKDIR) smoke-test
	cd include/libbio && find . -name '*.hh' ! -name '*_decl.hh' ! -name '*_def.hh' | while read x; \
	do \
		path="$${x:2}"; \
		flattened="$${path//[\/]/_}"; \
		printf "#include <libbio/%s>\n" "$${path}" > ../../smoke-test/$${flattened}.cc; \
	done
	
	echo "-include ../local.mk" > smoke-test/Makefile
	echo "include ../common.mk" >> smoke-test/Makefile
	echo 'SMOKE_TEST_OBJECTS = $$(shell ls *.cc | while read x; do echo "$$$${x%.*}.o"; done)' >> smoke-test/Makefile
	echo 'all: $$(SMOKE_TEST_OBJECTS)' >> smoke-test/Makefile
	$(MAKE) -C smoke-test

