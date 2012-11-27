#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(Buffer) {
	Buffer buf("test");

	CHECK(std::string(buf.base(),buf.size()).compare("test") == 0);
}
