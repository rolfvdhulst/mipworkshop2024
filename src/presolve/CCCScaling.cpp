//
// Created by rolf on 27-9-23.
//

#include <iostream>
#include "mipworkshop2024/presolve/CCCScaling.hpp"

MatrixComponentsScaling::MatrixComponentsScaling(std::size_t rows, std::size_t cols)
        : components(rows, cols),
          rowScaling(rows, 1.0), colScaling(cols, 1.0) {

}

MatrixComponents::MatrixComponents(std::size_t rows, std::size_t cols) :
        rowComponent(rows, -1),
        colComponent(cols, -1) {

}

bool checkProblemScaling(const Problem &problem, const SparseMatrix& transposed, const Component &component) {
    for (const auto &row: component.rows) {
        if (!(isInfinite(-problem.lhs[row]) || isFeasIntegral(problem.lhs[row]))) {
            return false;
        }
        if (!(isInfinite(problem.rhs[row]) || isFeasIntegral(problem.rhs[row]))) {
            return false;
        }
    }
    for (const auto &col: component.cols) {
        if (!(isInfinite(-problem.lb[col]) || isFeasIntegral(problem.lb[col]))) {
            return false;
        }
        if (!(isInfinite(problem.ub[col]) || isFeasIntegral(problem.ub[col]))) {
            return false;
        }
    }

    for (index_t row: component.rows) {
        for (const auto &nonzero: transposed.getPrimaryVector(row)) {
            if (problem.colType[nonzero.index()] == VariableType::CONTINUOUS) {
                if (!isFeasEq(fabs(nonzero.value()), 1.0)) {
                    return false;
                }
            } else {
                if (!isFeasIntegral(nonzero.value())) {
                    return false;
                }
            }
        }
    }
    return true;
}

template<typename T>
std::optional<double> calculateIntegralRowScalar(const MatrixSlice<T> &rowSlice,
                                                 double rowLhs, double rowRhs,
                                                 const Problem &problem) {
    std::vector<double> scalars;
    for (const Nonzero &nonzero: rowSlice) {
        if (problem.colType[nonzero.index()] == VariableType::CONTINUOUS) continue;
        scalars.push_back(nonzero.value());
    }
    if (rowLhs != -infinity) {
        scalars.push_back(rowLhs);
    }
    if (rowRhs != infinity) {
        scalars.push_back(rowRhs);
    }

    auto result = calculateIntegralScalar(scalars, -1e-10, 1e-10, 100'000'000, 1e4);
    return result;

}

MatrixComponentsScaling computeCCCScaling(const Problem &problem) {
    SparseMatrix rowMatrix = problem.matrix.transposedFormat();
    index_t numRows = problem.numRows();
    index_t numCols = problem.numCols();

    std::vector<bool> rowScaledForIntegrality;
    std::vector<double> rowIntegralityMultiplier;
    for (index_t row = 0; row < numRows; ++row) {
        auto scalar = calculateIntegralRowScalar(rowMatrix.getPrimaryVector(row), problem.lhs[row], problem.rhs[row],
                                                 problem);
        rowIntegralityMultiplier.push_back(scalar.value_or(1.0));
        rowScaledForIntegrality.push_back(scalar.has_value());
    }


    MatrixComponentsScaling mcs(numRows, numCols);
    long currentComponent = 0;

    auto &colComponent = mcs.components.colComponent;
    auto &rowComponent = mcs.components.rowComponent;
    auto &colScale = mcs.colScaling;
    auto &rowScale = mcs.rowScaling;

    std::vector<index_t> dfsStack;
    for (std::size_t i = 0; i < numCols; ++i) {
        if (problem.colType[i] != VariableType::CONTINUOUS || mcs.components.colComponent[i] >= 0) continue;
        Component component;

        dfsStack.clear();
        dfsStack.push_back(numRows + i);
        colComponent[i] = currentComponent;
        colScale[i] = 1.0;

        bool isScalable = true;

        while (!dfsStack.empty()) {
            index_t index = dfsStack.back();
            dfsStack.pop_back();
            if (index >= numRows) {
                index_t column = index - numRows;
                component.cols.push_back(column);

                double searchColumnScale = colScale[column];

                for (const Nonzero &nonzero: problem.matrix.getPrimaryVector(column)) {
                    assert(rowComponent[nonzero.index()] < 0
                           || rowComponent[nonzero.index()] == currentComponent);
                    double nonzeroValue = nonzero.value() * rowIntegralityMultiplier[nonzero.index()];
                    if (rowComponent[nonzero.index()] < 0) {
                        dfsStack.push_back(nonzero.index());
                        rowComponent[nonzero.index()] = currentComponent;
                        rowScale[nonzero.index()] = 1.0 / (fabs(nonzeroValue) * searchColumnScale);
                    } else {
                        //Row scale was already determined earlier. Check if with nonzero it is (close enough to) 1/-1
                        if (fabs(fabs(rowScale[nonzero.index()] * nonzeroValue * searchColumnScale) - 1.0) > 1e-10) {
                            isScalable = false;
                        }
                    }
                }

            } else {
                component.rows.push_back(index);

                double searchRowScale = rowScale[index];

                if (!rowScaledForIntegrality[index]) {
                    isScalable = false;
                }

                for (const Nonzero &nonzero: rowMatrix.getPrimaryVector(index)) {
                    if (problem.colType[nonzero.index()] != VariableType::CONTINUOUS) continue;
                    assert(colComponent[nonzero.index()] < 0
                           || colComponent[nonzero.index()] == currentComponent);
                    double nonzeroValue = nonzero.value() * rowIntegralityMultiplier[index];

                    if (colComponent[nonzero.index()] < 0) {
                        dfsStack.push_back(problem.numRows() + nonzero.index());
                        colComponent[nonzero.index()] = currentComponent;
                        colScale[nonzero.index()] = 1.0 / (fabs(nonzeroValue) * searchRowScale);
                    } else {
                        if (fabs(fabs(colScale[nonzero.index()] * nonzeroValue * searchRowScale) - 1.0) > 1e-10) {
                            isScalable = false;
                        }
                    }
                }
            }
        }

        if (isScalable) {
            //Determine if we can find a single integral scalar a such that scaling the columns by
            //y/a gives us integral bounds
            std::vector<double> boundScalings;
            for (const auto &col: component.cols) {
                if (problem.lb[col] != 0.0 && problem.lb[col] != -infinity) {
                    boundScalings.push_back(problem.lb[col] / colScale[col]);
                }
                if (problem.ub[col] != 0.0 && problem.ub[col] != infinity) {
                    boundScalings.push_back(problem.ub[col] / colScale[col]);
                }
            }
            for(const auto& row : component.rows){
                boundScalings.push_back(rowScale[row]);
            }
            auto scalar = calculateIntegralScalar(boundScalings, -1e-10, 1e-10, 1'000'000, 1e8);

            if (scalar.has_value()) {
                double globalScale = scalar.value();
                for (const auto &row: component.rows) {
                    mcs.rowScaling[row] *= rowIntegralityMultiplier[row] * globalScale;
                }
                for (const auto &col: component.cols) {
                    mcs.colScaling[col] /= globalScale;
                }
            } else {
                isScalable = false;
            }
        }


        if (!isScalable) {
            for (const auto &row: component.rows) {
                mcs.rowScaling[row] = 1.0;
            }
            for (const auto &col: component.cols) {
                mcs.colScaling[col] = 1.0;
            }
        }
        mcs.components.contComponents.push_back(component);
        mcs.scalableComponents.push_back(isScalable);
        ++currentComponent;
    }


    std::size_t numScaled = 0;
    std::size_t numTotal = 0;

    std::size_t numRowCandidates = 0;
    std::size_t numColCandidates = 0;
    std::size_t numScaledRowCandidates = 0;
    std::size_t numScaledColCandidates = 0;
    for (std::size_t i = 0; i < mcs.components.contComponents.size(); ++i) {
        if (!mcs.scalableComponents[i]) continue;
        ++numTotal;
        bool scaled = false;
        const auto &component = mcs.components.contComponents[i];
        numRowCandidates += component.rows.size();
        numColCandidates += component.cols.size();

        for (const auto &row: component.rows) {
            if (rowScale[row] != 1.0) {
                scaled = true;
                break;
            }
        }
        if (!scaled) {
            for (const auto &col: component.cols) {
                if (colScale[col] != 1.0) {
                    scaled = true;
                    break;
                }
            }
        }
        if (scaled) {
            ++numScaled;

            double maxScalar = std::numeric_limits<double>::min();
            double minScalar = std::numeric_limits<double>::max();
            for (const auto &row: component.rows) {
                double scale = rowScale[row];
                if (scale < minScalar) {
                    minScalar = scale;
                }
                if (scale > maxScalar) {
                    maxScalar = scale;
                }
            }
            for (const auto &col: component.cols) {
                double scale = colScale[col];
                if (scale < minScalar) {
                    minScalar = scale;
                }
                if (scale > maxScalar) {
                    maxScalar = scale;
                }
            }
            numScaledColCandidates += component.cols.size();
            numScaledRowCandidates += component.rows.size();
            if (mcs.components.contComponents.size() < 200) {
                std::cout << "Min scalar: " << minScalar << " max scalar: " << maxScalar << std::endl;
            }
        }
    }

    std::cout << "Scaled: " << numScaled << " / " << numTotal << " valid components, "
              << mcs.components.contComponents.size() << " total components, Rows: "<<numScaledRowCandidates<<" / "<<numRowCandidates
              <<", Cols: "<<numScaledColCandidates <<" / "<<numColCandidates<<"\n";
    Problem scaledProblem = problem;
    scaledProblem.scale(rowScale, colScale);
    SparseMatrix transposed = scaledProblem.matrix.transposedFormat();

    for (std::size_t i = 0; i < mcs.components.contComponents.size(); ++i) {
        if(!mcs.scalableComponents[i]) continue;
        const auto& component = mcs.components.contComponents[i];
        bool check = checkProblemScaling(scaledProblem,transposed, component);
        if (!check) {
            std::cout << "Fail\n";
        }
    }
    return mcs;
}

