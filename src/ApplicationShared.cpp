//
// Created by rolf on 6-8-23.
//
#include <iostream>
#include <filesystem>
#include <cassert>
#include "mipworkshop2024/IO.h"
#include "mipworkshop2024/ApplicationShared.h"
#include "mipworkshop2024/presolve/Presolver.h"
#include <mipworkshop2024/Solve.h>

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

void printInstanceString(const std::string &fullPath) {
    std::cout << "[INSTANCE] " << std::filesystem::path(fullPath).filename().string() << "\n";
}

//TODO: fix
std::chrono::high_resolution_clock::time_point printStartString() {
    time_t now = time(nullptr);
    struct tm *tm = localtime(&now);
    auto time = std::chrono::high_resolution_clock::now();
    auto sinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());
    std::size_t milliSeconds = sinceEpoch.count() % 1000;
    char s[64];
    size_t ret = strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%S", tm);
    assert(ret);
    std::cout << "[START] " <<  s << "."<< std::setw(3) << std::setfill('0')<< milliSeconds << "\n";
    return time;
}

std::chrono::high_resolution_clock::time_point printEndString() {

    time_t now = time(nullptr);
    struct tm *tm = localtime(&now);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto sinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(endTime.time_since_epoch());
    std::size_t milliSeconds = sinceEpoch.count() % 1000;
    char s[64];
    size_t ret = strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%S", tm);
    assert(ret);
    std::cout << "[END] " << s << "."<< std::setw(3) << std::setfill('0')<< milliSeconds<<"\n";
    return endTime;
}

bool doPresolve(const std::string &problemPath,
                const std::string &presolvedProblemPath,
                const std::string &outputPath) {
    if (!std::filesystem::exists(problemPath)) {
        std::cerr << "Input file: " << problemPath << " does not exist!\n";
        return false;
    }
    if (!std::filesystem::exists(outputPath)) {
        std::cerr << "Output directory does not yet exist, creating folder at: " << outputPath << "\n";
        if (!std::filesystem::create_directory(outputPath)) {
            std::cerr << "Could not create output directory!\n";
            return false;
        }
    }
    auto path = std::filesystem::path(problemPath);
    auto start = std::chrono::high_resolution_clock::now();
    auto problemOpt = readMPSFile(path);
    auto end = std::chrono::high_resolution_clock::now();
    if (!problemOpt.has_value()) {
        std::cerr << "Error during reading file: " << path << "\n";
        return false;
    }
    auto problem = problemOpt.value();
    printInstanceString(path);
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms to read problem\n";


    auto compStart = printStartString();
    Presolver presolver;
    presolver.doPresolve(problem, TUSettings{
            .doDowngrade = false,
            .writeType = VariableType::INTEGER,
            .dynamic = false
    });
    auto compEnd = printEndString();

    {
        start = std::chrono::high_resolution_clock::now();
        if (!writeMPSFile(presolver.presolvedProblem(), presolvedProblemPath)) {
            std::cerr << "Could not write to: " << presolvedProblemPath << "\n";
            return false;
        }
        end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << " ms to write presolved problem to "<<presolvedProblemPath << "\n";
    }
    {
        std::string postsolvepath = outputPath + problem.name + ".postsolve";
        start = std::chrono::high_resolution_clock::now();
        if (!writePostSolveStackFile(presolver.postSolveStack(),postsolvepath)) {
            std::cerr << "Could not write to: " << postsolvepath << "\n";
            return false;
        }
        end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                  << " ms to write postsolve stack to "<< postsolvepath << "\n";
    }


    return true;
}

bool doPostsolve(const std::string &problemPath,
                 const std::string &presolvedProblemPath,
                 const std::string &outputPath,
                 const std::string &presolvedSolution,
                 const std::string &postsolvedSolution) {

    if (!std::filesystem::exists(problemPath) || !std::filesystem::is_regular_file(problemPath)) {
        std::cerr << "Input file: " << problemPath << " does not exist!\n";
        return false;
    }
    if (!std::filesystem::exists(presolvedProblemPath) || !std::filesystem::is_regular_file(presolvedProblemPath)) {
        std::cerr << "Presolved file " << presolvedProblemPath << " does not exist!\n";
        return false;
    }
    if (!std::filesystem::exists(outputPath)) {
        std::cerr << "Output directory at: " << outputPath << " does not exist!\n";
        return false;
    }
    if (!std::filesystem::exists(presolvedSolution)) {
        std::cerr << "Presolved solution at: " << presolvedSolution << " does not exist!\n";
        return false;
    }
    printInstanceString(problemPath);

    auto problem = readMPSFile(problemPath);
    if(!problem.has_value()){
        std::cerr<<"Could not read original problem : "<< problemPath<<"\n";
        return false;
    }
    auto solution = readSolFile(presolvedSolution);
    if (!solution.has_value()) {
        std::cerr << "Error during reading of solution file: " << presolvedSolution << "\n";
        return false;
    }

    std::string postSolvePath =  outputPath + problem->name + ".postsolve";
    if(!exists(std::filesystem::path(postSolvePath))){
        std::cerr << "Postsolve file  at: " << postSolvePath << " does not exist!\n";
        return false;
    }

    auto postSolveStream = std::ifstream(postSolvePath);
    auto postSolveStack = postSolveStackFromStream(postSolveStream);
    if(!postSolveStack.has_value()){
        std::cerr << "Error reading postsolve file at: " << postSolvePath << "\n";
        return false;
    }
    auto convertedSol = problem->convertExternalSolution(solution.value());
    if(!convertedSol.has_value()){
        std::cerr<<"Could not convert solution!\n";
        return false;
    }

    auto start = printStartString();
    ExternalSolution processedSolution;
    if (problem->isFeasible(convertedSol.value())){ //No need to do anything complicated if solution is already feasible
        processedSolution = solution.value();
    }else{
        auto recovered = doPostSolve(problem.value(),convertedSol.value(),postSolveStack.value());
        if(recovered.has_value()){
            processedSolution = problem->convertSolution(recovered.value());
        }else{
            std::cerr<<"Could not convert solution!\n";
            return false;
        }
    }
    auto end = printEndString();

    {
        auto test = problem->convertExternalSolution(processedSolution);
        if(!test.has_value() || !problem->isFeasible(test.value())){
            std::cerr<<"recovered solution is still not valid... still writing in case tolerances are different\n";}
    }

    std::ofstream outStream(postsolvedSolution);
    if (!outStream.is_open()) {
        std::cerr << "Could not open solution file at: " << postsolvedSolution << "\n";
        return false;
    }
    start = std::chrono::high_resolution_clock::now();
    if (!solToStream(processedSolution, outStream)) {
        std::cerr << "Error during writing postsolved solution: " << postsolvedSolution << "\n";
        return false;
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Wrote solution to: " << postsolvedSolution << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";

    return true;
}

static
SCIP_RETCODE runSCIP(const std::string &problemPath, const std::string &solutionPath) {
    SCIP *scip = NULL;

    /*********
     * Setup *
     *********/

    /* initialize SCIP */
    SCIP_CALL(SCIPcreate(&scip));

    /* include default SCIP plugins */
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    SCIPprintVersion(scip,stdout);

    SCIP_CALL(SCIPreadProb(scip, problemPath.c_str(), NULL));

    SCIP_CALL(SCIPsetRealParam(scip, "limits/time", 100.0));
    SCIP_CALL(SCIPsolve(scip));

    if(SCIPgetNSols(scip) > 0){
        ExternalSolution solution;
        SCIP_SOL *sol = SCIPgetBestSol(scip);
        solution.objectiveValue = SCIPsolGetOrigObj(sol);
        SCIP_VAR **vars = SCIPgetOrigVars(scip);
        int nVars = SCIPgetNOrigVars(scip);
        for (int i = 0; i < nVars; ++i) {
            const char *name = SCIPvarGetName(vars[i]);
            double value = SCIPgetSolVal(scip, sol, vars[i]);
            solution.variableValues[name] = value;
        }

        std::ofstream fileStream(solutionPath);
        if (!fileStream.is_open() || !fileStream.good()) {
            return SCIP_FILECREATEERROR;
        }
        if (!solToStream(solution, fileStream)) {
            return SCIP_FILECREATEERROR;
        }
    }

    /********************
     * Deinitialization *
     ********************/

    SCIP_CALL(SCIPfree(&scip));

    BMScheckEmptyMemory();

    return SCIP_OKAY;
}

bool runSCIPSeparate(const std::string &problemPath, const std::string &writeSolutionPath) {

    SCIP_RETCODE retcode;

    retcode = runSCIP(problemPath, writeSolutionPath);
    if (retcode != SCIP_OKAY) {
        SCIPprintError(retcode);
        return false;
    }

    return true;
}