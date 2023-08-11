#!/bin/bash
input="$PWD/data/first_round.test"
while IFS= read -r line
do
  ./build/apps/runIntegratedSCIP "/home/rolf/phd/data/benchmark/$line.mps.gz"
done < "$input"