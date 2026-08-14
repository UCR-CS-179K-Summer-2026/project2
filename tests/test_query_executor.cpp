//first real ported test cases, using the actual parser/query executor API from query_executor.h and main.cpp (confirmed 2026-08-13, Each TEST() below mirrors an existing case from main.cpp's runTest() calls,

// these tests load the same test_data_*.json files main.cpp uses, via relative paths from the repo root. See tests/CMakeLists.txt, gtest_discover_tests() is configured with WORKING_DIRECTORY set to the repo root specifically so these relative paths resolve the same way they do when you run: ./build/test_executor manually from repo root.

#include <gtest/gtest.h>
#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryParser.h"
#include "query_executor.h"

namespace {

// Small helper mirroring main.cpp's runTest(), but returning the results instead of printing them, so tests can assert on them directly.
std::vector<std::string> runQuery(const std::string& file, const std::string& queryStr) {
    parser p;
    if (!p.loadFile(file)) {
        ADD_FAILURE() << "Failed to load file: " << file;
        return {};
    }
    p.indexStructure();
    p.constructTree();

    QueryParser qp;
    Query query = qp.parse(queryStr);  // let this throw for malformed-query tests
    auto results = executeQuery(p.getRoot(), query, p.getJsonData());

    std::vector<std::string> out;
    out.reserve(results.size());
    for (const auto* node : results) {
        out.push_back(nodeToString(node, p.getJsonData()));
    }
    return out;
}

// --- Ported from main.cpp Test 1: wildcard dot-path -----------------------
TEST(DotPathQuery, WildcardOverEmployeesArray) {
    auto results = runQuery("test_data_employees.json", "Google.employees[*].name");
    // Expects: ["John Doe", "Jane Doe", DNE] -- third employee has no "name" field
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], "\"John Doe\"");
    EXPECT_EQ(results[1], "\"Jane Doe\"");
    EXPECT_EQ(results[2], "DNE");
}

// --- Ported from main.cpp Test 4: array indexing ---------------------------
TEST(DotPathQuery, ArrayIndexReturnsCorrectSku) {
    auto results = runQuery("test_data_inventory.json", "items[0].sku");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0], "\"A1\"");
}

// --- Ported from main.cpp Test 5: malformed query should throw, not crash -
TEST(DotPathQuery, MalformedArrayIndexThrowsParseError) {
    parser p;
    ASSERT_TRUE(p.loadFile("test_data_inventory.json"));
    p.indexStructure();
    p.constructTree();

    QueryParser qp;
    EXPECT_THROW(qp.parse("items[abc]"), std::exception);
}

// --- Ported from main.cpp Test 18: filter query, numeric > -----------------
TEST(FilterQuery, NumericGreaterThanReturnsMatchingNames) {
    auto results = runQuery("test_data_filter.json", "GET name FROM store.products WHERE price > 300");
    // Expects: all 6 products, given current test data prices
    ASSERT_EQ(results.size(), 6u);
    EXPECT_EQ(results[0], "\"Laptop\"");
}

}  // namespace