//
// Created by rolf on 14-11-23.
//

#include <mipworkshop2024/Logging.h>
#include <iostream>
#include <fstream>
std::unordered_map<std::string,ProblemLogData> getFileResults(std::string path,std::string endsWith){
    std::unordered_map<std::string,ProblemLogData> stats;
    for(const auto& file : std::filesystem::recursive_directory_iterator(path)){
        if(!(file.is_regular_file() && file.path().extension() == ".json")) continue;
        if(!endsWith.empty() && !file.path().stem().string().ends_with(endsWith)) continue;
        nlohmann::json json;
        std::ifstream stream(file.path());
        stream >> json;

        auto data = ProblemLogData::fromJson(json);
        auto name = file.path().stem().filename().string();
        std::string trimmed_name = name.substr(0,name.size()-endsWith.size());
        stats[trimmed_name] = data;
    }

    return stats;
}

double shiftedGeometricMean(const std::vector<double>& values, double shift){
    if(values.empty()){
        return 0.0;
    }
    double sum = 0.0;
    for(const auto& value : values){
        sum += std::log(value + shift);
    }
    sum /= values.size();

    return std::exp(sum)-shift;
}
double geometricMean(const std::vector<double>& values){
    if(values.empty()){
        return 0.0;
    }
    double sum = 0.0;
    for(const auto& value : values){
        sum += std::log(value);
    }
    sum /= values.size();

    return std::exp(sum);
}

double totalPrimalIntegral(const SolveStatistics& stats){
    return 100.0*stats.TUDetectionTime + stats.primalDualIntegral;
}
double absGap(const SolveStatistics& stats){
    return fabs(stats.primalBound-stats.dualBound);
}
double relGap(const SolveStatistics& stats){
    if(isInfinite(stats.primalBound)){
        return infinity;
    }
    if(isInfinite(-stats.dualBound)) {
        return infinity;
    }
    if(stats.isInfeasible || stats.isUnbounded){
        return 0.0;
    }
    if(isInfinite(stats.dualBound)) {
        return 0.0;
    }
    if(isFeasEq(stats.primalBound,stats.dualBound)){
        return 0.0;
    }
    double absDual = std::abs(stats.dualBound);
    double absPrimal = std::abs(stats.primalBound);
    if(isFeasEq(absDual,0.0) || isFeasEq(absPrimal,0.0) || isInfinite(absDual) || isInfinite(absPrimal) ||
    stats.dualBound * stats.primalBound < 0.0 ){
        return infinity;
    }

    return std::abs((stats.primalBound-stats.dualBound) / std::min(absDual,absPrimal));
}

std::size_t gapCount(const std::vector<double>& numbers, double limit){
    return std::count_if(numbers.begin(),numbers.end(),[&](double value){ return value < limit;}
    );
}
int main(int argc, char ** argv){
    std::vector<std::string> input(argv,argv+argc);

    if(input.size() != 2){
        std::cerr<<"Wrong number of arguments!\n";
        return EXIT_FAILURE;
    }
    const auto& folder = input[1];
    auto baseline = getFileResults(folder,"_b");
    auto full = getFileResults(folder,"_c");
    auto partial = getFileResults(folder,"_d");
    auto toContOnly = getFileResults(folder,"_e");
    auto toIntOnly = getFileResults(folder,"_f");

    std::vector<double> baseSolveTimes;
    std::vector<double> fullSolveTimes;
    std::vector<double> partialSolveTimes;
    std::vector<double> contSolveTimes;
    std::vector<double> intSolveTimes;

    std::vector<double> baseNodes;
    std::vector<double> fullNodes;
    std::vector<double> partialNodes;
    std::vector<double> contNodes;
    std::vector<double> intNodes;

    std::vector<double> baseAbsGap;
    std::vector<double> fullAbsGap;
    std::vector<double> partialAbsGap;
    std::vector<double> contAbsGap;
    std::vector<double> intAbsGap;

    std::vector<double> baseRelGap;
    std::vector<double> fullRelGap;
    std::vector<double> partialRelGap;
    std::vector<double> contRelGap;
    std::vector<double> intRelGap;

    std::vector<double> baseScore;
    std::vector<double> fullScore;
    std::vector<double> partialScore;
    std::vector<double> contScore;
    std::vector<double> intScore;

    for(const auto& pair : baseline){
        if(pair.first == "brazil3" || pair.first == "irish-electricity" || pair.first == "graph20-20-1rand") continue;
        //TODO: these instances have problems? (brazil3; bug reported, irish-electricity ? graph20-20-1rand?
        const auto& baseRes = pair.second;
        const auto& fullRes = full.at(pair.first);
        const auto& partialRes = partial.at(pair.first);
        const auto& contRes = toContOnly.at(pair.first);
        const auto& intRes = toIntOnly.at(pair.first);
        if(!baseRes.solveStatistics.has_value()){
            std::cout<<"No results: "<<pair.first<<"\n";
            continue;
        }
        const auto& baseStats = baseRes.solveStatistics.value();
        const auto& fullStats = fullRes.solveStatistics.value_or(baseStats);
        const auto& partialStats = partialRes.solveStatistics.value_or(baseStats);
        const auto& contStats = contRes.solveStatistics.value_or(baseStats);
        const auto& intStats = intRes.solveStatistics.value_or(baseStats);

        baseSolveTimes.push_back(baseStats.solvedOptimally ? baseStats.timeTaken : 3600.0);
        fullSolveTimes.push_back(fullStats.solvedOptimally ? fullStats.timeTaken : 3600.0);
        partialSolveTimes.push_back(partialStats.solvedOptimally ? partialStats.timeTaken : 3600.0);
        contSolveTimes.push_back(contStats.solvedOptimally ? contStats.timeTaken : 3600.0);
        intSolveTimes.push_back(intStats.solvedOptimally ? intStats.timeTaken : 3600.0);

        baseNodes.push_back(baseStats.nodes);
        fullNodes.push_back(fullStats.nodes);
        partialNodes.push_back(partialStats.nodes);
        contNodes.push_back(contStats.nodes);
        intNodes.push_back(intStats.nodes);


        if (absGap(baseStats) < 1e19) { baseAbsGap.push_back(absGap(baseStats)); }
        if (absGap(fullStats) < 1e19) { fullAbsGap.push_back(absGap(fullStats)); }
        if (absGap(partialStats) < 1e19) { partialAbsGap.push_back(absGap(partialStats)); }
        if (absGap(contStats) < 1e19) { contAbsGap.push_back(absGap(contStats)); }
        if (absGap(intStats) < 1e19) { intAbsGap.push_back(absGap(intStats)); }

        baseRelGap.push_back(relGap(baseStats));
        fullRelGap.push_back(relGap(fullStats));
        partialRelGap.push_back(relGap(partialStats));
        contRelGap.push_back(relGap(contStats));
        intRelGap.push_back(relGap(intStats));

        baseScore.push_back(totalPrimalIntegral(baseStats));
        fullScore.push_back(totalPrimalIntegral(fullStats));
        partialScore.push_back(totalPrimalIntegral(partialStats));
        contScore.push_back(totalPrimalIntegral(contStats));
        intScore.push_back(totalPrimalIntegral(intStats));

    }

    double baseMean = shiftedGeometricMean(baseSolveTimes,10);
    double fullMean = shiftedGeometricMean(fullSolveTimes,10);
    double partialMean = shiftedGeometricMean(partialSolveTimes,10);
    double contMean = shiftedGeometricMean(contSolveTimes,10);
    double intMean = shiftedGeometricMean(intSolveTimes,10);

    double nodeShift = 10.0;
    double nodeBase = shiftedGeometricMean(baseNodes,nodeShift);
    double nodeFull = shiftedGeometricMean(fullNodes,nodeShift);
    double nodePartial = shiftedGeometricMean(partialNodes,nodeShift);
    double nodeCont = shiftedGeometricMean(contNodes,nodeShift);
    double nodeInt = shiftedGeometricMean(intNodes,nodeShift);

    double absShift = 0.1;
    double baseAbs = shiftedGeometricMean(baseAbsGap,absShift);
    double fullAbs = shiftedGeometricMean(fullAbsGap,absShift);
    double partialAbs = shiftedGeometricMean(partialAbsGap,absShift);
    double contAbs = shiftedGeometricMean(contAbsGap,absShift);
    double intAbs = shiftedGeometricMean(intAbsGap,absShift);

    double scoreShift = 10.0;
    double baseScoreMean = shiftedGeometricMean(baseScore,scoreShift);
    double fullScoreMean = shiftedGeometricMean(fullScore,scoreShift);
    double partialScoreMean = shiftedGeometricMean(partialScore,scoreShift);
    double contScoreMean = shiftedGeometricMean(contScore,scoreShift);
    double intScoreMean = shiftedGeometricMean(intScore,scoreShift);
    std::cout<<"Baseline, Up+Down Implied Integer, Up Implied Integer, Down to Cont, Up to Integer\n";
    std::cout<<"Geometric mean time\n";
    std::cout<<baseMean<<", "<<fullMean<<", "<<partialMean<<", "<<contMean<<", "<<intMean<<"\n";

    std::cout<<"Mean nodes\n";
    std::cout<<nodeBase<<", "<<nodeFull<<", "<<nodePartial<<", "<<nodeCont<<", "<<nodeInt<<"\n";

    std::cout<<"Mean absolute gap\n";
    std::cout<<baseAbs<<", "<<fullAbs<<", "<<partialAbs<<", "<<contAbs<<", "<<intAbs<<"\n";

    std::cout<<"Score (Total PDI)\n";
    std::cout<<baseScoreMean<<", "<<fullScoreMean<<", "<<partialScoreMean<<", "<<contScoreMean<<", "<<intScoreMean<<"\n";

    double gapLimit = 0.01;
    std::size_t baseGapCount5 = gapCount(baseRelGap,gapLimit);
    std::size_t fullGapCount5 = gapCount(fullRelGap,gapLimit);
    std::size_t partialGapCount5 = gapCount(partialRelGap,gapLimit);
    std::size_t contGapCount5 = gapCount(contRelGap,gapLimit);
    std::size_t intGapCount5 = gapCount(intRelGap,gapLimit);

    std::cout<<"Number of problems with gap < "<<gapLimit*100.0 <<"% ( out of "<<baseRelGap.size()<<" )\n";
    std::cout<<baseGapCount5<<", "<< fullGapCount5<<", "<< partialGapCount5 <<", "<< contGapCount5 <<", "<< intGapCount5<<"\n";


    for(const auto& pair : baseline){
        if(pair.first == "brazil3" || pair.first == "irish-electricity" || pair.first == "graph20-20-1rand") continue; //TODO: these instances have problems...
        const auto& baseRes = pair.second;
        const auto& fullRes = full.at(pair.first);
        const auto& partialRes = partial.at(pair.first);
        const auto& contRes = toContOnly.at(pair.first);
        const auto& intRes = toIntOnly.at(pair.first);
        if(!baseRes.solveStatistics.has_value()){
            std::cout<<"No results: "<<pair.first<<"\n";
            continue;
        }
        const auto& baseStats = baseRes.solveStatistics.value();
        const auto& fullStats = fullRes.solveStatistics.value_or(baseStats);
        const auto& partialStats = partialRes.solveStatistics.value_or(baseStats);
        const auto& contStats = contRes.solveStatistics.value_or(baseStats);
        const auto& intStats = intRes.solveStatistics.value_or(baseStats);

        std::cout<<std::setw(30)<<pair.first<<": "<<std::setw(8)<< baseStats.timeTaken<<", "<<std::setw(8)<<fullStats.timeTaken<<", "
                <<std::setw(8)<<partialStats.timeTaken<<", "<<std::setw(8)<<contStats.timeTaken<<", "<<std::setw(8)<<intStats.timeTaken<<"\n";
    }

    std::cout<<"\nGap for all instances\n";
    for(const auto& pair : baseline){
        if(pair.first == "brazil3" || pair.first == "irish-electricity" || pair.first == "graph20-20-1rand") continue; //TODO: these instances have problems...
        const auto& baseRes = pair.second;
        const auto& fullRes = full.at(pair.first);
        const auto& partialRes = partial.at(pair.first);
        const auto& contRes = toContOnly.at(pair.first);
        const auto& intRes = toIntOnly.at(pair.first);
        if(!baseRes.solveStatistics.has_value()){
            std::cout<<"No results: "<<pair.first<<"\n";
            continue;
        }
        const auto& baseStats = baseRes.solveStatistics.value();
        const auto& fullStats = fullRes.solveStatistics.value_or(baseStats);
        const auto& partialStats = partialRes.solveStatistics.value_or(baseStats);
        const auto& contStats = contRes.solveStatistics.value_or(baseStats);
        const auto& intStats = intRes.solveStatistics.value_or(baseStats);

        std::cout<<std::setw(30)<<pair.first<<": "<<std::setw(8)<< absGap(baseStats)<<", "<<std::setw(8)<<absGap(fullStats)<<", "
                 <<std::setw(8)<<absGap(partialStats)<<", "<<std::setw(8)<<absGap(contStats)<<", "<<std::setw(8)<<absGap(intStats)<<"\n";
    }
    std::cout<<"\nScore for all instances\n";
    for(const auto& pair : baseline){
        if(pair.first == "brazil3" || pair.first == "irish-electricity" || pair.first == "graph20-20-1rand") continue; //TODO: these instances have problems...
        const auto& baseRes = pair.second;
        const auto& fullRes = full.at(pair.first);
        const auto& partialRes = partial.at(pair.first);
        const auto& contRes = toContOnly.at(pair.first);
        const auto& intRes = toIntOnly.at(pair.first);
        if(!baseRes.solveStatistics.has_value()){
            std::cout<<"No results: "<<pair.first<<"\n";
            continue;
        }
        const auto& baseStats = baseRes.solveStatistics.value();
        const auto& fullStats = fullRes.solveStatistics.value_or(baseStats);
        const auto& partialStats = partialRes.solveStatistics.value_or(baseStats);
        const auto& contStats = contRes.solveStatistics.value_or(baseStats);
        const auto& intStats = intRes.solveStatistics.value_or(baseStats);

        std::cout<<std::setw(30)<<pair.first<<": "<<std::setw(8)<< totalPrimalIntegral(baseStats)<<", "<<std::setw(8)<<totalPrimalIntegral(fullStats)<<", "
                 <<std::setw(8)<<totalPrimalIntegral(partialStats)<<", "<<std::setw(8)<<totalPrimalIntegral(contStats)<<", "<<std::setw(8)<<totalPrimalIntegral(intStats)<<"\n";
    }

}
