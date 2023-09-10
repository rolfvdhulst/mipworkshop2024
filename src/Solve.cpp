//
// Created by rolf on 7-8-23.
//

#include "mipworkshop2024/Solve.h"
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <scip/cons.h>

#include <iostream>

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
  for (int i = 0; i < problem.numCols(); ++i) {
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
    std::vector<std::vector<index_t>> consCols(problem.numRows(), std::vector<index_t>());
    std::vector<std::vector<double>> consValues(problem.numRows(), std::vector<double>());

    for (index_t i = 0; i < problem.numCols(); ++i) {
        for(const Nonzero& nonzero : problem.matrix.getPrimaryVector(i)){
            consCols[nonzero.index()].push_back(i);
            consValues[nonzero.index()].push_back(nonzero.value());
        }
    }
    std::vector<SCIP_VAR *> varBuffer;
    for (int i = 0; i < problem.numRows(); ++i) {
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
  if(sol != NULL) {
      solution.objectiveValue = SCIPsolGetOrigObj(sol);
      int nVars = SCIPgetNOrigVars(scip);
      for (int i = 0; i < nVars; ++i) {
          const char *name = SCIPvarGetName(vars[i]);
          double value = SCIPgetSolVal(scip, sol, vars[i]);
          solution.variableValues[name] = value;
      }
  }
  result.statistics.problemName = SCIPgetProbName(scip);

  result.statistics.timeTaken = SCIPgetSolvingTime(scip);
  result.statistics.nodes = SCIPgetNTotalNodes(scip);
  result.statistics.solvedOptimally = SCIPgetStatus(scip) == SCIP_STATUS_OPTIMAL ||
		  SCIPgetStatus(scip) == SCIP_STATUS_INFEASIBLE || SCIPgetStatus(scip) == SCIP_STATUS_UNBOUNDED;
  result.statistics.isInfeasible = SCIPgetStatus(scip) == SCIP_STATUS_INFEASIBLE;
  result.statistics.isUnbounded = SCIPgetStatus(scip) == SCIP_STATUS_UNBOUNDED;
  result.statistics.dualBound = SCIPgetDualbound(scip);
  result.statistics.primalBound = SCIPgetPrimalbound(scip);
  result.statistics.rootLPValue = SCIPgetLPRootObjval(scip);
  result.statistics.numSolutions = SCIPgetNSols(scip);

  result.statistics.numBinaryAfterPresolve = SCIPgetNBinVars(scip);
  result.statistics.numIntAfterPresolve = SCIPgetNIntVars(scip);
  result.statistics.numImpliedAfterPresolve = SCIPgetNImplVars(scip);
  result.statistics.numContinuousAfterPresolve = SCIPgetNContVars(scip);
  result.statistics.numConsAfterPresolve = SCIPgetNConss(scip);

	result.statistics.numBinaryBeforePresolve = SCIPgetNOrigBinVars(scip);
	result.statistics.numIntBeforePresolve = SCIPgetNOrigIntVars(scip);
	result.statistics.numImpliedBeforePresolve = SCIPgetNOrigImplVars(scip);
	result.statistics.numContinuousBeforePresolve = SCIPgetNOrigContVars(scip);
	result.statistics.numConsBeforePresolve = SCIPgetNOrigConss(scip);
	result.statistics.TUDetectionTime = 0.0;


	SCIP_CALL(SCIPprintStatistics(scip,stdout));
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

SCIP_RETCODE doSolveTULP(const Problem& problem,
		const TotallyUnimodularColumnSubmatrix& submatrix,
		Solution& currentSol){
	std::vector<std::size_t> rowMapping(problem.matrix.numRows(),INVALID);
	for(index_t i = 0; i < submatrix.submatRows.size(); ++i){
		rowMapping[submatrix.submatRows[i]] = i;
	}
	SCIP * scip;
	SCIP_CALL(SCIPcreate(&scip));
	SCIPprintVersion(scip,stdout);

	/* include default SCIP plugins */
	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

	std::string name = problem.name + "_TU";
	SCIP_CALL(SCIPcreateProbBasic(scip,name.c_str()));
	SCIP_CALL(SCIPsetRealParam(scip,"limits/time",100.0));

	auto sense = problem.sense == ObjSense::MINIMIZE ? SCIP_OBJSENSE_MINIMIZE : SCIP_OBJSENSE_MAXIMIZE;
	SCIP_CALL(SCIPsetObjsense(scip,sense));

	std::vector<SCIP_VAR*> vars;
	for(index_t col : submatrix.submatColumns){
		SCIP_VAR * var;
		SCIP_CALL(SCIPcreateVarBasic(scip,&var,problem.colNames[col].c_str(),
				convertValue(scip,problem.lb[col]),
				convertValue(scip,problem.ub[col]),
				convertValue(scip,problem.obj[col]),
				SCIP_VARTYPE_INTEGER)); //TODO: integer or continuous?

				//TODO: check how to warmstart LP solver
		SCIP_CALL(SCIPaddVar(scip,var));
		vars.push_back(var);
	}

	std::vector<double> consLHS;
	std::vector<double> consRHS;
	for(const auto& row : submatrix.submatRows){
		consLHS.push_back(problem.lhs[row]);
		consRHS.push_back(problem.rhs[row]);
	}
	std::vector<std::vector<SCIP_VAR *>> consVars(submatrix.submatRows.size(), std::vector<SCIP_VAR*>());
	std::vector<std::vector<double>> consValues(submatrix.submatRows.size(), std::vector<double>());
	index_t newCol = 0;
	for(index_t column : submatrix.submatColumns){
		for(const Nonzero& nonzero : problem.matrix.getPrimaryVector(column)){
			index_t originalRow = nonzero.index();
			index_t newRow = rowMapping[originalRow];
			assert(newRow != INVALID);
			consValues[newRow].push_back(nonzero.value());
			consVars[newRow].push_back(vars[newCol]);
		}
		++newCol;
	}

	for(index_t column : submatrix.implyingColumns){
		double colVal = currentSol.values[column];
		assert(isFeasIntegral(colVal));
		for(const Nonzero& nonzero : problem.matrix.getPrimaryVector(column)){
			index_t originalRow = nonzero.index();
			index_t newRow = rowMapping[originalRow];
			if(newRow == INVALID) continue;
			double total = nonzero.value() * colVal;
			assert(isFeasIntegral(total));
			if(!isInfinite(-consLHS[newRow])){
				consLHS[newRow] -= total;
			}
			if(!isInfinite(consRHS[newRow])){
				consRHS[newRow] -= total;
			}
		}
	}

	std::vector<SCIP_CONS *> constraints;
	for(index_t i = 0; i < submatrix.submatRows.size(); ++i){
		index_t row = submatrix.submatRows[i];
		SCIP_CONS * cons = NULL;
		SCIP_CALL(SCIPcreateConsBasicLinear(scip, &cons, problem.rowNames[row].c_str(), consVars[i].size(),
				consVars[i].data(), consValues[i].data(),
				convertValue(scip, consLHS[i]),
				convertValue(scip, consRHS[i])));

		SCIP_CALL(SCIPaddCons(scip, cons));
		constraints.push_back(cons);
	}


	SCIP_CALL(SCIPsolve(scip));
	if(SCIPgetStatus(scip) != SCIP_STATUS_OPTIMAL){
		std::cout<<"Postsolve problem was not solved!\n";
		return SCIP_ERROR;
	}
	assert(SCIPgetNTotalNodes(scip) <= 1);
	SCIP_SOL* bestSol = SCIPgetBestSol(scip);
	for(index_t i = 0 ; i < submatrix.submatColumns.size(); ++i){
		double value = SCIPgetSolVal(scip,bestSol,vars[i]);
		assert(isFeasIntegral(value));
		currentSol.values[submatrix.submatColumns[i]] = value;
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
std::optional<Solution> doPostSolve(const Problem& originalProblem,
		const Solution& fractionalSolution,
		const PostSolveStack& postSolveStack){
	Solution correctedSol = fractionalSolution;
	for(auto it = postSolveStack.reductions.rbegin(); it != postSolveStack.reductions.rend(); ++it){
		bool isAlreadyGood = true;
		for(index_t column : it->submatColumns){
			if(!isFeasIntegral(correctedSol.values[column])){
				isAlreadyGood = false;
				break;
			}
		}
		if(isAlreadyGood) continue;
#ifndef NDEBUG
		for(index_t implyingCol : it->implyingColumns){
			assert(isFeasIntegral(correctedSol.values[implyingCol]));
		}
#endif
		SCIP_RETCODE code = doSolveTULP(originalProblem,*it,correctedSol);
		if(code != SCIP_OKAY){
			std::cout<<"Some error occurred during TU postsolve\n";
			return std::nullopt;
		}
		for(index_t col : it->submatColumns){
			if(!isFeasIntegral(correctedSol.values[col])){
				std::cout<<"Still not integral after TU submatrix solve... very suspicious\n";
				return std::nullopt;
			}
		}
	}

	return correctedSol;
}