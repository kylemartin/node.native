# Calling 'make' without first calling 'configure' will configure and build for release
# Calling 'make' after calling 'configure' will use configured settings

# The default compilers if not set in environment or through configuration
-include settings.mk

# Get previous configuration settings if available, redefining CC and CXX if set
-include config.mk

# By default build for release
export BUILDTYPE ?= Release
PYTHON ?= python

all: native

native: out/Makefile
	$(MAKE) -C out

out/Makefile: config.gypi 
	./tools/gyp_native -f make test/test.gyp

GYPFILES = native.gyp common.gypi deps/uv/uv.gyp deps/http-parser/http_parser.gyp test/test.gyp

# If configure not already called or dependent files changed run configure
# This will reuse previous BUILDTYPE from config.mk
config.gypi: configure $(GYPFILES)
ifeq ($(BUILDTYPE),Debug)
	./configure --debug
else
	./configure
endif

clean:
	-rm -rf out/Makefile 
	-find out/ -name '*.o' -o -name '*.a' | xargs rm -rf

distclean:
	-rm -rf out
	-rm -f config.gypi
	-rm -f config.mk

all_tests := http

# ifeq ($(BUILDTYPE),Release)
# test: native
# 	@tools/test-wrapper-gypbuild.py -j16 --outdir=out --mode=release --arch=x64
# else
# test: native_g
# 	@tools/test-wrapper-gypbuild.py -j16 --outdir=out --mode=debug --arch=x64
# endif

test: native
	$(PYTHON) tools/test.py --mode=$(BUILDTYPE) $(all_tests)

# The .PHONY is needed to ensure that we recursively use the out/Makefile
# to check for changes.

.PHONY: native clean distclean all test build_tests
