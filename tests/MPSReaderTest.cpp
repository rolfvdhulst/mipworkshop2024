//
// Created by rolf on 2-8-23.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <mipworkshop2024/IO.h>

TEST(MPSReader,parseMany){
  //Test the MPS reader by reading many different files.
  //The files are taken from scips checking set.
  std::filesystem::directory_iterator iterator("/home/rolf/math/mipworkshop2024/data/instances/"); //TODO: remove hardcoding
  for(const auto& path : iterator){
    if(path.is_regular_file() &&
        (path.path().extension() == ".gz" || (path.path().extension() == ".gz" && path.path().stem().extension() == ".mps"))){
      std::cout<<"Reading "<<path<<std::endl;
      auto problem = readMPSFile(path.path());
      EXPECT_TRUE(problem.has_value());
    }
  }
}

TEST(MPSReader,parseDifficultBounds){
  auto problem = readMPSFile("/home/rolf/math/mipworkshop2024/data/instances/leo1.mps.gz");
  EXPECT_TRUE(problem.has_value());
}