//
// Created by rolf on 6-8-23.
//
#include <iostream>
#include <filesystem>
#include <cassert>
#include "IO.h"
#include "mipworkshop2024/ApplicationShared.h"

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
  //TODO: presolving methods to modify the problem and produce some post solving data
  if(!writeMPSFile(problem.value(),presolvedProblemPath)){
    std::cerr<<"Could not write to: "<<presolvedProblemPath<<"\n";
    return false;
  }

  return true;
}
