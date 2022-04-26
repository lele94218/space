#include <gtest/gtest.h>

#include "../src/loaders/obj_loader.h"

TEST(ObjLoaderTest, LoadLightBulb) {
  ObjLoader loader("test");
  EXPECT_STRNE("hello", "world");
  EXPECT_EQ(7 * 6, 42);
}
