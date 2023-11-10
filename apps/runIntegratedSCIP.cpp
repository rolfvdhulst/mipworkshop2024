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

struct Configuration{
    std::string name;
    std::optional<TUSettings> settings;
};
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

bool processProblem(const Problem& problem, const std::filesystem::path& path, const Configuration& config){
    double totalTimeLimit = 3600.0;
    ProblemLogData logData;
    if(!config.settings.has_value()){
        //baseline config
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
            solPath += "_";
            solPath += config.name;
            solPath += ".sol";
            std::ofstream logFile(solPath);
            if(!solToStream(normalSCIP->solution, logFile)){
                std::cout << "Could not write solution!\n";
                return false;
            }
        }
        logData.solveStatistics = normalSCIP->statistics;
        logData.solveStatistics->numTUImpliedColumns = 0;
        logData.solveStatistics->TUDetectionTime = 0.0;
        logData.numUpgraded = 0;
        logData.numDowngraded = 0;
        logData.writeType = VariableType::IMPLIED_INTEGER;
        logData.doDownGrade = false;
    }else{
        auto presolveStart = std::chrono::high_resolution_clock::now();
        Presolver presolver;
        presolver.doPresolve(problem, config.settings.value());
        auto presolvedProblem = presolver.presolvedProblem();
        std::size_t numFoundColumns = 0;
        for(const auto& reduction : presolver.postSolveStack().reductions){
            numFoundColumns += reduction.submatColumns.size();
        }
        auto presolveEnd = std::chrono::high_resolution_clock::now();
        double presolveTime = std::chrono::duration<double>(presolveEnd - presolveStart).count();
        std::cout<<"Presolving took: "<<presolveTime<<" seconds\n";
        double reducedTimeLimit = std::max(totalTimeLimit - presolveTime, 1.0);

        logData.writeType = config.settings->writeType;
        logData.doDownGrade = config.settings->doDowngrade;
        if(presolver.postSolveStack().totallyUnimodularColumnSubmatrixFound()){
            auto result = solveProblemSCIP(presolvedProblem,reducedTimeLimit);
            if(!result.has_value()){
                std::cerr<<"Error during solving problem!\n";
                return false;
            }
            result->statistics.TUDetectionTime = presolveTime;
            result->statistics.timeTaken += presolveTime;
            result->statistics.numTUImpliedColumns = 0;
            logData.numUpgraded = presolver.numUpgraded;
            logData.numDowngraded = presolver.numDowngraded;


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
                solPath += "_";
                solPath += config.name;
                solPath += ".sol";
                std::ofstream logFile(solPath);
                if(!solToStream(convertedSoLExternal, logFile)){
                    std::cout << "Could not write solution!\n";
                    return false;
                }
            }
            logData.solveStatistics = result->statistics;
        }
    }

    {
        auto logPath = path;
        logPath += "/integratedOutput/";
        logPath += problem.name;
        logPath += "_";
        logPath += config.name;
        logPath += ".json";
        std::ofstream logFile(logPath);
        logFile << logData.toJson();
        logFile.flush();
    }

    return true;
}
int main(int argc, char **argv) {
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() != 3) {
        std::cerr << "Wrong number of arguments!\n";
        return EXIT_FAILURE;
    }
    auto problem = readProblem(args[1]);
    if (!problem.has_value()) {
        return EXIT_FAILURE;
    }
	std::filesystem::path path(args[2]);

    std::vector<Configuration> configs = {
            Configuration{
                    .name = "b",
                    .settings = std::nullopt
            },
            Configuration{
                    .name = "c",
                    .settings = TUSettings{
                            .doDowngrade = true,
                            .writeType = VariableType::IMPLIED_INTEGER
                    }
            },
            Configuration{
                .name = "d",
                .settings = TUSettings{
                    .doDowngrade = false,
                    .writeType = VariableType::IMPLIED_INTEGER
                }
            },
            Configuration{
                .name = "e",
                .settings = TUSettings{
                    .doDowngrade = true,
                    .writeType = VariableType::CONTINUOUS
                }
            },
            Configuration{
                .name = "f",
                .settings = TUSettings{
                    .doDowngrade = false,
                    .writeType = VariableType::INTEGER
                }
            }
    };

    std::cout<<"Problem: "<<problem->name<<"\n";
    bool good = true;
    for(const auto& config : configs){
        if(!processProblem(problem.value(),path,config)){
            good = false;
        }
    }
	return good ? EXIT_SUCCESS : EXIT_FAILURE;

}