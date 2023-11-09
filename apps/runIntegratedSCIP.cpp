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
#include "mipworkshop2024/Logging.h"
#include "mipworkshop2024/presolve/CCCScaling.hpp"

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

bool processProblem(const Problem& problem, const std::filesystem::path& path){
	std::cout<<"Problem: "<<problem.name<<"\n";
    auto presolveStart = std::chrono::high_resolution_clock::now();
    Presolver presolver;
	presolver.doPresolve(problem);
    auto presolvedProblem = presolver.presolvedProblem();
    std::size_t numFoundColumns = 0;
    for(const auto& reduction : presolver.postSolveStack().reductions){
        numFoundColumns += reduction.submatColumns.size();
    }
//    auto result = computeCCCScaling(problem);
//    Problem scaledProblem = problem;
//    scaledProblem.scale(result.rowScaling,result.colScaling);
//    Presolver secondPresolver;
//    secondPresolver.doPresolve(scaledProblem);
//    auto secondPresolvedProblem = secondPresolver.postSolveStack();
//    std::size_t scaledFoundColumns = 0;
//    for(const auto& reduction : secondPresolver.postSolveStack().reductions){
//        scaledFoundColumns += reduction.submatColumns.size();
//    }
//    if(scaledFoundColumns != numFoundColumns){
//        std::cout<<"Non-scaled: "<<numFoundColumns<<", scaled: "<<scaledFoundColumns<<"\n";
//    }
    auto presolveEnd = std::chrono::high_resolution_clock::now();
    double presolveTime = std::chrono::duration<double>(presolveEnd - presolveStart).count();
	std::cout<<"Presolving took: "<<presolveTime<<" seconds\n";
    double totalTimeLimit = 200.0;
    double reducedTimeLimit = std::max(totalTimeLimit - presolveTime, 1.0);

    ProblemLogData logData;
    if(presolver.postSolveStack().totallyUnimodularColumnSubmatrixFound()){
        auto result = solveProblemSCIP(presolvedProblem,reducedTimeLimit);
        if(!result.has_value()){
            std::cerr<<"Error during solving problem!\n";
            return false;
        }
        result->statistics.TUDetectionTime = presolveTime;
        result->statistics.timeTaken += presolveTime;
        result->statistics.numTUImpliedColumns = 0;
        for(const auto& reduction : presolver.postSolveStack().reductions){
            result->statistics.numTUImpliedColumns += reduction.submatColumns.size();
        }

        if(result->statistics.numSolutions > 0 ){
            auto convertedSol = problem.convertExternalSolution(result->solution);
            if (!convertedSol.has_value())
            {
                std::cout << "Solution cannot be interpreted!\n";
                return false;
            }
            if(!problem.isFeasible(convertedSol.value())){
                std::cout<<"Recovering solution in postSolve\n";
                auto recovered = doPostSolve(problem,convertedSol.value(),presolver.postSolveStack());
                if(!recovered.has_value()){
                    return false;
                }
                if(!problem.isFeasible(recovered.value())){
                    std::cout<<"Did not recover solution!\n";
                    return false;
                }
                convertedSol = recovered;

                result->statistics.primalBound = problem.computeObjective(convertedSol.value());
                std::cout<<"Recovered solution with objective: "<<result->statistics.primalBound<<"\n";
            }
            auto convertedSoLExternal = problem.convertSolution(*convertedSol);
            auto solPath = path;
            solPath += "/integratedSolutions/";
            solPath += problem.name;
            solPath += "_t";
            solPath += ".sol";
            std::ofstream logFile(solPath);
            if(!solToStream(convertedSoLExternal, logFile)){
                std::cout << "Could not write solution!\n";
                return false;
            }
        }

        logData.presolvedStatistics = result->statistics;

    }else{
        logData.presolvedStatistics = logData.baseLineStatistics;
    }

	auto normalSCIP = solveProblemSCIP(problem,totalTimeLimit);

	if(!normalSCIP.has_value()){
		std::cerr<<"Error during solving problem!\n";
		return false;
	}
	if(normalSCIP->statistics.numSolutions > 0){
		auto convertedSol = problem.convertExternalSolution(normalSCIP->solution);
		if (!convertedSol.has_value())
		{
			std::cout << "Solution cannot be interpreted!\n";
			return false;
		}
		if (!problem.isFeasible(convertedSol.value()))
		{
			std::cout << "Solution is not feasible!\n";
			return false;
		}
		auto solPath = path;
		solPath += "/integratedSolutions/";
		solPath += problem.name;
		solPath += "_b";
		solPath += ".sol";
		std::ofstream logFile(solPath);
		if(!solToStream(normalSCIP->solution, logFile)){
			std::cout << "Could not write solution!\n";
			return false;
		}
	}
	logData.baseLineStatistics = normalSCIP->statistics;
	logData.baseLineStatistics->numTUImpliedColumns = 0;


	{
		auto logPath = path;
		logPath += "/integratedOutput/";
		logPath += problem.name;
		logPath += ".json";
		std::ofstream logFile(logPath);
		logFile << logData.toJson();
		logFile.flush();
	}

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
	std::filesystem::path path(args[1]);
	path = path.parent_path().parent_path();

    if(!processProblem(problem.value(),path)){
        return EXIT_FAILURE;
    }
	return EXIT_SUCCESS;

}