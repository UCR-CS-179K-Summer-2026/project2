#include "QueryParser.h"

#include <cctype>
#include <stdexcept>

using namespace std;


Query QueryParser::parse(const string& input) const {
    if (input.empty()) {
        throw runtime_error("Query is empty.");
    }
    
    Query query;

    if (input.rfind("GET ", 0) == 0) { //if it starts with GET, filter parser
        query.type = QueryType::Filter;
        query.filter = parseFilterQuery(input);
    }
    else {
        query.type = QueryType::DotPath;
        query.dotPath = parseDotPath(input);
    }

    return query;
}

DotPathQuery QueryParser::parseDotPath(const string& input) const {
    //updated dot path, put prev code in helper parse function to minimize repeated code in filter
    DotPathQuery query;
    
    size_t i = 0;
    
    query.path = parsePath(input, i);
    
    while (i < input.size() && std::isspace(static_cast<unsigned char>(input[i]))) {
        ++i;
    }

    if (i < input.size()) {
        throw runtime_error("Unexpected trailing token inside dot path expression");
    }

    return query;
}

//note: helper function to skip spaces for filter parser
void QueryParser::skipWhitespace(const std::string& input, std::size_t& i) const {
    while (i < input.size() && isspace(static_cast<unsigned char>(input[i]))) {
        ++i;
    }
}


FilterQuery QueryParser::parseFilterQuery(const std::string& input) const { //GET, FROM, WHERE
    FilterQuery filterQuery;
    size_t i = 0;

    //skipping extra spaces before comparing
    skipWhitespace(input, i);

    //GET comparison

    if (input.compare(i, 3, "GET") != 0) {
        throw std::runtime_error(
            "filter query needs to start with GET"
        );
    }
    i += 3;
    
    skipWhitespace(input, i);

    filterQuery.selectField = parsePath(input, i);
    
    //check if valid entrance after GET
    if (filterQuery.selectField.empty()) {
        throw runtime_error("expected field after GET");
    }

    skipWhitespace(input, i);

    //FROM comparison, following GET
    if (input.compare(i, 4, "FROM") != 0) {
        throw std::runtime_error(
            "Expected FROM"
        );
    }
    i += 4;

    skipWhitespace(input, i);
    
    filterQuery.sourcePath = parsePath(input, i);
    
    if (filterQuery.sourcePath.empty()) { //no source to get from
        throw runtime_error("missing source path after 'FROM'");
    }

    skipWhitespace(input, i);

    //WHERE

    if (input.compare(i, 5, "WHERE") == 0) { //where filter not mandatory for query to run, available choice
        i += 5;

        Condition whereCond;

        //left-hand parsing
        skipWhitespace(input, i);
        
        whereCond.field = parsePath(input, i);
        
        if (whereCond.field.empty()) {
            throw runtime_error("expected input after WHERE filter");
        }

        skipWhitespace(input, i);
        
        string operatortext;
        
        //found one of these operators
        while (i < input.size() && (input[i] == '=' || input[i] == '!' || input[i] == '<' || input[i] == '>')) { 
            operatortext += input[i]; //load operator text
            ++i;
        }
        
        if (operatortext.empty()) {
            throw runtime_error("expecting a comparison operator after WHERE filter");
        }
        
        whereCond.comparisonOp = parseOperator(operatortext);

        //right hand

        skipWhitespace(input, i);

        if (i >= input.size()) { //check if there's actually a number after the operator
            throw runtime_error("expected number after operator");
        }

        string readVal;
        
        while (i < input.size() && !isspace(static_cast<unsigned char>(input[i]))) {
            readVal += input[i];
            ++i;
        }
        
        whereCond.value = readVal;

        filterQuery.conditions.push_back(whereCond);
    }
    
    skipWhitespace(input, i);

    if (i < input.size()) {
        throw runtime_error("unread/extra input located at index: " + to_string(i));
    }

    return filterQuery;
}

std::vector<PathPart> QueryParser::parsePath(const std::string& input, std::size_t& i) const { //parsing path
    std::vector<PathPart> path;

    while (i < input.size()) {

        //end parsing when hit a spaceto separate types
        if (isspace(static_cast<unsigned char>(input[i]))) {
            break;
        }

        if (!isalpha(static_cast<unsigned char>(input[i])) &&
            input[i] != '_') {
            throw runtime_error(
                "Expected key at position " + to_string(i) //throw error for invalid position placement
            );
        }

        string key;

        //reading key name:
        while (i < input.size() && (isalnum(static_cast<unsigned char>(input[i])) || 
			input[i] == '_')) {
            key += input[i];
            ++i;
        }

        path.push_back({
            PathPartType::Key,
            key,
            -1
        });

        //arr handling after key
        if (i < input.size() && input[i] == '[') { //'[' opens correctly
            ++i;

			//unclosed brackets error
            if (i >= input.size()) {
                throw runtime_error(
                    "Unclosed bracket at end of query"
                );
            }

            //wildcard handling: [*]
            if (input[i] == '*') {
                ++i;

                if (i >= input.size() || input[i] != ']') { //err for unclosed [
                    throw runtime_error(
                        "Unclosed bracket after *."
                    );
                }

                ++i;

                path.push_back({ //encompassing all elements of arr
                    PathPartType::AllElements, "", -1
                });
            }


            //specific array index handling
			else if (isdigit(static_cast<unsigned char>(input[i]))) {
                string digits;

                while (i < input.size() && isdigit(input[i])) {
					digits += input[i]; //load nums into digits to convert
					++i;
				}
				int index = stoi(digits);

                if (i >= input.size() || input[i] != ']') { //unclosed brackets
                    throw runtime_error(
                        "Unclosed bracket after array index"
                    );
                }

                ++i;

                path.push_back({
                    PathPartType::ArrayIndex, "", index
                });
            }

            else {
                throw runtime_error(
                    "error: missing array index or * in brackets."
                );
            }
        }

        if (i < input.size() && isspace(static_cast<unsigned char>(input[i]))) { //skip extra space without throwing an error
            break;
        }

        //if input isn't finished, 
        if (i < input.size() ) {
			if (input[i] != '.') {
                throw runtime_error(
                    "Error: Expected '.' at position " + to_string(i)
                );
            }

            ++i;

            if (i >= input.size()) {
                throw runtime_error(
                    "Error: Query is ending with a dot"
                );
            }
        }
    }

    //return query;
    return path;

}



FilterOperators QueryParser::parseOperator(const std::string& operatorText) const { 
    if (operatorText == "=") {
        return FilterOperators::Equal;
    }
    if (operatorText == "!=") {
        return FilterOperators::NotEqual;
    }
    if (operatorText == "<") {
        return FilterOperators::LessThan;
    }
    if (operatorText == "<=") {
        return FilterOperators::LessThanOrEqual;
    }
    if (operatorText == ">") {
        return FilterOperators::GreaterThan;
    }
    if (operatorText == ">=") {
        return FilterOperators::GreaterThanOrEqual;
    }

    throw runtime_error(
        "Unsupported operator"
    );
}