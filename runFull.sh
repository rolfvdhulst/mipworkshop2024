#!/bin/bash
instance="$PWD/data/instances/$1.mps.gz"
presolved="$PWD/data/presolved/$1.mps.gz"
postsolveDir="$PWD/data/output/"
scipSolution="$PWD/data/presolveSolutions/$1.sol"
postSolveSolution="$PWD/data/postsolveSolutions/$1.sol"
baseLineSolution="$PWD/data/baselineSolutions/$1.sol"
./presolve.sh $instance $presolved $postsolveDir
./build/apps/runSCIP $presolved $scipSolution
./postsolve.sh $instance $presolved $postsolveDir $scipSolution $postSolveSolution