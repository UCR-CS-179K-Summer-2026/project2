#include <iostream>
#include <string>

#include "QueryParser.h"

using namespace std;

void printQuery(const Query& query) {
    for (const PathPart& part : query.dotPath.path) {
        if (part.type == PathPartType::Key) {
            cout << "Key: " << part.key << '\n';
        }
        
		else if (part.type == PathPartType::ArrayIndex) {
            cout << "Array index: " << part.index << '\n';
        }
        
		else if (part.type == PathPartType::AllElements) {
            cout << "All elements\n";
        }
    }
}

int main() {
    QueryParser parser;

    string input;
    cout << "Enter query: ";
    getline(cin, input);

    try {
        Query query = parser.parse(input);

        cout << "Query parsed successfully \n";
        printQuery(query);
    }
	
    catch (const exception& error) {
        cerr << "Query error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
