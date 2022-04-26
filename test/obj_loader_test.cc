#include "../src/loaders/obj_loader.h"

#include <glog/logging.h>
#include <gtest/gtest.h>

TEST(ObjLoaderTest, LoadLightBulb) {
  ObjLoader loader("../assets/light_bulb/light_bulb.obj");
  loader.Load();
  LOG(ERROR) << loader.object_3d_->children().size();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
