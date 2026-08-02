#pragma once

#include <string>
#include <vector>

enum class QueryType {
    DotPath
};

//allpossible components of a path, 
//[*] means every element in array
enum class PathPartType {
    Key,
    ArrayIndex,
    AllElements
};

// Represents one component of a JSON path.
struct PathPart {
    PathPartType type;
    std::string key;
    //when we're using array index
    int index = -1;
};

struct DotPathQuery {
    std::vector<PathPart> path;
};

struct Query {
    QueryType type = QueryType::DotPath;
    DotPathQuery dotPath;
};