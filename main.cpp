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
    runTest("test_data_inventory.json", "items[*].missingField"); // [CHANGED] field absent on every element -> DNEs now, not "nulls" -- see the DNE-vs-real-null change (Test 45+ below)
 
    std::cout << "\n--- Test 4: array indexing ---" << std::endl;
    runTest("test_data_inventory.json", "items[0].sku");
    runTest("test_data_inventory.json", "items[1].inStock");
    runTest("test_data_inventory.json", "items[99].sku"); // [CHANGED] out of range -> DNE now, not "null"
 
    std::cout << "\n--- Test 5: malformed query -> should throw, not crash ---" << std::endl;
    runTest("test_data_inventory.json", "items[abc]");

    //Structural edge cases: empty containers, missing/null paths, deep nesting
    std::cout << "\n--- Test 6: empty object -> null, not crash ---" << std::endl;
    // [FLAGGED, not changed] if empty_obj actually exists in test_data_edge.json as {}, this should print "{}" via nodeToString's object case -- not "null" and not "DNE" -- since the query resolves to a real (empty) object node, not a missing/failed lookup. Worth checking this label against the actual data: if it's testing "querying an empty object works", the expected output is "{}", and the old "-> null" wording may have already been informal/stale before this change. Left the test itself untouched since I don't have test_data_edge.json's actual contents to confirm either way.
    runTest("test_data_edge.json", "empty_obj");

    std::cout << "\n--- Test 7: empty array with wildcard -> [] ---" << std::endl;
    runTest("test_data_edge.json", "empty_arr[*].sku"); // wildcard over zero elements shows empty result, not null

    std::cout << "\n--- Test 8: nonexistent top-level key -> DNE ---" << std::endl; // [CHANGED] was "-> null"
    runTest("test_data_edge.json", "foo.bar");

    std::cout << "\n--- Test 9: path continues past a null value -> DNE, not throw ---" << std::endl; // [CHANGED] was "-> null"
    // [CHANGED] if a.b is a real JSON null in the data, querying "a.b" alone would correctly print "null" (the real value). But this query goes one step further, past the null, to ".c" -- executeStep's Key case requires the current node to be an object to descend into it, and a null-type node isn't one, so this fails to resolve and now prints DNE, not null. That's the actual point of this test: DNE means "the query couldn't resolve", null means "the query resolved to a real null value" -- this case demonstrates the former even though a null is technically present partway through the path.
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

    // [NEW] Tier 7: AND condition tests Tasnim's QueryParser now supports chaining multiple WHERE conditions with AND.

    std::cout << "\n--- Test 41: AND with two conditions ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE category = electronics AND inStock = true");

    std::cout << "\n--- Test 42: AND with three conditions ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE category = electronics AND inStock = true AND price < 100");

    std::cout << "\n--- Test 43: AND typo (ANDX) should not be treated as AND ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price > 300 ANDX inStock = true");
    // Expects: PARSE ERROR: unread/extra input located at index: 47

    std::cout << "\n--- Test 44: trailing AND with nothing after it ---" << std::endl;
    runTest("test_data_filter.json", "GET name FROM store.products WHERE price > 300 AND");
    // Expects: PARSE ERROR: expected input after WHERE filter


    // [NEW] Tier 8: DNE vs. real null Uses test_data_dne.json built specifically to exercise all four cases side by side: a real JSON null, a missing key, an out-of-range index, and a type mismatch. The first should print "null"; the other three should all print "DNE", since none of them are a real null value
    std::cout << "\n--- Test 45: real JSON null value -> null ---" << std::endl;
    runTest("test_data_dne.json", "user.middleName");
    // Expects: [null] -- this is a genuine null in the data, so it should NOT print DNE.

    std::cout << "\n--- Test 46: key does not exist -> DNE ---" << std::endl;
    runTest("test_data_dne.json", "user.nickname");
    // Expects: [DNE]

    std::cout << "\n--- Test 47: array index out of range -> DNE ---" << std::endl;
    runTest("test_data_dne.json", "items[5].sku");
    // Expects: [DNE]

    std::cout << "\n--- Test 48: type mismatch, indexing into a non-array -> DNE ---" << std::endl;
    runTest("test_data_dne.json", "user.name[0]");
    // Expects: [DNE]

    std::cout << "\n--- Test 49: normal existing value, sanity check ---" << std::endl;
    runTest("test_data_dne.json", "user.name");
    // Expects: ["Alice"] : confirms ordinary lookups are unaffected by the DNE change 

    /*std::cout << "\n--- Test 50: array indexing at start of large file (500KB) ---" << std::endl;
    runTest("test_data_large.json", "store.products[0].name");
    // Expects: ["Classic Desk 0"]

    std::cout << "\n--- Test 51: array indexing at middle of large file ---" << std::endl;
    runTest("test_data_large.json", "store.products[2827].name");
    // Expects: ["Standard Fan 2827"] (index 2827 of 5654 total products)

    std::cout << "\n--- Test 52: array indexing at last valid index ---" << std::endl;
    runTest("test_data_large.json", "store.products[5653].name");
    // Expects: ["Smart Chair 5653"] (5654 products total, so 5653 is the last valid index)

    std::cout << "\n--- Test 53: array indexing one past the end -> DNE ---" << std::endl;
    runTest("test_data_large.json", "store.products[5654].name");
    // Expects: [DNE] -- off-by-one boundary check on a real large array

    std::cout << "\n--- Test 54: array indexing way out of range -> DNE ---" << std::endl;
    runTest("test_data_large.json", "store.products[99999].name");
    // Expects: [DNE]

    std::cout << "\n--- Test 55: negative index on large file ---" << std::endl;
    runTest("test_data_large.json", "store.products[-1].name");
    // Expects: PARSE ERROR: negative array indices not supported

    std::cout << "\n--- Test 56: wildcard across all 5654 products ---" << std::endl;
    runTest("test_data_large.json", "store.products[*].name");
    // Expects: 5654 results -- good for eyeballing wildcard fan-out cost at scale

    std::cout << "\n--- Test 57: filter query with AND on large file ---" << std::endl;
    runTest("test_data_large.json", "GET name FROM store.products WHERE category = electronics AND inStock = true");
    // Expects: 465 results

    std::cout << "\n--- Test 58: filter query, narrow numeric range (few matches) ---" << std::endl;
    runTest("test_data_large.json", "GET name FROM store.products WHERE price < 10");
    // Expects: 15 results -- good stress case: full row scan, small result set

    std::cout << "\n--- Test 59: filter query, upper price range ---" << std::endl;
    runTest("test_data_large.json", "GET name FROM store.products WHERE price > 1990");
    // Expects: 16 results

    std::cout << "\n--- Test 60: filter query, category + price AND ---" << std::endl;
    runTest("test_data_large.json", "GET name FROM store.products WHERE category = furniture AND price < 50");
    // Expects: 32 results

    std::cout << "\n--- Test 61: filter query, no matches on large file ---" << std::endl;
    runTest("test_data_large.json", "GET name FROM store.products WHERE price > 999999");
    // Expects: [] (empty, full scan finds nothing -- worst case for timing since no early exits help)

    std::cout << "\n--- Test 62: GET field that doesn't exist on any row ---" << std::endl;
    runTest("test_data_large.json", "GET count FROM store.products WHERE category = electronics");
    // Expects: 948 results, every one of them DNE (products don't have a "count" field), it confirms DNE handling holds up at scale, not just on small hand-built data */

    return 0;
}