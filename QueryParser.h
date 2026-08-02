#pragma once

#include <string>

#include "QueryStruct.h"

class QueryParser {
public:
    Query parse(const std::string& input) const;  // Parses a complete query string.


private:
    DotPathQuery parseDotPath(const std::string& input) const; //dot path query parser
};