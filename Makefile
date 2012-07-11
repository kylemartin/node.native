-include config.mk

BUILDTYPE ?= Release
PYTHON ?= python

# BUILDTYPE=Debug builds both release and debug builds. If you want to compile
# just the debug build, run `make -C out BUILDTYPE=Debug` instead.

all: out/Makefile native

native: config.gypi
	$(MAKE) -C out BUILDTYPE=Release

config.gypi: configure
	./configure

out/Makefile: native.gyp config.gypi common.gypi deps/uv/uv.gyp deps/http-parser/http_parser.gyp 
	./tools/gyp_native -f make

clean:
	-rm -rf out/Makefile 
	-find out/ -name '*.o' -o -name '*.a' | xargs rm -rf

distclean:
	-rm -rf out
	-rm -f config.gypi
	-rm -f config.mk

# The .PHONY is needed to ensure that we recursively use the out/Makefile
# to check for changes.

.PHONY: native clean distclean all
