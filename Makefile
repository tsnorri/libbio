-include local.mk
include common.mk


all: src/libbio.a

.PHONY: clean clean-all check-headers

tests: tests/coverage/index.html

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	$(RM) -rf check-headers

clean-all: clean
	$(RM) -rf lib/rapidcheck/build
	$(RM) -rf lib/Catch2/build

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

src/libbio.a:
	$(MAKE) -C src all

src/libbio.coverage.a:
	$(MAKE) -C src libbio.coverage.a

tests/coverage/index.html: src/libbio.coverage.a lib/rapidcheck/build/librapidcheck.a lib/Catch2/build/src/libCatch2.a
	$(MAKE) -C tests coverage

lib/rapidcheck/build/librapidcheck.a:
	$(RM) -rf lib/rapidcheck/build && \
	cd lib/rapidcheck && \
	$(MKDIR) build && \
	cd build && \
	$(CMAKE) \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DBUILD_SHARED_LIBS=OFF \
		.. && \
	$(MAKE)

lib/Catch2/build/src/libCatch2.a:
	$(RM) -rf lib/Catch2/build && \
	cd lib/Catch2 && \
	$(MKDIR) build && \
	cd build && \
	$(CMAKE) \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DBUILD_SHARED_LIBS=OFF \
		.. && \
	VERBOSE=1 $(MAKE)

