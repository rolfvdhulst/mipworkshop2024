//
// Created by rolf on 8-8-23.
//

#include "mipworkshop2024/Logging.h"
nlohmann::json detectionStatisticsToJson(const DetectionStatistics& stats){
    nlohmann::json values;

    values["method"] = stats.method;
    values["timeTaken"] = stats.timeTaken;
    values["numUpgraded"] = stats.numUpgraded;
    values["numDowngraded"] = stats.numDowngraded;
    values["numErasedComponents"] = stats.numErasedComponents;
    values["numComponents"] = stats.numComponents;
    values["numNodesTypeS"] = stats.numNodesTypeS;
    values["numNodesTypeP"] = stats.numNodesTypeP;
    values["numNodesTypeQ"] = stats.numNodesTypeQ;
    values["numNodesTypeR"] = stats.numNodesTypeR;
    values["largestRNumArcs"] = stats.largestRNumArcs;
    values["numTotalRArcs"] = stats.numTotalRArcs;
    values["numRows"] = stats.numRows;
    values["numColumns"] = stats.numColumns;

    return values;
}
DetectionStatistics detectionStatisticsFromJson(const nlohmann::json& json){
    DetectionStatistics stats;
    stats.method = json["method"];
    stats.timeTaken = json["timeTaken"];
    stats.numUpgraded = json["numUpgraded"];
    stats.numDowngraded = json["numDowngraded"];
    stats.numErasedComponents = json["numErasedComponents"];
    stats.numComponents = json["numComponents"];
    stats.numNodesTypeS = json["numNodesTypeS"];
    stats.numNodesTypeP = json["numNodesTypeP"];
    stats.numNodesTypeQ = json["numNodesTypeQ"];
    stats.numNodesTypeR = json["numNodesTypeR"];
    stats.largestRNumArcs = json["largestRNumArcs"];
    stats.numTotalRArcs = json["numTotalRArcs"];
    stats.numRows = json["numRows"];
    stats.numColumns = json["numColumns"];

    return stats;
}
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

     values["primalDualIntegral"] = stats.primalDualIntegral;
     values["avgPDI"] = stats.avgPDI;
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
    if(json.contains("primalDualIntegral")){
        stats.primalDualIntegral = json["primalDualIntegral"];
    }else{
        stats.primalDualIntegral = infinity;
    }
    if(json.contains("avgPDI")){
        stats.avgPDI = json["avgPDI"];
    }else{
        stats.avgPDI = infinity;
    }
	return stats;
}
nlohmann::json ProblemLogData::toJson() const
{
	nlohmann::json json;
	if(solveStatistics.has_value()){
		json["solveStatistics"] = statisticsToJson(solveStatistics.value());
	}

    json["numUpgraded"] = numUpgraded;
    json["numDowngraded"] = numDowngraded;
    json["writeType"] = writeType;
    json["doDownGrade"] = doDownGrade;

    auto array = nlohmann::json::array();
    for(const auto& statistics : detectionStatistics){
        array.push_back(detectionStatisticsToJson(statistics));
    }
    json["detectionStatistics"] = array;

	return json;
}

ProblemLogData ProblemLogData::fromJson(const nlohmann::json& json)
{
	ProblemLogData data;
	if(json.contains("solveStatistics")){
		data.solveStatistics = statisticsFromJson(json["solveStatistics"]);
	}

    data.numUpgraded = json["numUpgraded"];
    data.numDowngraded = json["numDowngraded"];
    data.writeType = json["writeType"];
    data.doDownGrade = json["doDownGrade"];

    if(json.contains("detectionStatistics")){
        for (const auto& value : json["detectionStatistics"]){
            data.detectionStatistics.push_back(detectionStatisticsFromJson(value));
        }
    }
    
	return data;
}