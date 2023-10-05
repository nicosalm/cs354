
# "phony" targets don't actually correspond to real files. They are more like
# aliases that trigger other targets to run
.PHONY: all heap $(RUNTESTS_TARGETS) buildtests runtests

# If you just say "make", without specifying what, it runs the first target in
# the Makefile, which is this one.
default: heap

# Collect a list of all tests in the tests directory.
ALL_TESTS=$(patsubst tests/%.c, %, $(wildcard tests/*.c))
ALL_TESTS_BINARIES=$(patsubst tests/%.c, tests/%, $(wildcard tests/*.c))

# Define `make buildtests`, which causes all test programs to be built.
buildtests: $(ALL_TESTS_BINARIES)

# Define `make runtests`, which causes all test programs to be run and then
# prints out a summary. Note that we cause all the tests to run by defining
# them as dependencies and then having make run them via the `*.run` targets
# defined later. The commands here just print the summary.
RUNTESTS_TARGETS=$(patsubst %, %.run, $(ALL_TESTS))
runtests: $(RUNTESTS_TARGETS)
	@echo @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@echo PASSED: && grep -rl "PASS" tests/ || echo NONE
	@echo
	@echo FAILED: && grep -rl "FAIL" tests/ | xargs -d '\n' tail -n 3 ; \
		grep -qr "FAIL" tests/ || echo NONE

# Build the heap implementation itself into a shared library
heap: libheap.so

cs354heap.o: cs354heap.c cs354heap.h
	gcc -g -c -Wall -m32 -fpic $<

libheap.so: cs354heap.o
	gcc -shared -Wall -m32 -o $@ $<

# Define `make $TESTNAME.run`. This phony target runs the test and pipes its
# output into a file with the .output extension. It has a dependency on the
# actual test binary itself, which causes the test program to be built if it's
# not already built.
$(RUNTESTS_TARGETS): %.run: tests/%
	@echo =================================================================
	@echo == $@ ==
	@LD_LIBRARY_PATH=$(pwd):$(LD_LIBRARY_PATH) sh -c '$< 2>&1 && echo PASS || echo FAIL' | tee $<.output
	@echo -----------------------------------------------------------------

# Defines a how to build the test programs -- call make in the tests/
# directory. This also forces our heap library to be built if it's not already.
$(ALL_TESTS_BINARIES): %: %.c libheap.so
	make -C tests $(patsubst tests/%, %, $@)

# Running make clean deletes any compiled artifacts so that we only have our
# code left.
clean:
	rm -f *.o *.so
	make -C tests clean
