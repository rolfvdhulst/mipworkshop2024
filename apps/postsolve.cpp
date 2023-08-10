//
// Created by rolf on 1-8-23.
//

#include <iostream>
#include <string>
#include <vector>
#include <mipworkshop2024/ApplicationShared.h>
#include <filesystem>
int main(int argc, char** argv){
  std::vector<std::string> args(argv,argv+argc);
  if(args.size() != 6)
  {
    std::cerr<<"Not enough paths specified!\n";
    return EXIT_FAILURE;
  }
  const std::string& fileName = args[1];
  const std::string& presolvedFileName = args[2];
  const std::string& postSolveDirectory = args[3];
  const std::string& presolvedSolution = args[4];
  const std::string& postSolvedSolution = args[5];

  if(doPostsolve(fileName,presolvedFileName,postSolveDirectory,presolvedSolution,postSolvedSolution)){
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
