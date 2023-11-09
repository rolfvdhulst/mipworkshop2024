//
// Created by rolf on 6-8-23.
//

#ifndef MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_APPLICATIONSHARED_H
#define MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_APPLICATIONSHARED_H

#include <string>
#include <chrono>


bool doPresolve(const std::string& problemPath,
                const std::string& presolvedProblemPath,
                const std::string& outputPath);

bool doPostsolve(const std::string& problemPath,
                const std::string& presolvedProblemPath,
                const std::string& outputPath,
                const std::string& presolvedSolution,
                const std::string& postsolvedSolution);

bool runSCIPSeparate(const std::string& problemPath, const std::string& writeSolutionPath);

#endif //MIPWORKSHOP2024_INCLUDE_MIPWORKSHOP2024_APPLICATIONSHARED_H
