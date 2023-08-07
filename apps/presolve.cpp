//
// Created by rolf on 1-8-23.
//

#include <iostream>
#include <string>
#include <vector>
#include <mipworkshop2024/ApplicationShared.h>

int main(int argc, char** argv) {
  std::vector<std::string> args(argv,argv+argc);
  if(args.size() != 4)
  {
    std::cerr<<"Not enough paths specified. Please specify an .mps.gz file, a filename to write the presolved model to and a folder for additional data!\n";
    return EXIT_FAILURE;
  }
  const std::string& fileName = args[1];
  const std::string& presolvedFileName = args[2];
  const std::string& postSolveDirectory = args[3];
  bool good = doPresolve(fileName,presolvedFileName,postSolveDirectory);
  if(!good){
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}