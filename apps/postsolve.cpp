//
// Created by rolf on 1-8-23.
//

#include <iostream>
#include <string>
#include <vector>
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
  if(!std::filesystem::exists(fileName) || !std::filesystem::is_regular_file(fileName)){
    std::cerr<<"Input file: "<<fileName<<" does not exist!\n";
    return EXIT_FAILURE;
  }
  if(!std::filesystem::exists(presolvedFileName) || !std::filesystem::is_regular_file(fileName)){
    std::cerr<<"Presolved file "<<presolvedFileName<<" does not exist!\n";
    return EXIT_FAILURE;
  }
  if(!std::filesystem::exists(postSolveDirectory)){
    std::cerr<<"Output directory at: "<< postSolveDirectory<< " does not exist!\n";
    return EXIT_FAILURE;
  }
  if(!std::filesystem::exists(presolvedSolution)){
    std::cerr<<"Presolved solution at: "<<presolvedSolution << " does not exist!\n";
    return EXIT_FAILURE;
  }
  if(!std::filesystem::exists(postSolvedSolution)){
    std::cerr<<"Postsolved solution at: "<<postSolvedSolution << " does not exist!\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
