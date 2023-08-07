//
// Created by rolf on 1-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_IO_H
#define MIPWORKSHOP2024_SRC_IO_H

#include "Problem.h"
#include "ExternalSolution.h"
#include <optional>
#include <fstream>
#include <filesystem>

bool solToStream(const ExternalSolution& solution, std::ostream& stream);

std::optional<ExternalSolution> solFromStream(std::istream& stream);
std::optional<ExternalSolution> solFromCompressedStream(std::istream& stream);

std::optional<Problem> readMPSFile(const std::filesystem::path& path);
std::optional<Problem> problemFromMPSstream(std::istream& stream);
std::optional<Problem> problemFromCompressedMPSStream(std::istream& stream);


bool writeMPSFile(const Problem& problem,const std::filesystem::path& path);
bool problemToStream(const Problem& problem,std::ostream& stream);
bool problemToStreamCompressed(const Problem&, std::ostream& stream);
#endif //MIPWORKSHOP2024_SRC_IO_H
