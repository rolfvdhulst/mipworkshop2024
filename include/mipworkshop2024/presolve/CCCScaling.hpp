//
// Created by rolf on 27-9-23.
//

#ifndef MIPWORKSHOP2024_CONNECTEDCOMPONENTSCALING_HPP
#define MIPWORKSHOP2024_CONNECTEDCOMPONENTSCALING_HPP

#include <vector>
#include "mipworkshop2024/Problem.h"
struct Component{
    std::vector<std::size_t> rows;
    std::vector<std::size_t> cols;
};

struct MatrixComponents{
    MatrixComponents(std::size_t rows, std::size_t cols);
    std::vector<Component> contComponents;
    std::vector<long> rowComponent;
    std::vector<long> colComponent;
};

struct MatrixComponentsScaling{
    MatrixComponentsScaling(std::size_t rows, std::size_t cols);
    MatrixComponents components;
    std::vector<bool> scalableComponents;
    std::vector<double> rowScaling;
    std::vector<double> colScaling;
};

MatrixComponentsScaling computeCCCScaling(const Problem& problem);

#endif //MIPWORKSHOP2024_CONNECTEDCOMPONENTSCALING_HPP
