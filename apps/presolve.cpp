//
// Created by rolf on 1-8-23.
//

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <mipworkshop2024/Shared.h>
#include <mipworkshop2024/IO.h>
#include <mipworkshop2024/Problem.h>


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
  if(!std::filesystem::exists(fileName)){
    std::cerr<<"Input file: "<<fileName<<" does not exist!\n";
    return EXIT_FAILURE;
  }
  if(!std::filesystem::exists(postSolveDirectory)){
    std::cerr<<"Output directory does not yet exist, creating folder at: "<< postSolveDirectory <<"\n";
    if(!std::filesystem::create_directory(postSolveDirectory)){
      std::cerr<<"Could not create output directory!\n";
      return EXIT_FAILURE;
    }
  }
  auto path = std::filesystem::path(fileName);
  auto problem = readMPSFile(path);
  if(!problem.has_value()){
    std::cerr<<"Error during reading file: "<<path<<"\n";
    return EXIT_FAILURE;
  }
  printInstanceString(path);

  //TODO: presolving methods to modify the problem and produce some post solving data

  if(!writeMPSFile(problem.value(),presolvedFileName)){
    std::cerr<<"Could not write to: "<<presolvedFileName<<"\n";
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}