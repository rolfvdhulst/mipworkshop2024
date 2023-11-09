//
// Created by rolf on 6-8-23.
//

#include "mipworkshop2024/presolve/PostSolveStack.h"
void PostSolveStack::totallyUnimodularColumnSubmatrix(const TotallyUnimodularColumnSubmatrix& submatrix)
{
	reductions.emplace_back(submatrix);
    containsTUSubmatrix = true;
}

bool PostSolveStack::totallyUnimodularColumnSubmatrixFound() const {
    return containsTUSubmatrix;
}

nlohmann::json submatToJson(const TotallyUnimodularColumnSubmatrix& submat){
    nlohmann::json json;
    json["submatColumns"] = submat.submatColumns;
    json["implyingColumns"] = submat.implyingColumns;
    json["submatRows"] = submat.submatRows;
    return json;
}

TotallyUnimodularColumnSubmatrix submatFromJson(const nlohmann::json& json){
    TotallyUnimodularColumnSubmatrix submat;
    if(json.contains("submatColumns")){
        for(const auto& col : json["submatColumns"]){
            submat.submatColumns.push_back(col);
        }
    }

    if(json.contains("implyingColumns")){
        for(const auto& col : json["implyingColumns"]){
            submat.implyingColumns.push_back(col);
        }
    }

    if(json.contains("submatRows")){
        for(const auto& row : json["submatRows"]){
            submat.submatRows.push_back(row);
        }
    }

    return submat;
}
nlohmann::json postSolveToJson(const PostSolveStack& stack){
    nlohmann::json json;
    for(const auto& reduction : stack.reductions){
        json["reductions"].push_back(submatToJson(reduction));
    }
    json["containsTUSubmatrix"] = stack.containsTUSubmatrix;
    return json;
}

PostSolveStack postSolveFromJson(const nlohmann::json& json){
    PostSolveStack stack;
    if(json.contains("reductions")){
        for(const auto& reduction : json["reductions"]){
            stack.reductions.push_back(submatFromJson(reduction));
        }
    }
    stack.containsTUSubmatrix = json["containsTUSubmatrix"];

    return stack;
}