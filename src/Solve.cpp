//
// Created by rolf on 7-8-23.
//

#include "mipworkshop2024/Solve.h"
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <scip/cons.h>

double convertValue(SCIP * scip, double value){
  if(value == -infinity){
    return -SCIPinfinity(scip);
  }else if (value == infinity){
    return SCIPinfinity(scip);
  }
  return value;
}
SCIP_RETCODE scipDoSolveProblem(const Problem& problem, SCIPRunResult& result,
                                double timeLimit){
  SCIP* scip = NULL;
  /*********
   * Setup *
   *********/

  /* initialize SCIP */
  SCIP_CALL( SCIPcreate(&scip) );

  SCIPprintVersion(scip,stdout);

  /* include default SCIP plugins */
  SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

  SCIP_CALL(SCIPcreateProbBasic(scip,problem.name.c_str()));
  if(problem.objectiveOffset != 0.0){
    SCIP_CALL(SCIPaddOrigObjoffset(scip,problem.objectiveOffset));
  }
  auto sense = problem.sense == ObjSense::MINIMIZE ? SCIP_OBJSENSE_MINIMIZE : SCIP_OBJSENSE_MAXIMIZE;
  SCIP_CALL(SCIPsetObjsense(scip,sense));

  std::vector<SCIP_VAR*> vars;
  for (int i = 0; i < problem.numCols; ++i) {
    SCIP_VAR* var;
    SCIP_VARTYPE type;
    switch(problem.colType[i]){
    case VariableType::CONTINUOUS: type = SCIP_VARTYPE_CONTINUOUS; break;
    case VariableType::BINARY: type = SCIP_VARTYPE_BINARY; break;
    case VariableType::INTEGER: type = SCIP_VARTYPE_INTEGER; break;
    case VariableType::IMPLIED_INTEGER: type = SCIP_VARTYPE_IMPLINT; break;
    }

    SCIP_CALL(SCIPcreateVarBasic(scip,&var,problem.colNames[i].c_str(),
                  convertValue(scip,problem.lb[i]),
                  convertValue(scip,problem.ub[i]),
                  convertValue(scip,problem.obj[i]),type));

    SCIP_CALL(SCIPaddVar(scip,var));
    vars.push_back(var);
  }

  std::vector<SCIP_CONS *> constraints;
  //TODO: temporary solution for initializing the constraints...
  {
    std::vector<std::vector<index_t>> consCols(problem.numRows, std::vector<index_t>());
    std::vector<std::vector<double>> consValues(problem.numRows, std::vector<double>());

    for (index_t i = 0; i < problem.numCols; ++i) {
      const index_t *colRows = problem.matrix.primaryIndices(i);
      const double *values = problem.matrix.primaryValues(i);
      index_t nonzeros = problem.matrix.primaryNonzeros(i);
      for (index_t j = 0; j < nonzeros; ++j) {
        index_t row = colRows[j];
        consCols[row].push_back(i);
        consValues[row].push_back(values[j]);
      }
    }
    std::vector<SCIP_VAR *> varBuffer;
    for (int i = 0; i < problem.numRows; ++i) {
      SCIP_CONS *cons;
      varBuffer.clear();
      for (auto &index : consCols[i]) {
        varBuffer.push_back(vars[index]);
      }
      SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, problem.rowNames[i].c_str(), varBuffer.size(),
                                          varBuffer.data(), consValues[i].data(),
                                          convertValue(scip, problem.lhs[i]),
                                          convertValue(scip, problem.rhs[i])));

      SCIP_CALL(SCIPaddCons(scip, cons));
      constraints.push_back(cons);
    }
  }

//  //For some reason SCIPconsAddCoef will not link...
//  for (index_t i = 0; i < problem.numCols; ++i) {
//    const index_t * colRows = problem.matrix.primaryIndices(i);
//    const double * values = problem.matrix.primaryValues(i);
//    index_t nonzeros = problem.matrix.primaryNonzeros(i);
//    for (index_t j = 0; j < nonzeros; ++j){
//      SCIP_CALL(SCIPconsAddCoef(scip,constraints[colRows[j]],vars[i],values[j]));
//    }
//  }

  SCIP_CALL(SCIPsetRealParam(scip,"limits/time",timeLimit));
  SCIP_CALL(SCIPsolve(scip));

  auto& solution = result.solution;
  SCIP_SOL * sol = SCIPgetBestSol(scip);
  solution.objectiveValue = SCIPsolGetOrigObj(sol);
  int nVars = SCIPgetNOrigVars(scip);
  for (int i = 0; i < nVars; ++i) {
    const char * name = SCIPvarGetName(vars[i]);
    double value = SCIPgetSolVal(scip,sol,vars[i]);
    solution.variableValues[name] = value;
  }

  for(SCIP_VAR *  var : vars){
    SCIP_CALL(SCIPreleaseVar(scip,&var));
  }
  for(SCIP_CONS * cons : constraints ){
    SCIP_CALL(SCIPreleaseCons(scip,&cons));
  }
  /********************
   * Deinitialization *
   ********************/

  SCIP_CALL( SCIPfree(&scip) );

  BMScheckEmptyMemory();

  return SCIP_OKAY;

}
std::optional<SCIPRunResult> solveProblemSCIP(const Problem& problem, double timeLimit){
  SCIPRunResult result;

  SCIP_RETCODE code = scipDoSolveProblem(problem,result,timeLimit);
  if(code != SCIP_OKAY){
    SCIPprintError(code);
    return std::nullopt;
  }
  return result;
}