#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(Buffer) {
	Buffer buf1("test");

	CHECK(buf1.str().compare("test") == 0);

	std::string lorem = COMMON_LOREM;

	Buffer buf2(lorem);

	CHECK(buf2.str().compare(lorem) == 0);

	buf2.append(buf1);

	CHECK(buf2.str().compare(lorem + "test") == 0);
}
