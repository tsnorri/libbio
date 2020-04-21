-include local.mk
include common.mk

.PHONY: check-headers


all:
	$(MAKE) -C src all

tests: all
	cd lib/Catch2 && $(PYTHON) scripts/generateSingleHeader.py
	$(MAKE) -C tests all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(RM) -rf check-headers

clean-all: clean

check-headers:
	$(RM) -rf check-headers
	$(MKDIR) check-headers
	cd include/libbio && find . -name '*.hh' | while read x; \
	do \
		path="$${x:2}"; \
		flattened="$${path//[\/]/_}"; \
		printf "#include <libbio/%s>\n" "$${path}" > ../../check-headers/$${flattened}.cc; \
	done
	echo "-include ../local.mk" > check-headers/Makefile
	echo "include ../common.mk" >> check-headers/Makefile
	echo 'SMOKE_TEST_OBJECTS = $$(shell ls *.cc | while read x; do echo "$$$${x%.*}.o"; done)' >> check-headers/Makefile
	echo 'all: $$(SMOKE_TEST_OBJECTS)' >> check-headers/Makefile
	echo "" >> check-headers/Makefile
	$(MAKE) -C check-headers
