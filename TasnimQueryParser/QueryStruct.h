#pragma once
 
#include <string>
#include <vector>
  
enum class QueryType {
    DotPath,
    Filter
};

//dot query
//all possible parts of a path, [*]=every element in array
enum class PathPartType {
    Key,
    ArrayIndex,
    AllElements
};
 
// Represents one component of a JSON path.
struct PathPart {
    PathPartType type;
    std::string key;
 
    int index = -1;
};
 
struct DotPathQuery {
    std::vector<PathPart> path;
};
 

//filter query

enum class FilterOperators {
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual
};

struct Condition {
    std::vector<PathPart> field;
    FilterOperators comparisonOp;
    
    std::string value;
};

struct FilterQuery {
    //GET... selected field
    std::vector<PathPart> selectField;

    //FROM... source path
    std::vector<PathPart> sourcePath;

    // WHERE .. (condition, comp operators)
    std::vector<Condition> conditions;
};

struct Query {
    QueryType type = QueryType::DotPath;
    
    DotPathQuery dotPath;
    FilterQuery filter;
};


//edge cases:
//empty string, emok