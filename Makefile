# Calling 'make' without first calling 'configure' will configure and build for release
# Calling 'make' after calling 'configure' will use configured settings

# The default compilers if not set in environment or through configuration
-include build/settings.mk

# Get previous configuration settings if available, redefining CC and CXX if set
-include out/config.mk

# By default build for release
export BUILDTYPE ?= Release
PYTHON ?= python

all: native

native: out/Makefile out/config.gypi
	$(MAKE) -C out

out/Makefile: out/config.gypi
	build/gyp_native -f make

# out/Makefile: config.gypi 
# 	./build/gyp_native -f make

GYPFILES = src/native.gyp examples/examples.gyp \
	deps/uv/uv.gyp deps/http-parser/http_parser.gyp \
	tests/tests.gyp tests/http/http.gyp \
	build/common.gypi

# If configure not already called or dependent files changed run configure
# This will reuse previous BUILDTYPE from config.mk
out/config.gypi: build/configure $(GYPFILES)
ifeq ($(BUILDTYPE),Debug)
	./build/configure --debug
else
	./build/configure
endif

clean:
	-find out/ -name '*.o' -o -name '*.a' | xargs rm -rf

distclean:
	-rm -rf out

all_tests := http net native

# ifeq ($(BUILDTYPE),Release)
# test: native
# 	@tools/test-wrapper-gypbuild.py -j16 --outdir=out --mode=release --arch=x64
# else
# test: native_g
# 	@tools/test-wrapper-gypbuild.py -j16 --outdir=out --mode=debug --arch=x64
# endif

test: native
	$(PYTHON) tools/test.py --mode=$(BUILDTYPE) $(all_tests)

test-verbose: native
	$(PYTHON) tools/test.py --verbose --mode=$(BUILDTYPE) $(all_tests)

test-%: native
	$(PYTHON) tools/test.py --mode=$(BUILDTYPE) $*

# The .PHONY is needed to ensure that we recursively use the out/Makefile
# to check for changes.

.PHONY: native clean distclean all test
