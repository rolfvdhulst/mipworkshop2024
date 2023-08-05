//
// Created by rolf on 1-8-23.
//

#include "mipworkshop2024/IO.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <iostream>
#include <bitset>

bool solToStream(const Solution& solution, std::ostream& stream){
  stream <<"=obj= " << std::to_string(solution.objectiveValue);
  if(!stream.good()){
    return false;
  }
  for(const auto& pair : solution.variableValues){
    stream <<"\n" << pair.first <<" "<<std::to_string(pair.second);
  }
  return stream.good();
}

std::optional<Solution> solFromStream(std::istream& stream){
  Solution solution;
  std::string buffer;

  while(!stream.eof()){
    getline(stream,buffer);
    if(buffer.empty() || buffer.starts_with('#')){
      continue;
    }else if(buffer.starts_with("=obj= ")){
      double value = std::stod(buffer.substr(6));
      solution.objectiveValue = value;
    }else{
      int whiteSpaceIndex = -1;
      for (int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] == ' '){
          whiteSpaceIndex = i;
          break;
        }
      }
      if(whiteSpaceIndex == -1){
        std::cerr<<"Could not read line: "<<buffer<<"\n";
        continue;
      }
      std::string varName = buffer.substr(0,whiteSpaceIndex);
      double value = std::stod(buffer.substr(whiteSpaceIndex+1));
      solution.variableValues[varName] = value ;
    }
  }

  if(!stream.eof()){
    return std::nullopt;
  }
  return solution;
}

std::optional<Solution> solFromCompressedStream(std::istream& stream){
  boost::iostreams::filtering_istream is;
  is.push(boost::iostreams::gzip_decompressor());
  is.push(stream);
  Solution solution;

  std::string buffer;
  buffer.reserve(1024);
  while(!is.eof()){
    std::getline(is,buffer);
    if(buffer.empty() || buffer.starts_with('#')){
      continue;
    }else if(buffer.starts_with("=obj= ")){
      double value = std::stod(buffer.substr(6));
      solution.objectiveValue = value;
    }else{
      int whiteSpaceIndex = -1;
      for (int i = 0; i < buffer.size(); ++i) {
        if(buffer[i] == ' '){
          whiteSpaceIndex = i;
          break;
        }
      }
      if(whiteSpaceIndex == -1){
        std::cerr<<"Could not read line: "<<buffer<<"\n";
        continue;
      }
      std::string varName = buffer.substr(0,whiteSpaceIndex);
      double value = std::stod(buffer.substr(whiteSpaceIndex+1));
      solution.variableValues[varName] = value ;
    }
  }

  if(!is.eof()){
    return std::nullopt;
  }
  return solution;
}

std::optional<Problem> readMPSFile(const std::filesystem::path& path){
  std::ifstream stream(path);
  if(path.extension() == ".gz" && path.stem().extension() == ".mps"){
    return problemFromCompressedMPSStream(stream);
  }else if(path.extension() == ".mps"){
    return problemFromMPSstream(stream);
  }
  std::cerr<<"Path does not have correct extensions!\n";
  return std::nullopt;
}
std::optional<Problem> problemFromCompressedMPSStream(std::istream& stream){
  boost::iostreams::filtering_istream is;
  is.push(boost::iostreams::gzip_decompressor());
  is.push(stream);

  return problemFromMPSstream(is);
}


enum class MPSSection{
  NAME = 0,
  ROWS = 1,
  COLUMNS = 2,
  RHS = 3,
  RANGES = 4,
  BOUNDS = 5,
  ENDATA = 6,
  OBJSENSE = 7,
  OBJNAME = 8,
  UNDEFINED
};

std::string sectionToString(MPSSection section){
  switch(section){
  case MPSSection::NAME: return "NAME";
  case MPSSection::ROWS: return "ROWS";
  case MPSSection::COLUMNS: return "COLUMNS";
  case MPSSection::RHS: return "RHS";
  case MPSSection::RANGES: return "RANGES";
  case MPSSection::BOUNDS: return "BOUNDS";
  case MPSSection::ENDATA: return "ENDATA";
  case MPSSection::OBJSENSE: return "OBJSENSE";
  case MPSSection::OBJNAME: return "OBJNAME";
  case MPSSection::UNDEFINED:
  default:
    return "UNDEFINED";
  }
}

constexpr std::array<MPSSection,5> MANDATORY_SECTIONS= {MPSSection::NAME,MPSSection::ROWS,MPSSection::COLUMNS,
                                                        MPSSection::RHS,MPSSection::ENDATA};

struct FreeRowData{
  FreeRowData();
  double rhs;
  std::vector<double> coefficients;
  std::vector<std::string> columns; //TODO: inefficient solution
};
FreeRowData::FreeRowData() : rhs{0.0}{

}
/// Currently, we support a few of CPLEX' extensions to MPS files.
/// The only exception as far as we are aware is that we do not suppor that the dollar sign '$',
/// can be used in field 3 and 5 to treat the remaining characters as a comment
struct MPSReader{
  MPSSection section = MPSSection::UNDEFINED;
  std::size_t lineCount = 0;
  std::bitset<9> sawSection;

  std::string objectiveRowName;

  std::unordered_map<std::string,FreeRowData> freeRows;

  bool processedFirstColumn = false;
  bool markNewColsInteger = false;
  bool colIsInteger = false;
  std::string lastColumn;
  std::vector<index_t> columnRows;
  std::vector<double> columnValues;

  bool finalizeModel(Problem& problem);
  bool setNewSection(Problem& problem,MPSSection section);
  //Method which is called when the next section is detected

  void finalizeSection(Problem& problem);
  void finalizeCOLUMNS(Problem& problem);

  bool processNewSection(const std::string& line, Problem& problem);
  bool processSectionLine(const std::string& line, Problem& problem);
  bool processNAMELine(const std::string& line, Problem& problem);
  bool processROWSLine(const std::string& line, Problem& problem);

  bool processCOLUMNSLine(const std::string& line, Problem& problem);
  bool processRHSLine(const std::string& line, Problem& problem);
  bool processRANGESLine(const std::string& line, Problem& problem);
  bool processBOUNDSLine(const std::string& line, Problem& problem);
  bool processOBJSENSELine(const std::string& line, Problem& problem);
  bool processObjsenseWord(const std::string& word, Problem& problem);
  bool processOBJNAMELine(const std::string& line, Problem& problem);


  bool addRow(Problem& problem, const std::string& name, char senseChar);
  bool addRange(Problem& problem, const std::string& rowName, const std::string& value );

  bool addColumnNonzero(Problem& problem,
                        const std::string& rowName,
                        const std::string& value);
  void flushColumn(Problem& problem);

  bool addRHS(Problem& problem,
              const std::string& rowName, const std::string& value);

  bool computeBoundValue(
      index_t& outCol,
      double& outVal,
      Problem& problem,
      const std::string& colName,
      const std::string& value);
};


std::vector<std::string> splitString(const std::string& string,char delim=' '){
  std::vector<std::string> result;

  for(std::size_t stringPos = 0; stringPos < string.size(); ){
    if(string[stringPos] == delim){
      ++stringPos;
      continue;
    }
    std::size_t wordBegin = stringPos;
    std::size_t wordSize = 0;
    while(stringPos != string.size() && string[stringPos] != delim){
      ++stringPos;
      ++wordSize;
    }
    result.push_back(string.substr(wordBegin,wordSize));
  }

  return result;
}

constexpr std::array<const char*,1> UNSUPPORTED_SECTIONS = {"SOS"};
bool MPSReader::processNewSection(const std::string& line, Problem &problem) {
  auto words = splitString(line);
  assert(!words.empty());
  if(words[0] == "NAME" ){
    setNewSection(problem,MPSSection::NAME);
    if(words.size() == 2){
      problem.name = words.back();
    }
    if(words.size() > 2){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }

  if (words[0] == "OBJSENSE") {
      setNewSection(problem, MPSSection::OBJSENSE);
      if (words.size() == 2) {
          if (!processObjsenseWord(words[1], problem)) {
              return false;
          }
      }
      if (words.size() > 2) {
          std::cerr << "Unexpected fields in mps file, line: " << lineCount << "\n";
          return false;
      }
      return true;
  }

  if(words[0] == "OBJNAME"){
      setNewSection(problem, MPSSection::OBJNAME);
      if(words.size() == 2){
          objectiveRowName = words[1];
      }
      if (words.size() > 2) {
          std::cerr << "Unexpected fields in mps file, line: " << lineCount << "\n";
          return false;
      }
      return true;
  }
  if(words[0] == "ROWS"){
    setNewSection(problem,MPSSection::ROWS);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }
  if(words[0] == "COLUMNS"){
    if(!sawSection.test(static_cast<std::size_t>(MPSSection::ROWS))){
      std::cerr<<"The COLUMNS section occurs before the row section\n";
      return false;
    }
    setNewSection(problem,MPSSection::COLUMNS);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }

  //TODO: check if other sections were done
  if(words[0] == "RHS"){
    setNewSection(problem,MPSSection::RHS);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }
  if(words[0] == "RANGES"){
    setNewSection(problem,MPSSection::RANGES);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }
  if(words[0] == "BOUNDS"){
    setNewSection(problem,MPSSection::BOUNDS);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }

  if(words[0] == "ENDATA"){
    setNewSection(problem,MPSSection::ENDATA);
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    return true;
  }
  for(const auto& name : UNSUPPORTED_SECTIONS){
      if(words[0] == name){
          std::cerr<<"Section: "<<name<<" is not yet supported\n";
          return false;
      }
  }

  std::cerr<<"Unexpected section name: "<<words[0]<<", line "<<lineCount<<"\n";
  return false;
}

bool MPSReader::processSectionLine(const std::string& line, Problem &problem) {
  switch(section){
  case MPSSection::NAME: return processNAMELine(line,problem);
  case MPSSection::ROWS: return processROWSLine(line,problem);
  case MPSSection::COLUMNS: return processCOLUMNSLine(line,problem);
  case MPSSection::RHS: return processRHSLine(line,problem);
  case MPSSection::RANGES: return processRANGESLine(line,problem);
  case MPSSection::BOUNDS: return processBOUNDSLine(line,problem);
  case MPSSection::OBJSENSE: return processOBJSENSELine(line,problem);
  case MPSSection::OBJNAME: return processOBJNAMELine(line,problem);
  default:{
    std::cerr<<"Can not process line "<<lineCount<<" in section "<<sectionToString(section)<<"\n";
    return false;
  }
  }
}
bool MPSReader::setNewSection(Problem& problem, MPSSection t_section) {
  finalizeSection(problem);
  assert(t_section != MPSSection::UNDEFINED);
  section = t_section;
  auto pos = static_cast<std::size_t>(section);
  if(sawSection.test(pos)) {
    std::cerr<<"Error on line: "<<lineCount<<", already saw section: "<<sectionToString(section)<<"\n";
    return false;
  }
  sawSection.set(pos);
  return true;
}

///Checks if the mps file was complete or if any sections were missing
bool MPSReader::finalizeModel(Problem& problem) {
  if(section != MPSSection::ENDATA){
    std::cerr<<"File does not contain an ENDATA section\n";
    return false;
  }
  //check if all mandatory sections are present
  bool good = true;
  for(MPSSection mandatorySection : MANDATORY_SECTIONS){
    good = good && sawSection.test(static_cast<std::size_t>(mandatorySection));
  }
  if(!good){
    std::cerr<<"Missing mandatory MPS sections: ";
    for(MPSSection mandatorySection : MANDATORY_SECTIONS){
      if(!sawSection.test(static_cast<std::size_t>(mandatorySection))){
        std::cerr<<sectionToString(mandatorySection)<<", ";
      }
    }
    std::cerr<<"\n";
    return false;
  }

  //TODO: fix that variables in integral sections are assumed to be binary, unless bounds are specified
  //Set objective. We do this only at the end because of the OBJNAME section might declare a free row to be the objective later
  if(!objectiveRowName.empty()){
    auto freeRowIt = freeRows.find(objectiveRowName);
    assert(freeRowIt != freeRows.end());
    const auto& freeRowData = freeRowIt->second;
    for(std::size_t i = 0; i < freeRowData.coefficients.size(); ++i){
      auto colIndexIt = problem.colToIndex.find(freeRowData.columns[i]);
      if(colIndexIt == problem.colToIndex.end()){
        std::cerr<<"Could not find column: "<<freeRowData.columns[i] <<" in the objective\n";
        return false;
      }
      index_t column = colIndexIt->second;
      problem.obj[column] = freeRowData.coefficients[i];
    }
    problem.objectiveOffset = -freeRowData.rhs;

  }
  //Cleanup the row name mapping by removing free rows; we initially have this so that it is easier to test for duplicate row names
  for(const auto& freeRow : freeRows){
    std::size_t numRemoved = problem.rowToIndex.erase(freeRow.first);
    assert(numRemoved == 1);
  }
  return true;
}
bool MPSReader::processNAMELine(const std::string &line, Problem &problem) {
    auto words = splitString(line); //TODO: maybe check if we can increase performance here because we do this for every section
    assert(!words.empty());
    if(words.size() > 1){
      std::cerr<<"Unexpected fields in mps file, line: "<<lineCount<<"\n";
      return false;
    }
    if(!problem.name.empty()){
      std::cerr<<"[Warning] overwriting problem name, line "<<lineCount<<"\n";
    }
    problem.name = words[0];

    return true;
}

bool MPSReader::processROWSLine(const std::string& line, Problem& problem){
  auto words = splitString(line); //TODO: maybe check if we can increase performance here because we do this for every section
  assert(!words.empty());
  assert(!words[0].empty());
  char senseChar = words[0][0];
  if(words[0].size() != 1 || !(senseChar == 'N' || senseChar == 'L' || senseChar == 'G' || senseChar == 'E')){
    std::cerr<<"Error in ROWS section, line: "<<lineCount<<": please provide a valid sense character\n";
    return false;
  }
  if(words.size() != 2){
    std::cerr<<"Unexpected number of fields in mps file,ROW section, line: "<<lineCount<<"\n";
    return false;
  }
  return addRow(problem,words[1],senseChar);
}
bool MPSReader::processCOLUMNSLine(const std::string& line, Problem& problem){
  auto words = splitString(line); //TODO: maybe check if we can increase performance here because we do this for every section
  assert(!words.empty());
  if(words.size() != 3 && words.size() != 5){
    std::cerr<<"Unexpected number of fields in mps file,COLUMNS section, line: "<<lineCount<<"\n";
    return false;
  }
  //TODO: marker for integer variables
  if(words[1] == "'MARKER'"){
    if(words[2] == "'INTORG'"){
      if(markNewColsInteger){
        std::cerr<<"Cannot nest 'INTORG' markers, line: "<<lineCount<<"\n";
        return false;
      }
      markNewColsInteger = true;
      return true;
    }else if(words[2] == "'INTEND'"){
      if(!markNewColsInteger){
        std::cerr<<"Cannot nest 'INTEND' markers, line: "<<lineCount<<"\n";
        return false;
      }
      markNewColsInteger = false;
      return true;
    }else{
      std::cerr<<"Unexpected MARKER value in line: "<<lineCount<<"\n";
      return false;
    }
  }
  if(words[0] != lastColumn){
    if(processedFirstColumn){
      flushColumn(problem);
    }else{
      processedFirstColumn = true;
    }

    if(problem.colToIndex.contains(words[0])){
      std::cerr<<"Cannot declare a second column with name: "<< words[0]<<"\n";
      return false;
    }
    lastColumn = words[0];
    colIsInteger = markNewColsInteger;
  }
  if(!addColumnNonzero(problem,words[1],words[2])){
    return false;
  }
  if(words.size() == 5){
    return addColumnNonzero(problem,words[3],words[4]);
  }
  return true;
}
bool MPSReader::processRHSLine(const std::string &line, Problem &problem) {
  auto words = splitString(line); //TODO
  assert(!words.empty());
  if(words.size() != 3 && words.size() != 5){
    std::cerr<<"Unexpected number of fields in mps file, RHS section, line: "<<lineCount<<"\n";
    return false;
  }
  if(!addRHS(problem,words[1],words[2])){
    return false;
  }
  if(words.size() == 5){
    return addRHS(problem,words[3],words[4]);
  }
  return true;
}
bool MPSReader::processRANGESLine(const std::string& line, Problem& problem){
  auto words = splitString(line);
  if(words.size() != 3 && words.size() != 5){
    std::cerr<<"Unexpected number of fields in mps file, RANGES section, line: "<<lineCount<<"\n";
    return false;
  }
  if(!addRange(problem,words[1],words[2])){
    return false;
  }
  if(words.size() == 5){
    return addRange(problem,words[3],words[4]);
  }
  return true;
}
bool MPSReader::processBOUNDSLine(const std::string& line, Problem& problem){
  auto words = splitString(line); //TODO
  assert(!words.empty());
  if(words.size() <3 && words.size() >6){
    std::cerr<<"Unexpected number of fields in mps file, BOUNDS section, line: "<<lineCount<<"\n";
    return false;
  }
  if(words[0].size() != 2){
    std::cerr<<"Bound identifier "<<words[0]<<" was not recognized, line "<<lineCount<<"\n";
    return false;
  }
  if(words[0] == "SC"){
    std::cerr<<"Semi-continuous variables are not supported, line: "<<lineCount<<"\n";
    return false;
  }

  bool doNotComputeValues = words[0] == "FR" || words[0] == "MI" || words[0] == "PL" || words[0] == "BV";
  index_t indices[2] = {INVALID,INVALID};
  double values[2] = {0.0,0.0};
  index_t numIndices = 1;

  if(doNotComputeValues){
    if(words.size() > 4){
      std::cerr<<"Unexpected number of fields in mps file, BOUNDS section, line: "<<lineCount<<"\n";
      return false;
    }
    auto it = problem.colToIndex.find(words[2]);
    if(it == problem.colToIndex.end()){
      std::cerr<<"Could not find column name: "<<words[2] <<" in BOUNDS section, line: "<<lineCount <<"\n";
      return false;
    }
    indices[0] = it->second;

    if(words.size() ==4){
      //only MIPLIB benchmark instances which have a redundant word here are leo1 and leo2
      //std::cout<<"Ignoring word: "<<words[3]<<"\n";

    }

    if (words[0] == "FR"){
      for (int i = 0; i < numIndices; ++i) {
        problem.lb[indices[i]] = -infinity;
        problem.ub[indices[i]] = infinity;
      }
    }else if(words[0] == "MI"){
      for (int i = 0; i < numIndices; ++i) {
        problem.lb[indices[i]] = -infinity;
      }
    }else if(words[0] == "PL"){
      for (int i = 0; i < numIndices; ++i) {
        problem.ub[indices[i]] = infinity;
      }
    }else if(words[0] == "BV"){
      for (int i = 0; i < numIndices; ++i) {
        problem.lb[indices[i]] = 0.0;
        problem.ub[indices[i]] = 1.0;
        problem.colType[indices[i]] = VariableType::BINARY;
      }
    }
    return true;
  }
  if(words.size() !=4  && words.size() != 6){
    std::cerr<<"Unexpected number of fields in mps file, BOUNDS section, line: "<<lineCount<<"\n";
    return false;
  }

  if(!computeBoundValue(indices[0],values[0],problem,words[2],words[3])){
    return false;
  }
  if(words.size() == 6){
    if(!computeBoundValue(indices[1],values[1],problem,words[4],words[5])){
      return false;
    }
    ++numIndices;
  }

  if(words[0] == "LO"){
    for (index_t i = 0; i < numIndices; ++i) {
      problem.lb[indices[i]] = values[i];
    }
  }else if(words[0] == "LI"){
    for(index_t i = 0; i < numIndices; ++i){
      if(!isFeasIntegral(values[i])){
        std::cerr<<"Lower bound value: "<<values[i]<<" in line "<<lineCount<<" is not integral\n";
        return false;
      }
    }
    for (index_t i = 0; i < numIndices; ++i) {
      problem.lb[indices[i]] = values[i];
      problem.colType[indices[i]] = VariableType::INTEGER;
    }
  }else if(words[0] == "UP"){
    for (index_t i = 0; i < numIndices; ++i) {
      problem.ub[indices[i]] = values[i];
    }
  }else if(words[0] == "UI"){
    for(index_t i = 0; i < numIndices; ++i){
      if(!isFeasIntegral(values[i])){
        std::cerr<<"Upper bound value: "<<values[i]<<" in line "<<lineCount<<" is not integral\n";
        return false;
      }
    }
    for (index_t i = 0; i < numIndices; ++i) {
      problem.ub[indices[i]] = values[i];
      problem.colType[indices[i]] = VariableType::INTEGER;
    }
  }else if(words[0] == "FX"){
    for (index_t i = 0; i < numIndices; ++i) {
      problem.lb[indices[i]] = values[i];
      problem.ub[indices[i]] = values[i];
    }
  }
  else{
    std::cerr<<"Unrecognized BOUNDS identifier: "<<words[0]<<", line "<< lineCount<<"\n";
    return false;
  }
  return true;

}


bool MPSReader::addRow(Problem& problem, const std::string& name, char senseChar) {
  if(problem.rowToIndex.contains(name)){
    std::cerr<<"Cannot declare a second row with name: "<< name<<"\n";
    return false;
  }
  if(senseChar == 'N'){
    problem.rowToIndex[name] = INVALID;
    freeRows[name] = FreeRowData();
    if(objectiveRowName.empty()){
      objectiveRowName = name;
    }
    return true;
  }
  double lhs = -infinity;
  double rhs = infinity;
  switch(senseChar){
  case 'L':{
    rhs = 0.0;
    break;
  }
  case 'G':{
    lhs = 0.0;
    break;
  }
  case 'E': {
    lhs = 0.0;
    rhs = 0.0;
    break;
  }
  default:
    break;
  }
  problem.addRow(name,lhs,rhs);
  return true;
}
void MPSReader::finalizeSection(Problem& problem) {
  switch(section){
  case MPSSection::COLUMNS: {
    finalizeCOLUMNS(problem);
    break;
  }
  default: break;
  }
}
void MPSReader::finalizeCOLUMNS(Problem& problem) {
  flushColumn(problem);
}
void MPSReader::flushColumn(Problem &problem) {
  double lb = 0.0;
  double ub = colIsInteger ? 1.0 : infinity;
  VariableType type = colIsInteger ? VariableType::INTEGER : VariableType::CONTINUOUS;
  problem.addColumn(lastColumn,columnRows,columnValues,type,lb,ub);
  columnRows.clear();
  columnValues.clear();
}
bool MPSReader::addColumnNonzero(Problem& problem,
    const std::string& rowName, const std::string& value) {
  auto it = problem.rowToIndex.find(rowName);
  if(it == problem.rowToIndex.end()){
    std::cerr<<"Row: "<<rowName<< " was not declared\n";
    return false;
  }
  double val = std::stod(value); //TODO: error handling
  index_t rowIndex = it->second;
  if(rowIndex == INVALID){ //Row is free
    auto freeIt = freeRows.find(rowName);
    assert(freeIt != freeRows.end());
    auto& rowData = freeIt->second;
    rowData.coefficients.push_back(val);
    rowData.columns.push_back(lastColumn);
  }else{
    columnRows.push_back(rowIndex);
    columnValues.push_back(val);
  }

  return true;
}

bool MPSReader::computeBoundValue(index_t &outCol,
                                  double &outVal,
                                  Problem &problem,
                                  const std::string &colName,
                                  const std::string &value) {
  auto colIt = problem.colToIndex.find(colName);
  if(colIt == problem.colToIndex.end()){
    std::cerr<<"Could not find column: "<<colName<<" in BOUNDS section, line: "<<lineCount<<"\n";
    return false;
  }
  outCol = colIt->second;
  outVal = std::stod(value); //TODO: error handling

  return true;
}
bool MPSReader::addRHS(Problem& problem,
                       const std::string &rowName, const std::string &value) {
  auto it = problem.rowToIndex.find(rowName);
  if(it == problem.rowToIndex.end()){
    std::cerr<<"Row: "<< rowName <<"was not yet declared, line "<<lineCount<<"\n";
    return false;
  }
  index_t index = it->second;
  double val = std::stod(value); //TODO: check
  if(it->second == INVALID){
    //Row is free
    auto freeIt = freeRows.find(rowName);
    if(freeIt == freeRows.end()){
      std::cerr<<"Could not find free row: "<<rowName<<"\n";
      return false;
    }
    freeIt->second.rhs = val;
    return true;
  }

  if(problem.lhs[index] != -infinity){
    problem.lhs[index] = val;
  }
  if(problem.rhs[index] != infinity){
    problem.rhs[index] = val;
  }
  return true;
}
bool MPSReader::addRange(Problem &problem, const std::string &rowName, const std::string &value) {
  auto it = problem.rowToIndex.find(rowName);
  if(it == problem.rowToIndex.end()){
    std::cerr<<"Could not find row: "<<rowName<<" in RANGES section, line: "<<lineCount<<"\n";
    return false;
  }
  index_t rowIndex = it->second;
  if(rowIndex == INVALID){
    //Row is free; ranges does not make any sense
    std::cerr<<"Can not specify RANGES for free row, line: "<<lineCount<<"\n";
    return false;
  }
  double val= std::stod(value);

  bool lhsInfinite = problem.lhs[rowIndex] == -infinity;
  bool rhsInfinite = problem.rhs[rowIndex] == infinity;
  if(lhsInfinite && !rhsInfinite){
    problem.lhs[rowIndex] = problem.rhs[rowIndex] - fabs(val);
  }else if(rhsInfinite && !lhsInfinite){
    problem.rhs[rowIndex] = problem.lhs[rowIndex] + fabs(val);
  }else if (!rhsInfinite && !lhsInfinite) {
    if(val>= 0.0){
      problem.rhs[rowIndex] += val;
    }else{
      problem.lhs[rowIndex] += val;
    }
  }else{
    std::cerr<<"Can not specify RANGES for free row, line: "<<lineCount<<"\n";
    return false;
  }


  return true;
}

bool MPSReader::processOBJNAMELine(const std::string &line, Problem &) {
    auto words = splitString(line);
    assert(!words.empty());
    if(words.size() > 2){
        std::cerr<<"Unexpected fields in mps file, OBJNAME section, line: "<<lineCount<<"\n";
        return false;
    }
    objectiveRowName = words[0];
    return true;
}

bool MPSReader::processOBJSENSELine(const std::string &line, Problem &problem) {
    auto words = splitString(line);
    assert(!words.empty());
    if(words.size() > 2){
        std::cerr<<"Unexpected fields in mps file, OBJSENSE section, line: "<<lineCount<<"\n";
        return false;
    }

    return processObjsenseWord(words[0],problem);
}

bool MPSReader::processObjsenseWord(const std::string &word, Problem &problem) {
    if(word == "MAX"){
        problem.sense = ObjSense::MAXIMIZE;
    }else if(word == "MIN"){
        problem.sense = ObjSense::MINIMIZE;
    }
    else{
        std::cerr<<"Unexpected input: "<<word<<" in OBJSENSE section, line: "<<lineCount<<"\n";
        return false;
    }
    return true;
}

std::optional<Problem> problemFromMPSstream(std::istream& stream){
  std::string read;
  MPSReader reader;
  Problem problem;
  while(!stream.eof()){
    std::getline(stream,read);
    if(read.ends_with('\r')){
      read.pop_back();
    }
    std::cout<<read<<"\n";
    ++reader.lineCount;
    if(read.empty() || read.starts_with('*')) continue; //Line is a comment, we ignore it
    if(!read.starts_with(' ')){
      if(!reader.processNewSection(read,problem)){
        return std::nullopt;
      }
      if(reader.section == MPSSection::ENDATA){
        break;
      }
    }else{
      if(!reader.processSectionLine(read,problem)){
        return std::nullopt;
      }
    }
  }
  if(reader.finalizeModel(problem)){
    return problem;
  }
  return std::nullopt;
}
char rowSenseChar(double lhs, double rhs){
  if(lhs == rhs){
    return 'E';
  }
  bool lhsInfinite = lhs == -infinity;
  bool rhsInfinite = rhs == infinity;
  if(lhsInfinite && !rhsInfinite){
    return 'L';
  }
  if(rhsInfinite && !lhsInfinite){
    return 'G';
  }
  if(rhsInfinite && lhsInfinite){
    return 'N';
  }
  //in case the row has two bounded sides, we treat it as a less than or equal row
  return 'L';
}

bool problemToStream(const Problem& problem,std::ostream& stream){
  stream <<std::setprecision(15);
  assert(problem.matrix.format == SparseMatrixFormat::COLUMN_WISE);
  stream << sectionToString(MPSSection::NAME)<< "  "<<problem.name<<"\n";
  if(!stream.good()){
    return false;
  }
  if(problem.sense == ObjSense::MAXIMIZE){
      stream<<sectionToString(MPSSection::OBJSENSE)<<"\n";
      stream<<"  MAX\n";
  }

  //rows section
  stream << sectionToString(MPSSection::ROWS)<<"\n";
  stream <<"  "<<"N"<<"  "<<"obj\n";
  for(index_t i = 0; i < problem.numRows; ++i){
    stream<<"  "<<rowSenseChar(problem.lhs[i],problem.rhs[i])<<"  "<<problem.rowNames[i]<<"\n";
  }

  if(!stream.good()){
    return false;
  }

  stream << sectionToString(MPSSection::COLUMNS)<<"\n";
  bool inIntegralSection = false;
  std::size_t integralIndex = 0;
  for(index_t col = 0; col < problem.numCols; ++col){
    //TODO: write implicit integers as cont or integer vars?, for now we treat as continuous
    if(inIntegralSection &&
    problem.colType[col] == VariableType::CONTINUOUS ||
    problem.colType[col] == VariableType::IMPLIED_INTEGER){
      stream<<"  MARK"<<integralIndex<<"  'MARKER'      'INTEND'\n";
      inIntegralSection = false;
      ++integralIndex;
    }
    if(!inIntegralSection &&
    problem.colType[col] != VariableType::CONTINUOUS && problem.colType[col] != VariableType::IMPLIED_INTEGER){
      stream<<"  MARK"<<integralIndex<<"  'MARKER'      'INTORG'\n";
      inIntegralSection = true;
      ++integralIndex;
    }
    const std::string& name = problem.colNames[col];

    index_t nonzeros = problem.matrix.primaryNonzeros(col);
    const index_t * colRows = problem.matrix.primaryIndices(col);
    const double * colValues = problem.matrix.primaryValues(col);
    for(index_t i = 0; i < nonzeros; i+=2){
      stream<<"  "<<name<<"  "<<problem.rowNames[colRows[i]]<<"  "<<colValues[i];
      if(i== nonzeros-1){
        stream<<"\n";
        break;
      }
      stream<<"  "<<problem.rowNames[colRows[i+1]]<<"  "<<colValues[i+1]<<"\n";
    }
    if(problem.obj[col] != 0.0){
      stream<<"  "<<name<<"  obj  "<<problem.obj[col]<<"\n";
    }
  }
  if(inIntegralSection){
    stream<<"  MARK"<<integralIndex<<"  'MARKER'      'INTEND'\n";
    inIntegralSection = false;
    ++integralIndex;
  }

  if(!stream.good()){
    return false;
  }
  stream<<sectionToString(MPSSection::RHS)<<"\n";

  //TODO: writing as two lines might save some filespace
  for(index_t row = 0; row < problem.numRows; ++row){
    bool lhsInfinite = problem.lhs[row] == -infinity;
    bool rhsInfinite = problem.rhs[row] == infinity;
    //We skip free rows
    if(lhsInfinite && rhsInfinite) continue;
    double rhs = problem.rhs[row];
    if(rhsInfinite && !lhsInfinite){
      rhs = problem.lhs[row];
    }
    stream<<"  rhs  "<<problem.rowNames[row]<<" "<<rhs<<"\n";
  }
  if(problem.objectiveOffset != 0.0){
    stream<<"  rhs  obj  "<<-problem.objectiveOffset<<"\n";
  }
  std::size_t numWrittenRanges = 0;
  //TODO: inefficient file storing
  for(index_t row = 0; row < problem.numRows; ++row){
    if(problem.lhs[row] != -infinity && problem.rhs[row] != infinity &&
       problem.lhs[row] != problem.rhs[row]){
      if(numWrittenRanges == 0){
        stream<<sectionToString(MPSSection::RANGES)<<"\n";
      }
      ++numWrittenRanges;
      stream<<"  rhs  "<<problem.rowNames[row]<<"  "<<problem.rhs[row]-problem.lhs[row] <<"\n";
    }
  }
  if(!stream.good()){
    return false;
  }
  //TODO: finalize, along with OBJSENSE and OBJNAME sections
  stream<<sectionToString(MPSSection::BOUNDS)<<"\n";
  for(index_t col = 0; col < problem.numCols; ++col){
    if(problem.colType[col] == VariableType::BINARY ||
        (problem.colType[col] == VariableType::INTEGER &&
        isFeasEq(problem.lb[col],0.0) && isFeasEq(problem.ub[col],1.0))){
      stream<<"  BV Bound  "<<problem.colNames[col]<<"\n";
      continue;
    }
    if(problem.lb[col] == -infinity && problem.ub[col] == infinity){
      stream<<"  FR Bound  "<<problem.colNames[col]<<"\n";
      continue;
    }
    if(isFeasEq(problem.lb[col],problem.ub[col])){
      stream<<"  FX Bound  "<<problem.colNames[col]<<"  "<<problem.lb[col]<<"\n";
      continue;
    }
    //print lower and upper bound
    if(problem.lb[col] == -infinity){
      stream<<"  MI Bound  "<<problem.colNames[col]<<"\n";
    }else{
      stream<<"  LO Bound  "<<problem.colNames[col]<<"  "<<problem.lb[col]<<"\n";
    }
    if(problem.ub[col] == infinity){
      stream<<"  PL Bound  "<<problem.colNames[col]<<"\n";
    }else{
      stream<<"  UP Bound  "<<problem.colNames[col]<<"  "<<problem.ub[col]<<"\n";
    }
  }
  stream<<sectionToString(MPSSection::ENDATA);

  return stream.good();
}

bool problemToStreamCompressed(const Problem& problem, std::ostream& stream){
  boost::iostreams::filtering_ostream os;
  os.push(boost::iostreams::gzip_compressor());
  os.push(stream);
  bool good = problemToStream(problem,os);
  boost::iostreams::close(os);
  return good;
}
bool writeMPSFile(const Problem& problem,const std::filesystem::path& path){
  std::ofstream stream(path);
  return problemToStreamCompressed(problem,stream);
}