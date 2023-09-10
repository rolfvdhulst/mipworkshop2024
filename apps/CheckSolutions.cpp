//
// Created by rolf on 10-9-23.
//
#include <unordered_map>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <mipworkshop2024/Logging.h>

enum class MIPResultType{
	OPTIMAL, ///This instance has a proven optimal solution
	BEST, ///This instance has a solution with best known bound
	UNBOUNDED,///This instances is unbounded
	INFEASIBLE, ///this instance is infeasible
	UNKNOWN ///no solution is known for this instance
};
struct KnownData{
	MIPResultType type;
	double primalBound;
};
std::unordered_map<std::string,KnownData> getKnownBounds(const std::filesystem::path& path){
	std::ifstream file(path);
	std::unordered_map<std::string,KnownData> data;

	std::string instance;
	std::string type;
	while(!file.eof()){
		KnownData lineEntry{
			.primalBound = 0.0
		};

		file >> type;
		if(type == "=opt="){
			lineEntry.type = MIPResultType::OPTIMAL;
		}else if(type == "=best="){
			lineEntry.type = MIPResultType::BEST;
		}else if(type == "=inf="){
			lineEntry.type = MIPResultType::INFEASIBLE;
		}else if(type == "=unkn="){
			lineEntry.type = MIPResultType::UNKNOWN;
		}else if(type == "=unbd="){
			lineEntry.type = MIPResultType::UNBOUNDED;
		}else{
			std::cout<<"Error: invalid instance type!\n";
			return data;
		}
		file >> instance;
		if(lineEntry.type == MIPResultType::OPTIMAL || lineEntry.type == MIPResultType::BEST){
			file >> type;
			lineEntry.primalBound = std::stod(type);
		}
		data[instance] = lineEntry;
	}
	std::cout<<data.size()<<" known results\n";
	return data;
}

std::vector<ProblemLogData> getSolveResults(const std::filesystem::path& path){
	std::vector<ProblemLogData> data;
	for(const auto& file : std::filesystem::recursive_directory_iterator(path)){
		if(!(file.is_regular_file() && file.path().extension() == ".json")) continue;
		nlohmann::json json;
		std::ifstream stream(file.path());
		stream >> json;
		data.push_back(ProblemLogData::fromJson(json));
	}
	std::cout<<data.size()<<" problem results\n";
	return data;
}
bool checkStats(const SolveStatistics& stats, const std::unordered_map<std::string,KnownData>& known){
	auto it = known.find(stats.problemName);
	if(it == known.end()){
		std::cout<<"Could not find: "<<stats.problemName<<"\n";
		return false;
	}
	auto& knownData = it->second;
	if(knownData.type == MIPResultType::INFEASIBLE){
		if(stats.numSolutions > 0){
			std::cout<<"Solution for infeasible instance: "<<stats.problemName<<"\n";
			return false;
		}
	}else if(knownData.type == MIPResultType::UNKNOWN){
		if(stats.numSolutions > 0){
			std::cout<<"Found a first solution for: "<<stats.problemName<<"\n";
		}
	}else if(knownData.type == MIPResultType::UNBOUNDED){
		if(!stats.isUnbounded){
			if(stats.isInfeasible){
				std::cout<<"Unbounded instance: "<< stats.problemName<<" declared infeasible\n";
				return false;
			}
			if(stats.solvedOptimally){
				std::cout<<"Unbounded instance: "<< stats.problemName<<" declared bounded\n";
				return false;
			}
		}
	}else if(knownData.type == MIPResultType::BEST){
		if(stats.dualBound > knownData.primalBound){
			std::cout<<"Invalid dual bound for: "<<stats.problemName<<"\n";
			return false;
		}
		if(stats.primalBound < knownData.primalBound){
			std::cout<<"Found better solution for: "<<stats.problemName<<"\n";
		}
		if(stats.isInfeasible ){
			std::cout<<"Best instance: "<<stats.problemName <<" declared infeasible\n";
			return false;
		}
		if(stats.isUnbounded){
			std::cout<<"Best instance: "<<stats.problemName<<" declared unbounded\n";
		}
	}else if(knownData.type == MIPResultType::OPTIMAL){
		if(stats.dualBound > knownData.primalBound){
			std::cout<<"Invalid dual bound for: "<<stats.problemName<<"\n";
			return false;
		}
		if(stats.primalBound < knownData.primalBound){
			std::cout<<"Found better solution for: "<<stats.problemName<<", which was already optimal? \n";
			return false;
		}
		if(stats.isInfeasible || stats.isUnbounded){
			std::cout<<"Optimal instance: "<<stats.problemName <<" declared infeasible: "<<stats.isInfeasible<<" or unbounded: "<<stats.isUnbounded<<"\n";
			return false;
		}
	}
	return true;
}
bool checkProblem(const ProblemLogData& data, const std::unordered_map<std::string,KnownData>& known){
	bool good = true;
	if(data.baseLineStatistics.has_value() && !checkStats(data.baseLineStatistics.value(),known)){
		good = false;
	}
	if(data.presolvedStatistics.has_value() && !checkStats(data.presolvedStatistics.value(),known)){
		good = false;
	}
	return good;
}
int main(int argc, char ** argv){
	std::vector<std::string> args(argv,argv+argc);

	auto known = getKnownBounds(args[1]);
	auto solveResults = getSolveResults(args[2]);
	index_t correctSolutions = 0;
	for(const auto& result : solveResults){
		if(checkProblem(result,known)){
			++correctSolutions;
		}
	}
	std::cout<<correctSolutions<<" / " <<solveResults.size()<<" correct solutions\n";

}