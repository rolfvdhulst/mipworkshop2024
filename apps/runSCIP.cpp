//
// Created by rolf on 7-8-23.
//
#include <iostream>
#include <string>
#include <vector>
#include "mipworkshop2024/ApplicationShared.h"

int main(int argc,char** argv)
{
  std::vector<std::string> input(argv,argv+argc);

  if(input.size() != 3){
    std::cerr<<"Wrong number of arguments!\n";
    return EXIT_FAILURE;
  }
  const std::string& problemPath = input[1];
  const std::string& writeSolutionPath = input[2];
  if(runSCIPSeparate(problemPath,writeSolutionPath)){
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
