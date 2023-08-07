//
// Created by rolf on 1-8-23.
//

#ifndef MIPWORKSHOP2024_SRC_EXTERNALSOLUTION_H
#define MIPWORKSHOP2024_SRC_EXTERNALSOLUTION_H

#include <unordered_map>
#include <string>
struct ExternalSolution {
  double objectiveValue;
  std::unordered_map<std::string,double> variableValues;

};



#endif //MIPWORKSHOP2024_SRC_EXTERNALSOLUTION_H
