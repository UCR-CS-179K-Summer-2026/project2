
#pragma once

using namespace std;


enum class QueryType {
    DotPath, //a.b.c..
    Filter //GET FROM using operators..
};


//=, !=, <, <=, >, >=, operators for filter query
enum class ComparisonOperator {
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual
};


///path components, storing keys ex: "employee", arr index, and allElement filter ([*])
enum class PathPartType {
    Key,
    ArrayIndex,
	AllElement
};

struct PathPart {
    PathPartType type; //key, arrindex, [*]
    string key;
    int index = -1;
};

struct DotPathQuery { 
    vector<PathPart> path;
};

struct FilterQuery {
    vector<PathPart> selectField; //get
    vector<PathPart> sourcePath; //from
    vector<Condition> conditions; //where
};

// add filter query conditions with operators

struct Query {
    QueryType type;

    DotPathQuery dotPath;
    FilterQuery filter;
};

