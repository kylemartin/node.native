#include "cctest.h"

#include "native.h"

TEST(HelloWorld) {
  printf("hello world!\n");
  int hello_world = 0;
  CHECK_EQ(0, hello_world);
}
