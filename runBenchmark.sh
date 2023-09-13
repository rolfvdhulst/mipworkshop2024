#!/bin/bash
input="$PWD/data/first_round.test"

N=6
while IFS= read -r line
do
  (./build/apps/runIntegratedSCIP "/home/rolf/math/mipworkshop2024/data/instances/$line.mps.gz" > "/home/rolf/math/mipworkshop2024/data/integratedOutput/$line.txt";)&
  if [[ $(jobs -r -p | wc -l) -ge $N ]]; then
    wait -n
  fi
done < "$input"