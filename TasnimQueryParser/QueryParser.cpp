#include "QueryParser.h"

#include <cctype>
#include <stdexcept>

using namespace std;

Query QueryParser::parse(const string& input) const {
    if (input.empty()) {
        throw runtime_error("Query is empty.");
    }

    Query query;
    query.type = QueryType::DotPath;
    query.dotPath = parseDotPath(input);

    return query;
}

DotPathQuery QueryParser::parseDotPath(const string& input) const {
    DotPathQuery query;

    size_t i = 0;

    while (i < input.size()) {

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

        query.path.push_back({
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

                query.path.push_back({ //encompassing all elements of arr
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

                query.path.push_back({
                    PathPartType::ArrayIndex, "", index
                });
            }

            else {
                throw runtime_error(
                    "error: missing array index or * in brackets."
                );
            }
        }

        //if input isn't finished, 
        if (i < input.size()) {
            
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

    return query;
}