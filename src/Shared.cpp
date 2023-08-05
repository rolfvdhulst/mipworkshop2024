//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/Shared.h"
#include <iostream>
#include <filesystem>
#include <ctime>
#include <cassert>
void printInstanceString(const std::string& fullPath){
  std::cout<<"[INSTANCE] "<<std::filesystem::path(fullPath).filename().string()<<"\n";
}

//TODO: fix milliseconds here
void printStartString(){
  time_t now = time(nullptr);
  struct tm * tm = localtime(&now);
  char s[64];
  size_t ret = strftime(s,sizeof(s),"%Y-%m-%dT%H:%M:%S",tm);
  assert(ret);
  std::cout<<"[START] "<< s <<"\n";
}
void printEndString(){
  time_t now = time(nullptr);
  struct tm * tm = localtime(&now);
  char s[64];
  size_t ret = strftime(s,sizeof(s),"%Y-%m-%dT%H:%M:%S",tm);
  assert(ret);
  std::cout<<"[END] "<< s<<"\n";
}
