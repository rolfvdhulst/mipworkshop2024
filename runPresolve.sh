#!/bin/bash
instance="$PWD/data/instances/$1.mps.gz"
presolved="$PWD/data/presolved/$1.mps.gz"
postsolveDir="$PWD/data/output/"
./presolve.sh $instance $presolved $postsolveDir