#!/bin/bash

instanceFolder="$1"
softwareFolder="$2"
input="$PWD/data/first_round.test"

N=8
while IFS= read -r line
do
  ("$2"/build/apps/runIntegratedSCIP "$instanceFolder/$line.mps.gz" "$PWD/data/" > "$PWD/data/integratedOutput/$line.log" ;)&
  if [[ $(jobs -r -p | wc -l) -ge $N ]]; then
    wait -n
  fi
done < "$input"