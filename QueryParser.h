#pragma once

#include <string>
#include "QueryStruct.h"

using namespace std;

class QueryParser {
public:
    Query parse(const string& input) const;  // Parses a complete query string.


private:
    DotPathQuery parseDotPath(const string& input) const; //dot path query parser
};