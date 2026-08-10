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

        // valid filter queries using WHERE and AND
        {"GET name FROM School.students WHERE year > 2", true},

        {"GET name FROM School.students WHERE year > 1 AND name = 'Alice'", true},

        {"GET name FROM School.students WHERE year > 2 AND grade >= 90 AND active = true", true},

        //invalid query tests:
        {"", false},
        {"School..students", false},
        {"School.students.", false},
		{"School.students[-1].name", false},
        {"School.students[", false},
        {"School.students[*", false},
        {"School.students[abc].name", false},

        // invalid AND queries
        {"GET name FROM School.students WHERE grade > 75 AND", false},

        {"GET name FROM School.students WHERE grade > 80 AND AND name = 'Alice'", false},

        {"GET name FROM School.students WHERE grade > 80 ANDX name = 'Alice'", false},

        {"GET name FROM School.students WHERE grade > 80 AND name =", false}

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

    //AND structure test

    Query andQuery = parser.parse(
        "GET name FROM School.students "
        "WHERE year > 2 AND name = 'Bob'"
    );

    assert(andQuery.type == QueryType::Filter);

    assert(andQuery.filter.conditions.size() == 2);

    // First condition
    assert(andQuery.filter.conditions[0].field[0].key == "year");

    assert(
        andQuery.filter.conditions[0].comparisonOp ==
        FilterOperators::GreaterThan
    );

    assert(andQuery.filter.conditions[0].value == "2");

    // Second condition
    assert(andQuery.filter.conditions[1].field[0].key == "name");

    assert(
        andQuery.filter.conditions[1].comparisonOp ==
        FilterOperators::Equal
    );

    assert(andQuery.filter.conditions[1].value == "Bob");

    cout << "\nPassed "
              << passed
              << " out of "
              << tests.size()
              << " tests.\n";

    return passed == static_cast<int>(tests.size() ? 0 : 1);

}