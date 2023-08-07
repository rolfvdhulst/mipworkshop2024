//
// Created by rolf on 6-8-23.
//

#include "IncidenceAddition.h"
IncidenceAddition::IncidenceAddition(index_t numRows,
                                     index_t numCols,
                                     Submatrix::Initialization init,
                                     bool transposed) {
  incidenceSubmatrix = Submatrix(numRows,numCols,init);

}
bool isNegative(int index){
  return index < 0;
}
IncidenceAddition::UnionFindInfo IncidenceAddition::findDenseRepresentative(int index) {
  int current = index;
  int next;

  int totalSign = 1;
  while(!isNegative(next = unionFind[current].representative)){
    totalSign *= unionFind[current].edgeSign;
    current = next;
  }

  int root = current;
  current = index;

  int currentSign = totalSign;
  while(!isNegative(next = unionFind[current].representative)){
    unionFind[current].representative = root;
    int previousSign = unionFind[current].edgeSign;
    unionFind[current].edgeSign = currentSign;
    currentSign *= previousSign;
    current = next;
  }
  return UnionFindInfo{.representative = root,.edgeSign=totalSign};
}
