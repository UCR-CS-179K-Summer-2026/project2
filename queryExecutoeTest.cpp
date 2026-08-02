#include "query_executor.h"
#include <iostream>
 
// ---- small helpers to build JsonValue nodes by hand ----
JsonValuePtr S(const std::string& s) {
    auto v = std::make_shared<JsonValue>(); v->type = JsonType::String; v->strVal = s; return v;
}
JsonValuePtr N(double n) {
    auto v = std::make_shared<JsonValue>(); v->type = JsonType::Number; v->numVal = n; return v;
}
JsonValuePtr B(bool b) {
    auto v = std::make_shared<JsonValue>(); v->type = JsonType::Bool; v->boolVal = b; return v;
}
JsonValuePtr O(std::map<std::string, JsonValuePtr> fields) {
    auto v = std::make_shared<JsonValue>(); v->type = JsonType::Object; v->objVal = std::move(fields); return v;
}
JsonValuePtr A(std::vector<JsonValuePtr> elems) {
    auto v = std::make_shared<JsonValue>(); v->type = JsonType::Array; v->arrVal = std::move(elems); return v;
}
 
// ---- helper to build a QueryAST by hand (standing in for noow for Tasmin's parser) ----
QueryAST field(const std::string& name) { return {{StepKind::Field, name}}; }
QueryAST operator+(QueryAST a, const QueryAST& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}
QueryAST wildcard() { return {{StepKind::Wildcard, ""}}; }
 
void printResults(const std::string& label, const std::vector<JsonValuePtr>& results) {
    std::cout << label << " => [";
    for (size_t i = 0; i < results.size(); i++) {
        std::cout << (results[i] ? results[i]->toString() : "null");
        if (i + 1 < results.size()) std::cout << ", ";
    }
    std::cout << "]\n";
}
 
// TEST 1: original employees dataset (sanity check against known-good output)
void test1_employees() {
    std::cout << "\n--- Test 1: Employees dataset ---\n";
    auto emp1 = O({{"employeeId", N(1000)}, {"name", S("John Doe")}, {"salary", N(100000)}});
    auto emp2 = O({{"employeeId", N(1001)}, {"name", S("Jane Doe")}, {"salary", N(90000)}});
    auto emp3 = O({{"employeeId", N(3)}}); // missing name/salary
    auto root = O({{"Google", O({{"employees", A({emp1, emp2, emp3})}})}});
 
    // Query: Google.employees[*].name
    QueryAST ast = field("Google") + field("employees") + wildcard() + field("name");
    printResults("Google.employees[*].name", executeQuery(root, ast));
}
 

// TEST 2: a totally different shape: a school with nested classrooms and students. Deeper nesting, different field names, and hecne shows no hardcoding.
void test2_school() {
    std::cout << "\n--- Test 2: School dataset (different shape) ---\n";
    auto student1 = O({{"studentName", S("Alice")}, {"grade", N(91)}});
    auto student2 = O({{"studentName", S("Bob")}, {"grade", N(85)}});
    auto classroomA = O({
        {"teacher", S("Mr. Lee")},
        {"students", A({student1, student2})}
    });
    auto root = O({{"school", O({{"classroom", classroomA}})}});
 
    // Query: school.classroom.students[*].studentName
    QueryAST ast = field("school") + field("classroom") + field("students") + wildcard() + field("studentName");
    printResults("school.classroom.students[*].studentName", executeQuery(root, ast));
}
 

// TEST 3: an inventory dataset with a top-level array (not nested inside an object first) and boolean fields, to test more edge cases.
void test3_inventory() {
    std::cout << "\n--- Test 3: Inventory dataset (top-level array, booleans) ---\n";
    auto item1 = O({{"sku", S("A100")}, {"inStock", B(true)}});
    auto item2 = O({{"sku", S("A200")}, {"inStock", B(false)}});
    auto root = O({{"items", A({item1, item2})}});
 
    // Query: items[*].inStock
    QueryAST ast = field("items") + wildcard() + field("inStock");
    printResults("items[*].inStock", executeQuery(root, ast));
 
    // Query: items[*].missingField (tests missing-key -> null behavior)
    QueryAST ast2 = field("items") + wildcard() + field("missingField");
    printResults("items[*].missingField", executeQuery(root, ast2));
}
 
int main() {
    test1_employees();
    test2_school();
    test3_inventory();
    return 0;
}