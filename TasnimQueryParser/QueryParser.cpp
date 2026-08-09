#include "QueryParser.h"

#include <cctype>
#include <stdexcept>

using namespace std;


Query QueryParser::parse(const string& input) const {
    if (input.empty()) {
        throw runtime_error("Query is empty.");
    }
    /*Fix: white space handling */
    /* Trim leading/trailing whitespace before doing anything else, so both the GET/dot-path branch check and the parsers below
     always see clean input.*/
    size_t start = input.find_first_not_of(" \t\n\r\f\v");
    size_t end = input.find_last_not_of(" \t\n\r\f\v");

    if (start == string::npos) {
        // if the input was entirely whitespace
        throw runtime_error("Query is empty.");
    }

    string trimmed = input.substr(start, end - start + 1);
   /*----------------------------------*/ 
    Query query;

    if (trimmed.rfind("GET ", 0) == 0) { //if it starts with GET, filter parser
        query.type = QueryType::Filter;
        query.filter = parseFilterQuery(trimmed); //using trimmed for filterr query to check for whitespace
    }
    else {
        query.type = QueryType::DotPath;
        query.dotPath = parseDotPath(trimmed); //using trimmed for dot path query to check for whitespace
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

/*FIX 3: Checks that "keyword" appears at position i AND is immediately followed by whitespace or the end of the string. Without this, a typo like "FROMX" or "WHEREX" would silently match the keyword and then fail later with a confusing "expected key" or "trailing token" error instead of a clear message about the keyword itself.*/
bool QueryParser::matchKeyword(const std::string& input, std::size_t i, const std::string& keyword) const {
    if (input.compare(i, keyword.size(), keyword) != 0) {
        return false;
    }
 
    size_t after = i + keyword.size();
 
    if (after < input.size() && !isspace(static_cast<unsigned char>(input[after]))) {
        return false;
    }
 
    return true;
}

FilterQuery QueryParser::parseFilterQuery(const std::string& input) const { //GET, FROM, WHERE
    FilterQuery filterQuery;
    size_t i = 0;

    //skipping extra spaces before comparing
    skipWhitespace(input, i);

    //GET comparison
    // FIX 3: use matchKeyword instead of a bare compare, so "GETX" is rejected here too rather than only relying on the "GET " check in parse().
     if (!matchKeyword(input, i, "GET")) {
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
     // FIX 3: word-boundary check so "FROMX" isn't mistaken for the FROM keyword.
    if (!matchKeyword(input, i, "FROM")) {
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
    // FIX 3: word-boundary check so "WHEREX" isn't mistaken for the WHERE keyword 
    
    if (matchKeyword(input, i, "WHERE")) { //where filters not mandatory for query to run, available choice
        i += 5;

        while(true) {
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

            if (i >= input.size()) { //check if there's actually a value after the operator
                throw runtime_error("expected value after operator");
            }

            string readVal;
            
            // FIX 4: support quoted string values so they can contain spaces, 
            //e.g. WHERE name = 'John Doe'. Unquoted values (numbers, bare words) keeps the old behavior of stopping at the next whitespace.
            if (input[i] == '\'') {
                ++i; // skip opening quote
    
                while (i < input.size() && input[i] != '\'') {
                    readVal += input[i];
                    ++i;
                }
    
                if (i >= input.size()) {
                    throw runtime_error("unterminated string value after operator");
                }
    
                ++i; // skip closing quote
            }
            else {
                while (i < input.size() && !isspace(static_cast<unsigned char>(input[i]))) {
                    readVal += input[i];
                    ++i;
                }
            }
            
            whereCond.value = readVal;

            filterQuery.conditions.push_back(whereCond);

            //check for an AND condition:
            
            skipWhitespace(input, i);

            if(matchKeyword(input, i, "AND")) {
                i += 3;
                continue;
            }

            //no and, break loop;
            break;
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
            /*fix: negative indices check fix*/
			else if (isdigit(static_cast<unsigned char>(input[i])) || input[i] == '-') {
                string digits;

                if (input[i] == '-') {
                    digits += input[i];
                    ++i;
                }

                if (i >= input.size() || !isdigit(static_cast<unsigned char>(input[i]))) {
                    throw runtime_error(
                        "error: '-' must be followed by digits in array index"
                    );
                }

                while (i < input.size() && isdigit(input[i])) {
					digits += input[i]; //load nums into digits to convert
					++i;
				}
				int index = stoi(digits);

                if (index < 0) {
                    throw runtime_error(
                        "negative array indices not supported"
                    );
                }

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
            /*------------------------------*/
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