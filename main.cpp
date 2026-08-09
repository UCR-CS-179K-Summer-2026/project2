#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"
#include <iostream>
 
// Prints one test's results as a bracketed, comma-separated list
void printResults(const std::string& label, const std::vector<const parser::Node*>& results, const std::vector<char>& jsonData) {
    std::cout << label << " => [";
    for (size_t i = 0; i < results.size(); i++) {
        std::cout << nodeToString(results[i], jsonData);
        if (i + 1 < results.size()) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}
 
// Loads a JSON file, parses the given query string, executes it, and prints the result. malformed syntax are caught here and reported as "PARSE ERROR" instead of crashing.
// [CHANGED] now calls the 3-arg executeQuery(root, query, jsonData) overload instead of the 2-arg one. This is required to support filter (GET/FROM/WHERE) queries, since WHERE comparisons need the raw jsonData buffer to read actual values. The 3-arg overload still handles plain dot-path queries exactly as before, so all 17 existing tests below are unaffected by this change.
void runTest(const std::string& file, const std::string& queryStr) {
    parser p;
    if (!p.loadFile(file)) {
        std::cout << queryStr << " => FAILED TO LOAD " << file << std::endl;
        return;
    }
    p.indexStructure();
    p.constructTree();
 
    QueryParser qp;
    try {
        Query query = qp.parse(queryStr);
        auto results = executeQuery(p.getRoot(), query, p.getJsonData()); // [CHANGED] 3-arg overload
        printResults(queryStr, results, p.getJsonData());
    } catch (const std::exception& e) {
        std::cout << queryStr << " => PARSE ERROR: " << e.what() << std::endl;
    }
}
 
int main() {
    //Happy path coverage: wildcard, nesting, missing fields, array indexing
    std::cout << "--- Test 1: Employees dataset (wildcard) ---" << std::endl;
    runTest("test_data_employees.json", "Google.employees[*].name");
 
    std::cout << "\n--- Test 2: School dataset (deep nesting) ---" << std::endl;
    runTest("test_data_school.json", "school.classroom.students[*].studentName");
 
    std::cout << "\n--- Test 3: Inventory dataset (wildcard + missing field) ---" << std::endl;
    runTest("test_data_inventory.json", "items[*].inStock");
    runTest("test_data_inventory.json", "items[*].missingField"); // field absent on every element -> nulls, not a crash
 
    std::cout << "\n--- Test 4: array indexing ---" << std::endl;
    runTest("test_data_inventory.json", "items[0].sku");
    runTest("test_data_inventory.json", "items[1].inStock");
    runTest("test_data_inventory.json", "items[99].sku"); // out of range -> null
 
    std::cout << "\n--- Test 5: malformed query -> should throw, not crash ---" << std::endl;
    runTest("test_data_inventory.json", "items[abc]");

    //Structural edge cases: empty containers, missing/null paths, deep nesting
    std::cout << "\n--- Test 6: empty object -> null, not crash ---" << std::endl;
    runTest("test_data_edge.json", "empty_obj");

    std::cout << "\n--- Test 7: empty array with wildcard -> [] ---" << std::endl;
    runTest("test_data_edge.json", "empty_arr[*].sku"); // wildcard over zero elements shows empty result, not null

    std::cout << "\n--- Test 8: nonexistent top-level key -> null ---" << std::endl;
    runTest("test_data_edge.json", "foo.bar");

    std::cout << "\n--- Test 9: null partway through path -> null, not throw ---" << std::endl;
    runTest("test_data_edge.json", "a.b.c");

    std::cout << "\n--- Test 10: deep nesting (5 levels) ---" << std::endl;
    runTest("test_data_edge.json", "nested.x");

    std::cout << "\n--- Test 11: query resolves to non-leaf object ---" << std::endl;
    runTest("test_data_edge.json", "items[0]");

    std::cout << "\n--- Test 12: negative index ---" << std::endl;
    runTest("test_data_edge.json", "items[-1].sku"); // rejected by design; expects "negative array indices not supported"

    std::cout << "\n--- Test 13: nested wildcards (array of arrays) ---" << std::endl;
    runTest("test_data_edge.json", "items[*].tags[*]"); // wildcard-of-wildcard should flatten into one result list

    std::cout << "\n--- Test 14: empty query string ---" << std::endl;
    runTest("test_data_edge.json", "");

    std::cout << "\n--- Test 15: trailing dot ---" << std::endl;
    runTest("test_data_edge.json", "a.b.");

    std::cout << "\n--- Test 16: double dots ---" << std::endl;
    runTest("test_data_edge.json", "a..b");

    std::cout << "\n--- Test 17: just star and no sub query ---" << std::endl;
    runTest("test_data_employees.json", "Google.employees[*]");

    std::cout << "\n--- Test 17: just star and no sub query ---" << std::endl;
    runTest("test_data_sample.json", "a");

    // Tier 5: Filter Query Tests (GET / FROM / WHERE) Uses test_data_filter.json (a new dataset, a store with a nested products array) plus test_data_inventory.json where existing fields (items, sku, inStock) already support a filter test without needing new data.

    std::cout << "\n--- Test 18: filter query, numeric > ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price > 300");

   std::cout << "\n--- Test 19: filter query, boolean equality ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE inStock = true");

    std::cout << "\n--- Test 20: filter query, string equality, GET differs from WHERE field ---" << std::endl;
    runTest("test_data_filter.json", "GET price FROM store.products WHERE category = electronics");

    std::cout << "\n--- Test 21: filter query, exact numeric match ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price = 300");

    std::cout << "\n--- Test 22: filter query, no rows match ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price > 9999");
    // Expects: [] (empty result, not a crash or error)

    std::cout << "\n--- Test 23: filter query, quoted string value with a space ---" << std::endl;
    runTest("test_data_filter.json", "GET price FROM store.products WHERE name = 'Wireless Mouse'");

    std::cout << "\n--- Test 24: filter query, FROM path does not exist ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.nonexistentList WHERE price > 0");
    // Expects: [] -- FROM resolves to null/non-array, so the scan finds nothing to iterate rather than throwing

    std::cout << "\n--- Test 25: filter query, <= operator ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price <= 300");

    std::cout << "\n--- Test 26: filter query against existing Sprint-1 inventory data ---" << std::endl;
    runTest("test_data_inventory.json", "GET sku FROM items WHERE inStock = true");

    std::cout << "\n--- Test 27: Bug 1 regression - leading whitespace on filter query ---" << std::endl;
    runTest("test_data_filter.json", "   GET name FROM store.products WHERE price > 300");
    // Before the fix: leading whitespace caused a misroute into dot-path parsing and threw "Unexpected trailing token...".

    std::cout << "\n--- Test 28: Bug 1 regression - trailing whitespace on dot-path query ---" << std::endl;
    runTest("test_data_employees.json", "Google.employees[*].name   ");
    // Expects same result as Test 1, despite trailing spaces.

    std::cout << "\n--- Test 29: Bug 2 regression - negative array index ---" << std::endl;
    runTest("test_data_filter.json", "store.products[-1].name");
    // Expects: PARSE ERROR: negative array indices not supported

    std::cout << "\n--- Test 30: Bug 3 regression - FROM keyword typo ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROMX store.products");
    // Expects: PARSE ERROR: Expected FROM
    // Before the fix, "FROMX" would have been misread as a match for FROM.

    std::cout << "\n--- Test 31: Bug 3 regression - WHERE keyword typo ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHEREX price > 0");
    // Expects: PARSE ERROR: unread/extra input located at index: ...
    // WHERE is optional, so "WHEREX" must NOT match it. it should instead be rejected as leftover trailing input after a successful FROM parse.

    std::cout << "\n--- Test 32: Bug 4 regression - unquoted WHERE value containing a space ---" << std::endl;
    runTest("test_data_filter.json", "GET price FROM store.products WHERE name = Wireless Mouse");
    // Expects: PARSE ERROR (unread/extra input) -- without quotes, only "Wireless" is read as the value and "Mouse" is leftover. 

    std::cout << "\n--- Test 33: GET + FROM, no WHERE ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products");

    std::cout << "\n--- Test 34: GET + FROM, no WHERE, different field ---" << std::endl;
    runTest("test_data_filter.json", "GET price FROM store.products");

    std::cout << "\n--- Test 35: GET + FROM, no WHERE, trailing whitespace ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products   ");
    // Expects: confirms trailing whitespace after FROM doesn't falsely trigger the WHERE block

    std::cout << "\n--- Test 36: GET + FROM, bad FROM path, no WHERE ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.nonexistentList");
    // Expects: [] (no rows to project, no error)

    std::cout << "\n--- Test 37: GET + FROM + WHERE, boolean equality (regression check) ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE inStock = true");

    std::cout << "\n--- Test 38: GET + FROM + WHERE, numeric > (regression check) ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price > 300");

    std::cout << "\n--- Test 39: GET + FROM + WHERE, quoted string value (regression check) ---" << std::endl;
    runTest("test_data_filter.json", "GET price FROM store.products WHERE name = 'Wireless Mouse'");

    std::cout << "\n--- Test 40: WHEREX typo, confirms it's rejected not silently entered ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHEREX price > 0");
    // Expects: PARSE ERROR: unread/extra input located at index
    return 0;
} 