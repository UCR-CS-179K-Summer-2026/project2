#pragma once
 
#include <string>
#include <vector>
 
using namespace std;
 
enum class QueryType {
    DotPath
};
 
//all possible parts of a path, [*]=every element in array
enum class PathPartType {
    Key,
    ArrayIndex,
    AllElements
};
 
// Represents one component of a JSON path.
struct PathPart {
    PathPartType type;
    string key;
 
    int index = -1;
};
 
struct DotPathQuery {
    std::vector<PathPart> path;
};
 
struct Query {
    QueryType type = QueryType::DotPath;
    DotPathQuery dotPath;
};