#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"
#include <iostream>
 
void printResults(const std::string& label,
                   const std::vector<const parser::Node*>& results,
                   const std::vector<char>& jsonData) {
    std::cout << label << " => [";
    for (size_t i = 0; i < results.size(); i++) {
        std::cout << nodeToString(results[i], jsonData);
        if (i + 1 < results.size()) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}
 
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
        auto results = executeQuery(p.getRoot(), query);
        printResults(queryStr, results, p.getJsonData());
    } catch (const std::exception& e) {
        std::cout << queryStr << " => PARSE ERROR: " << e.what() << std::endl;
    }
}
 
int main() {
    std::cout << "--- Test 1: Employees dataset (wildcard) ---" << std::endl;
    runTest("test_data_employees.json", "Google.employees[*].name");
 
    std::cout << "\n--- Test 2: School dataset (deep nesting) ---" << std::endl;
    runTest("test_data_school.json", "school.classroom.students[*].studentName");
 
    std::cout << "\n--- Test 3: Inventory dataset (wildcard + missing field) ---" << std::endl;
    runTest("test_data_inventory.json", "items[*].inStock");
    runTest("test_data_inventory.json", "items[*].missingField");
 
    std::cout << "\n--- Test 4: NEW capability - array indexing ---" << std::endl;
    runTest("test_data_inventory.json", "items[0].sku");
    runTest("test_data_inventory.json", "items[1].inStock");
    runTest("test_data_inventory.json", "items[99].sku"); // out of range -> null
 
    std::cout << "\n--- Test 5: malformed query -> should throw, not crash ---" << std::endl;
    runTest("test_data_inventory.json", "items[abc]");

    std::cout << "\n--- Test 6: empty object -> null, not crash ---" << std::endl;
    runTest("test_data_edge.json", "empty_obj.anything");

    std::cout << "\n--- Test 7: empty array with wildcard -> [] ---" << std::endl;
    runTest("test_data_edge.json", "empty_arr[*].sku");

    std::cout << "\n--- Test 8: nonexistent top-level key -> null ---" << std::endl;
    runTest("test_data_edge.json", "foo.bar");

    std::cout << "\n--- Test 9: null partway through path -> null, not throw ---" << std::endl;
    runTest("test_data_edge.json", "a.b.c");

    std::cout << "\n--- Test 10: deep nesting (5+ levels) ---" << std::endl;
    runTest("test_data_edge.json", "nested.x.y.z.w");

    std::cout << "\n--- Test 11: query resolves to non-leaf object ---" << std::endl;
    runTest("test_data_edge.json", "items[0]");

    std::cout << "\n--- Test 12: negative index ---" << std::endl;
    runTest("test_data_edge.json", "items[-1].sku");

    std::cout << "\n--- Test 13: nested wildcards (array of arrays) ---" << std::endl;
    runTest("test_data_edge.json", "items[*].tags[*]");

    std::cout << "\n--- Test 14: empty query string ---" << std::endl;
    runTest("test_data_edge.json", "");

    std::cout << "\n--- Test 15: trailing dot ---" << std::endl;
    runTest("test_data_edge.json", "a.b.");

    std::cout << "\n--- Test 16: double dots ---" << std::endl;
    runTest("test_data_edge.json", "a..b");
 
    return 0;
}
