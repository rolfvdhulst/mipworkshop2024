//
// Created by rolf on 8-8-23.
//

#include "mipworkshop2024/Logging.h"
nlohmann::json statisticsToJson(const SolveStatistics& stats){
	nlohmann::json values;
	values["primalBound"] = stats.primalBound;
	values["dualBound"] = stats.dualBound;
	values["rootLPValue"] = stats.rootLPValue;
	values["timeTaken"] = stats.timeTaken;
	values["nodes"] = stats.nodes;
	values["solvedOptimally"] = stats.solvedOptimally;
	values["numImpliedAfterPresolve"] = stats.numImpliedAfterPresolve;
	values["numSolutions"] = stats.numSolutions;
	values["numTUImpliedColumns"] = stats.numTUImpliedColumns;

	values["numBinaryAfterPresolve"] = stats.numBinaryAfterPresolve;
	values["numIntAfterPresolve"] = stats.numIntAfterPresolve;
	values["numImpliedAfterPresolve"] = stats.numImpliedAfterPresolve;
	values["numContinuousAfterPresolve"] = stats.numContinuousAfterPresolve;
	values["numConsAfterPresolve"] = stats.numConsAfterPresolve;

	values["numBinaryBeforePresolve"] = stats.numBinaryBeforePresolve;
	values["numIntBeforePresolve"] = stats.numIntBeforePresolve;
	values["numImpliedBeforePresolve"] = stats.numImpliedBeforePresolve;
	values["numContinuousBeforePresolve"] = stats.numContinuousBeforePresolve;
	values["numConsBeforePresolve"] = stats.numConsBeforePresolve;

	 values["problemName"] = stats.problemName;
	 values["isUnbounded"] = stats.isUnbounded;
	 values["isInfeasible"] = stats.isInfeasible;
	 values["TUDetectionTime"] = stats.TUDetectionTime;
	return values;
}
SolveStatistics statisticsFromJson(const nlohmann::json& json){
	SolveStatistics stats;
	stats.primalBound = json["primalBound"];
	stats.dualBound = json["dualBound"];
	stats.rootLPValue = json["rootLPValue"];
	stats.timeTaken = json["timeTaken"];
	stats.nodes = json["nodes"];
	stats.solvedOptimally = json["solvedOptimally"];
	stats.numSolutions = json["numSolutions"];
	stats.numTUImpliedColumns = json["numTUImpliedColumns"];
	stats.numBinaryAfterPresolve = json["numBinaryAfterPresolve"];
	stats.numIntAfterPresolve = json["numIntAfterPresolve"];
	stats.numImpliedAfterPresolve = json["numImpliedAfterPresolve"];
	stats.numContinuousAfterPresolve = json["numContinuousAfterPresolve"];
	stats.numConsAfterPresolve = json["numConsAfterPresolve"];
	stats.numBinaryBeforePresolve = json["numBinaryBeforePresolve"];
	stats.numIntBeforePresolve = json["numIntBeforePresolve"];
	stats.numImpliedBeforePresolve = json["numImpliedBeforePresolve"];
	stats.numContinuousBeforePresolve = json["numContinuousBeforePresolve"];
	stats.numConsBeforePresolve = json["numConsBeforePresolve"];

	stats.problemName = json["problemName"];
	stats.isInfeasible = json["isInfeasible"];
	stats.isUnbounded = json["isUnbounded"];
	stats.TUDetectionTime = json["TUDetectionTime"];
	return stats;
}
nlohmann::json ProblemLogData::toJson() const
{
	nlohmann::json json;
	if(presolvedStatistics.has_value()){
		json["presolvedStatistics"] = statisticsToJson(presolvedStatistics.value());
	}
	if(baseLineStatistics.has_value()){
		json["baseLineStatistics"] = statisticsToJson(baseLineStatistics.value());
	}

    json["numUpgraded"] = numUpgraded;
    json["numDowngraded"] = numDowngraded;
    json["writeType"] = writeType;

	return json;
}

ProblemLogData ProblemLogData::fromJson(const nlohmann::json& json)
{
	ProblemLogData data;
	if(json.contains("baseLineStatistics")){
		data.baseLineStatistics = statisticsFromJson(json["baseLineStatistics"]);
	}
	if(json.contains("presolvedStatistics")) {
        data.presolvedStatistics = statisticsFromJson(json["presolvedStatistics"]);
    }

    data.numUpgraded = json["numUpgraded"];
    data.numDowngraded = json["numDowngraded"];
    data.writeType = json["writeType"];
    
	return data;
}