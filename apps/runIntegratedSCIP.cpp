//
// Created by rolf on 7-8-23.
//

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <mipworkshop2024/IO.h>
#include <mipworkshop2024/Solve.h>
#include <mipworkshop2024/presolve/Presolver.h>

int main(int argc, char** argv){
  std::vector<std::string> args(argv,argv+argc);

  if(args.size() != 2){
    std::cerr<<"Wrong number of arguments!\n";
    return EXIT_FAILURE;
  }
  const std::string& fileName = args[1];
  std::filesystem::path filePath(fileName);
  auto dataDirectory = filePath.parent_path().parent_path();

  if(!std::filesystem::exists(fileName) || !std::filesystem::is_regular_file(fileName)){
    std::cerr<<"Input file: "<<fileName<<" does not exist!\n";
    return EXIT_FAILURE;
  }
  auto problem = readMPSFile(fileName);
  if(!problem.has_value()){
    std::cerr<<"Could not read MPS file: "<<fileName<<"\n";
    return EXIT_FAILURE;
  }
  double totalTimeLimit = 600.0;
  auto presolveStart = std::chrono::high_resolution_clock::now();
  Presolver presolver(problem.value());
  presolver.doPresolve();
  auto presolvedProblem = presolver.presolvedProblem();
  auto presolveEnd = std::chrono::high_resolution_clock::now();

  double presolveTime = std::chrono::duration<double>(presolveEnd-presolveStart).count();
  double reducedTimeLimit = std::max(totalTimeLimit-presolveTime,1.0);
  auto result = solveProblemSCIP(presolvedProblem,reducedTimeLimit);
  if(!result.has_value()){
    std::cerr<<"Error during solving problem!\n";
    return EXIT_FAILURE;
  }
  result->statistics.timeTaken += presolveTime;

  auto postSolvedSolution = result->solution;

  auto integratedPath = dataDirectory;
  integratedPath += "/integratedSolutions/";
  integratedPath += problem->name;
  integratedPath += ".sol";

  std::ofstream solFile(integratedPath);
  if(!solToStream(postSolvedSolution,solFile)){
    std::cerr<<"Could not write solution to: "<<integratedPath<<"\n";
    return EXIT_FAILURE;
  }

  auto normalSCIP = solveProblemSCIP(problem.value(),totalTimeLimit);
  return EXIT_SUCCESS;
}