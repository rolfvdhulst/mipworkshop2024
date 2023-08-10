//
// Created by rolf on 6-8-23.
//
#include <iostream>
#include <filesystem>
#include <cassert>
#include "mipworkshop2024/IO.h"
#include "mipworkshop2024/ApplicationShared.h"

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

void printInstanceString(const std::string& fullPath){
  std::cout<<"[INSTANCE] "<<std::filesystem::path(fullPath).filename().string()<<"\n";
}

//TODO: fix
std::chrono::high_resolution_clock::time_point printStartString(){
  time_t now = time(nullptr);
  struct tm * tm = localtime(&now);
  char s[64];
  size_t ret = strftime(s,sizeof(s),"%Y-%m-%dT%H:%M:%S",tm);
  assert(ret);
  std::cout<<"[START] "<< s <<"\n";
  return std::chrono::high_resolution_clock::now();
}
std::chrono::high_resolution_clock::time_point printEndString(){
  time_t now = time(nullptr);
  struct tm * tm = localtime(&now);
  char s[64];
  size_t ret = strftime(s,sizeof(s),"%Y-%m-%dT%H:%M:%S",tm);
  assert(ret);
  std::cout<<"[END] "<< s<<"\n";
  return std::chrono::high_resolution_clock::now();
}

bool doPresolve(const std::string& problemPath,
                const std::string& presolvedProblemPath,
                const std::string& outputPath){
  if(!std::filesystem::exists(problemPath)){
    std::cerr<<"Input file: "<<problemPath<<" does not exist!\n";
    return false;
  }
  if(!std::filesystem::exists(outputPath)){
    std::cerr<<"Output directory does not yet exist, creating folder at: "<< outputPath <<"\n";
    if(!std::filesystem::create_directory(outputPath)){
      std::cerr<<"Could not create output directory!\n";
      return false;
    }
  }
  auto path = std::filesystem::path(problemPath);
  auto start = std::chrono::high_resolution_clock::now();
  auto problem = readMPSFile(path);
  auto end = std::chrono::high_resolution_clock::now();
  if(!problem.has_value()){
    std::cerr<<"Error during reading file: "<<path<<"\n";
    return false;
  }
  printInstanceString(path);
  std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<<" ms to read problem\n";
  auto compStart = printStartString();
  auto compEnd = printEndString();

  start = std::chrono::high_resolution_clock::now();
  if(!writeMPSFile(problem.value(),presolvedProblemPath)){
    std::cerr<<"Could not write to: "<<presolvedProblemPath<<"\n";
    return false;
  }
  end = std::chrono::high_resolution_clock::now();
  std::cout<<std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<<" ms to write presolved problem\n";

  return true;
}
bool doPostsolve(const std::string& problemPath,
                 const std::string& presolvedProblemPath,
                 const std::string& outputPath,
                 const std::string& presolvedSolution,
                 const std::string& postsolvedSolution){

  if(!std::filesystem::exists(problemPath) || !std::filesystem::is_regular_file(problemPath)){
    std::cerr<<"Input file: "<<problemPath<<" does not exist!\n";
    return false;
  }
  if(!std::filesystem::exists(presolvedProblemPath) || !std::filesystem::is_regular_file(presolvedProblemPath)){
    std::cerr<<"Presolved file "<<presolvedProblemPath<<" does not exist!\n";
    return false;
  }
  if(!std::filesystem::exists(outputPath)){
    std::cerr<<"Output directory at: "<< outputPath<< " does not exist!\n";
    return false;
  }
  if(!std::filesystem::exists(presolvedSolution)){
    std::cerr<<"Presolved solution at: "<<presolvedSolution << " does not exist!\n";
    return false;
  }
  printInstanceString(problemPath);

  auto solution = readSolFile(presolvedSolution);
  if(!solution.has_value()){
    std::cerr<<"Error during reading of solution file: "<<presolvedSolution << "\n";
    return false;
  }

  auto start = printStartString();
  //TODO: actual postsolving

  auto end = printEndString();

  std::ofstream outStream(postsolvedSolution);
  if(!outStream.is_open()){
    std::cerr<<"Could not open solution file at: "<<postsolvedSolution<<"\n";
    return false;
  }
  start = std::chrono::high_resolution_clock::now();
  if(!solToStream(solution.value(),outStream)){
    std::cerr<<"Error during writing postsolved solution: "<<postsolvedSolution<<"\n";
    return false;
  }
  end = std::chrono::high_resolution_clock::now();
  std::cout<<"Wrote solution to: "<<postsolvedSolution<<" in "<<std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count()<<" ms\n";

  return true;
}

static
SCIP_RETCODE runSCIP(const std::string& problemPath, const std::string& solutionPath)
{
  SCIP* scip = NULL;

  /*********
   * Setup *
   *********/

  /* initialize SCIP */
  SCIP_CALL( SCIPcreate(&scip) );

  /* include default SCIP plugins */
  SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

  SCIP_CALL(SCIPreadProb(scip,problemPath.c_str(),NULL));

  SCIP_CALL(SCIPsetRealParam(scip,"limits/time",3.0));
  SCIP_CALL(SCIPsolve(scip));

  ExternalSolution solution;
  SCIP_SOL * sol = SCIPgetBestSol(scip);
  solution.objectiveValue = SCIPsolGetOrigObj(sol);
  SCIP_VAR ** vars = SCIPgetOrigVars(scip);
  int nVars = SCIPgetNOrigVars(scip);
  for (int i = 0; i < nVars; ++i) {
    const char * name = SCIPvarGetName(vars[i]);
    double value = SCIPgetSolVal(scip,sol,vars[i]);
    solution.variableValues[name] = value;
  }

  std::ofstream fileStream(solutionPath);
  if(!fileStream.is_open() || !fileStream.good()){
    return SCIP_FILECREATEERROR;
  }
  if(!solToStream(solution,fileStream)){
    return SCIP_FILECREATEERROR;
  }
  /********************
   * Deinitialization *
   ********************/

  SCIP_CALL( SCIPfree(&scip) );

  BMScheckEmptyMemory();

  return SCIP_OKAY;
}
bool runSCIPSeparate(const std::string& problemPath, const std::string& writeSolutionPath){

  SCIP_RETCODE retcode;

  retcode = runSCIP(problemPath,writeSolutionPath);
  if( retcode != SCIP_OKAY )
  {
    SCIPprintError(retcode);
    return false;
  }

  return true;
}