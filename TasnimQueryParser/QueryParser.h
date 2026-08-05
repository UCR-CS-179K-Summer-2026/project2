#pragma once

#include <string>
#include "QueryStruct.h"


class QueryParser {
public:
    Query parse(const std::string& input) const;  //parses a complete query string


private:
    DotPathQuery parseDotPath(const std::string& input) const; //dot path query parser
    
    std::vector<PathPart> parsePath(const std::string& input, std::size_t& i) const; //parsing path for both types

    FilterQuery parseFilterQuery(const std::string& input) const; //GET, FROM, WHERE

    FilterOperators parseOperator(const std::string& operatorText) const;

    void skipWhitespace(const std::string& input, std::size_t& i) const;
};