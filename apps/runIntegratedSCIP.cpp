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

std::optional<Problem> readProblem(const std::string &path) {

    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        std::cerr << "Input file: " << path << " does not exist!\n";
        return std::nullopt;
    }
    auto problem = readMPSFile(path);
    if (!problem.has_value()) {
        std::cerr << "Could not read MPS file: " << path << "\n";
        return std::nullopt;
    }
    return problem;
}
bool processProblem(const Problem& problem){
    auto presolveStart = std::chrono::high_resolution_clock::now();
    Presolver presolver;
    presolver.doPresolve(problem);
    if(!presolver.postSolveStack().totallyUnimodularColumnSubmatrixFound()){
        std::cout<<"Did not find a column TU submatrix which implies integrality\n";
        return true;
    }
    auto presolvedProblem = presolver.presolvedProblem();
    auto presolveEnd = std::chrono::high_resolution_clock::now();
    double presolveTime = std::chrono::duration<double>(presolveEnd - presolveStart).count();

    double totalTimeLimit = 600.0;
    double reducedTimeLimit = std::max(totalTimeLimit - presolveTime, 1.0);
    return true;
}
int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() != 2) {
        std::cerr << "Wrong number of arguments!\n";
        return EXIT_FAILURE;
    }
    auto problem = readProblem(args[1]);
    if (!problem.has_value()) {
        return EXIT_FAILURE;
    }

    std::size_t n = 35;
    std::string indent(n-problem->name.size(),' ');
    std::cout<<problem->name<<indent<<": ";
    if(!processProblem(problem.value())){
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
//  auto result = solveProblemSCIP(presolvedProblem,reducedTimeLimit);
//  if(!result.has_value()){
//    std::cerr<<"Error during solving problem!\n";
//    return EXIT_FAILURE;
//  }
//  result->statistics.timeTaken += presolveTime;
//
//  auto postSolvedSolution = result->solution;
//
//  auto integratedPath = dataDirectory;
//  integratedPath += "/integratedSolutions/";
//  integratedPath += problem->name;
//  integratedPath += ".sol";
//
//  std::ofstream solFile(integratedPath);
//  if(!solToStream(postSolvedSolution,solFile)){
//    std::cerr<<"Could not write solution to: "<<integratedPath<<"\n";
//    return EXIT_FAILURE;
//  }
//
//  auto normalSCIP = solveProblemSCIP(problem.value(),totalTimeLimit);
//  return EXIT_SUCCESS;
}