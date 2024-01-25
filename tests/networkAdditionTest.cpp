//
// Created by rolf on 24-11-23.
//

#include <mipworkshop2024/presolve/NetworkColumnAddition.h>
#include <gtest/gtest.h>
#include "TestHelpers.h"



SPQR_ERROR runDecomposition(ColTestCase &testCase,
                            std::vector<std::vector<col_idx>> &column_cycles,
                            bool &isGood,
                            bool guaranteedNetwork) {
    SPQR *env = NULL;
    SPQR_CALL(spqrCreateEnvironment(&env));
    Decomposition *dec = NULL;
    SPQR_CALL(createDecomposition(env, &dec, testCase.rows, testCase.cols));
    NetworkColumnAddition *newCol = NULL;
    SPQR_CALL(createNetworkColumnAddition(env, &newCol));
    {
        std::vector<row_idx> rows;
        std::vector<double> values;
        bool isNetwork = true;
        for (std::size_t col = 0; col < testCase.cols; ++col) {
            rows.clear();
            values.clear();
            for(const auto& nonz : testCase.matrix[col]){
                rows.push_back(nonz.index);
                values.push_back(nonz.value);
            }
            SPQR_CALL(networkColumnAdditionCheck(dec, newCol, col, rows.data(),values.data(),rows.size()));
            if (networkColumnAdditionRemainsNetwork(newCol)) {
                SPQR_CALL(networkColumnAdditionAdd(dec, newCol));
            } else {
                isNetwork = false;
                break;
            }

            bool isMinimal = checkDecompositionMinimal(dec);
            EXPECT_TRUE(isMinimal); //Check that there are no series-series and bond-bond connections
            //TODO: fix
//            bool fundamental_cycles_good = true;
//            for (std::size_t check_col = 0; check_col <= col; ++check_col) {
//                bool correct = checkCorrectnessColumn(dec, check_col, testCase.matrix[check_col].data(),
//                                                      testCase.matrix[check_col].size(), column_storage.data());
//                EXPECT_TRUE(correct);
//                if (!correct) {
//                    fundamental_cycles_good = false;
//                    std::cout << "seed: " << testCase.seed << " has mismatching fundamental cycle for " << check_col
//                              << "\n";
//                }
//            }
//            if (!fundamental_cycles_good) {
//                isGood = false;
//                break;
//            }
        }

        if (!isNetwork) {
//            if (guaranteedNetwork) {
//                EXPECT_FALSE(true);
//            }
//            bool cmrSaysGraphic = CMRisGraphic(testCase);
//            EXPECT_FALSE(cmrSaysGraphic);
//            if (cmrSaysGraphic) {
//                std::cout << "seed: " << testCase.seed << " is graphic according to CMR!\n";
//            }
        }
    }
    freeNetworkColumnAddition(env, &newCol);
    freeDecomposition(&dec);
    SPQR_CALL(spqrFreeEnvironment(&env));
    return SPQR_OKAY;
}

void runTestCase(const TestCase &testCase, bool guaranteedGraphic = false) {
    ASSERT_EQ(testCase.rows, testCase.matrix.size());
    ColTestCase colTestCase(testCase);

    std::vector<std::vector<col_idx>> column_cycles(testCase.cols);
    bool isGood = true;

    SPQR_ERROR error = runDecomposition(colTestCase, column_cycles, isGood, guaranteedGraphic);
    EXPECT_EQ(error, SPQR_OKAY);
    EXPECT_TRUE(isGood);
}
namespace NetworkColAdditionTest{

    TEST(NetworkColAddition, SingleColumn){
        auto testCase = stringToTestCase(    "+1 -1 "
                                             "+1  0 "
                                             "-1 +1 ",
                                             3,2);

    }
}