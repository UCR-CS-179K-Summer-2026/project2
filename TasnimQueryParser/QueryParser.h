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
    // Checks whether `keyword` occurs at position i AND is followed by whitespace or end-of-string (not another letter). Prevents false matches like "FROMX" being read as the FROM keyword.
    bool matchKeyword(const std::string& input, std::size_t i, const std::string& keyword) const;

    void skipWhitespace(const std::string& input, std::size_t& i) const;
};