//
// Created by rolf on 19-08-22.
//

#ifndef NETWORKDETECTION_TESTHELPERS_H
#define NETWORKDETECTION_TESTHELPERS_H

#include "mipworkshop2024/presolve/NetworkDecomposition.h"
#include <random>
#include <fstream>
#include <unordered_set>
#include <algorithm>
#include <vector>


struct Nonzero {
    matrix_size index;
    double value;
};
class TestCase {
public:
    TestCase(std::vector<std::vector<Nonzero>> matrix, std::size_t rows, std::size_t cols, std::size_t seed = 0);
    std::size_t rows;
    std::size_t cols;
    std::vector<std::vector<Nonzero>> matrix;
};

class ColTestCase {
public:
    explicit ColTestCase(const TestCase& testCase);

    std::size_t rows;
    std::size_t cols;
    std::vector<std::vector<Nonzero>> matrix;
};

TestCase stringToTestCase(const std::string& string, uint64_t rows, uint64_t cols);

struct Edge{
    std::size_t head;
    std::size_t tail;

    bool operator == (const Edge& other) const;
};

int findRepresentative(int entry, std::vector<int>& representative);

int makeUnion(std::vector<int>& representative, int first, int second);

namespace std {

    template <>
    struct hash<Edge>
    {
        std::size_t operator()(const Edge& k) const
        {
            return (k.head << 32) + k.tail;
        }
    };

}

TestCase createErdosRenyiTestcase(std::size_t nodes, double density, std::size_t seed);


#endif //NETWORKDETECTION_TESTHELPERS_H
