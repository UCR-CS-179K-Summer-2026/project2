#include "AaronJsonParser/parser.h"
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
 
void runTest(const std::string& label, const std::string& file, const QueryAST& ast) {
    parser p;
    if (!p.loadFile(file)) {
        std::cout << label << " => FAILED TO LOAD " << file << std::endl;
        return;
    }
    p.indexStructure();
    p.constructTree();
 
    auto results = executeQuery(p.getRoot(), ast);
    printResults(label, results, p.getJsonData());
}
 
int main() {
    std::cout << "--- Test 1: Employees dataset ---" << std::endl;
    runTest("Google.employees[*].name", "test_data_employees.json", {
        {StepKind::Field, "Google"},
        {StepKind::Field, "employees"},
        {StepKind::Wildcard, ""},
        {StepKind::Field, "name"}
    });
 
    std::cout << "\n--- Test 2: School dataset (different shape) ---" << std::endl;
    runTest("school.classroom.students[*].studentName", "test_data_school.json", {
        {StepKind::Field, "school"},
        {StepKind::Field, "classroom"},
        {StepKind::Field, "students"},
        {StepKind::Wildcard, ""},
        {StepKind::Field, "studentName"}
    });
 
    std::cout << "\n--- Test 3: Inventory dataset (top-level array, booleans) ---" << std::endl;
    runTest("items[*].inStock", "test_data_inventory.json", {
        {StepKind::Field, "items"},
        {StepKind::Wildcard, ""},
        {StepKind::Field, "inStock"}
    });
 
    runTest("items[*].missingField", "test_data_inventory.json", {
        {StepKind::Field, "items"},
        {StepKind::Wildcard, ""},
        {StepKind::Field, "missingField"}
    });
 
    return 0;
}