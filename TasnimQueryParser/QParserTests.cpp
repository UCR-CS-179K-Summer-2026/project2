#include <iostream>
#include <string>
#include <vector>

#include "QueryParser.h"

using namespace std;

struct TestCase {
    string input;
    bool shouldPass;
};

int main() {
    QueryParser parser;

	//testing valid vs invalid queries
    vector<TestCase> tests = {
        //valid query
        {"School.students.name", true},
        {"School.students[*].name", true},
        {"School.students[14].year", true},
        {"a.b.c", true},

        //invalid query tests:
        {"", false},
        {"School..students", false},
        {"School.students.", false},
		{"School.students[-1].name", false},
        {"School.students[", false},
        {"School.students[*", false},
        {"School.students[abc].name", false},

		//correct parsing tests

    };

    int passed = 0; //num of total passing tests

    for (const auto& test : tests) {
        bool success = false; //test status for each

        try {
            parser.parse(test.input);
            success = true;
        }
        catch (const exception&) {
            success = false;
        }

        if (success == test.shouldPass) {
            cout << "PASS: " << test.input << '\n';
            ++passed;
        }
        else {
            cout << "FAIL: " << test.input << '\n';
        }
    }


	//parser is working correctly:
	Query query = parser.parse("School.students[*].name");

	assert(query.dotPath.path.size() == 4);
	assert(query.dotPath.path[0].key == "School");
	assert(query.dotPath.path[1].key == "students");
	assert(query.dotPath.path[2].type == PathPartType::AllElements);
	assert(query.dotPath.path[3].key == "name");


    cout << "\nPassed "
              << passed
              << " out of "
              << tests.size()
              << " tests.\n";

    return passed == static_cast<int>(tests.size());
}