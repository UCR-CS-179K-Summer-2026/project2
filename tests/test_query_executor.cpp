//first real ported test cases, using the actual parser/query executor API from query_executor.h and main.cpp these tests load the same test_data_*.json files main.cpp uses, via relative paths from the repo root. See tests/CMakeLists.txt, gtest_discover_tests() is configured with WORKING_DIRECTORY set to the repo root specifically so these relative paths resolve the same way they do when you run: ./build/test_executor manually from repo root.

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

// Ported from main.cpp Test 1: wildcard dot-path
// TEST(DotPathQuery, WildcardOverEmployeesArray) {
//     auto results = runQuery("test_data_employees.json", "Google.employees[*].name");
//     // Expects: ["John Doe", "Jane Doe", DNE] : third employee has no "name" field
//     ASSERT_EQ(results.size(), 3u);
//     EXPECT_EQ(results[0], "\"John Doe\"");
//     EXPECT_EQ(results[1], "\"Jane Doe\"");
//     EXPECT_EQ(results[2], "DNE");
// }

// // Ported from main.cpp Test 4: array indexing
// TEST(DotPathQuery, ArrayIndexReturnsCorrectSku) {
//     auto results = runQuery("test_data_inventory.json", "items[0].sku");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"A1\"");
// }

// // Ported from main.cpp Test 5: malformed query should throw, not crash
// TEST(DotPathQuery, MalformedArrayIndexThrowsParseError) {
//     parser p;
//     ASSERT_TRUE(p.loadFile("test_data_inventory.json"));
//     p.indexStructure();
//     p.constructTree();

//     QueryParser qp;
//     EXPECT_THROW(qp.parse("items[abc]"), std::exception);
// }

// // Ported from main.cpp Test 18: filter query, numeric >
// TEST(FilterQuery, NumericGreaterThanReturnsMatchingNames) {
//     auto results = runQuery("test_data_filter.json", "GET name FROM store.products WHERE price > 300");
//     // Expects: all 6 products, given current test data prices
//     ASSERT_EQ(results.size(), 6u);
//     EXPECT_EQ(results[0], "\"Laptop\"");
// }
// //Remaining core traversal branches (executeStep / resolveSingle)

// //  Ported from main.cpp Test 49: plain key lookup, sanity check 
// TEST(DotPathQuery, KeyFoundReturnsValue) {
//     auto results = runQuery("test_data_dne.json", "user.name");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"Alice\"");
// }
 
// //  Ported from main.cpp Test 46: key does not exist -
// TEST(DotPathQuery, KeyNotFoundReturnsDNE) {
//     auto results = runQuery("test_data_dne.json", "user.nickname");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }
 
// //  Ported from main.cpp Test 2: wildcard over a nested array 
// TEST(DotPathQuery, WildcardOverNestedSchoolArray) {
//     auto results = runQuery("test_data_school.json", "school.classroom.students[*].studentName");
//     ASSERT_EQ(results.size(), 2u);
//     EXPECT_EQ(results[0], "\"Alice\"");
//     EXPECT_EQ(results[1], "\"Bob\"");
// }
 
// //  New: wildcard applied to a non-array node -> DNE
// // "user.name" in test_data_dne.json is a plain string, so applying [*] to it exercises the AllElements-on-non-array branch in executeStep() using real project data (no synthetic file needed).
// TEST(DotPathQuery, WildcardOnNonArrayReturnsDNE) {
//     auto results = runQuery("test_data_dne.json", "user.name[*]");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }
 
// //  Ported from main.cpp Test 4: array index in range, boolean value --
// TEST(DotPathQuery, ArrayIndexInRangeBoolean) {
//     auto results = runQuery("test_data_inventory.json", "items[1].inStock");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "false");
// }
 
// //  Ported from main.cpp Test 47: array index out of range 
// TEST(DotPathQuery, ArrayIndexOutOfRangeReturnsDNE) {
//     auto results = runQuery("test_data_dne.json", "items[5].sku");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }

// /*-- Suite 2 : nodeToString type variety + wildcard-of-wildcard flattening--*/
// //  Ported from main.cpp Test 13: nested wildcard-of-wildcard flattening -
// TEST(DotPathQuery, NestedWildcardOfWildcardFlattening) {
//     auto results = runQuery("test_data_edge.json", "items[*].tags[*]");
//     // items[0].tags = ["red","small"], items[1].tags = ["😃"] -- flattened
//     // in array order, not grouped per-item.
//     ASSERT_EQ(results.size(), 3u);
//     EXPECT_EQ(results[0], "\"red\"");
//     EXPECT_EQ(results[1], "\"small\"");
//     EXPECT_EQ(results[2], "\"\xF0\x9F\x98\x83\"");  // "😃" as UTF-8 bytes
// }
 
// //  New: number leaf prints its raw value, unquoted 
// TEST(NodeToString, NumberLeafPrintsRawValue) {
//     auto results = runQuery("test_data_dne.json", "user.age");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "30");
// }
 
// //  Ported from main.cpp Test 45: real JSON null -> "null", not DNE 
// TEST(NodeToString, RealNullPrintsAsNull) {
//     auto results = runQuery("test_data_dne.json", "user.middleName");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "null");
// }
 
// //  Ported from main.cpp Test 6: empty object prints "{}" -
// TEST(NodeToString, EmptyObjectPrintsBraces) {
//     auto results = runQuery("test_data_edge.json", "empty_obj");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "{}");
// }
 
// //  New: array-of-strings prints as a bracketed, quoted list, Arrays are inherently ordered (unlike object key iteration, whose order isn't confirmed -- see note above), so an exact string match is safe here.
// TEST(NodeToString, ArrayOfStringsPrintsBracketed) {
//     auto results = runQuery("test_data_edge.json", "items[0].tags");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "[\"red\",\"small\"]");
// }
 
// //  Ported from main.cpp Test 7: wildcard over an empty array --
// TEST(DotPathQuery, EmptyArrayWildcardReturnsEmptyResult) {
//     auto results = runQuery("test_data_edge.json", "empty_arr[*].sku");
//     // Zero elements to iterate -- zero results, not null/DNE/crash.
//     EXPECT_EQ(results.size(), 0u);
// }
// /*-- Suite 3: structural edge cases + negative index + FROM-path-missing --*/

// // Ported from main.cpp Test 8: nonexistent top-level key
// TEST(DotPathQuery, NonexistentTopLevelKeyReturnsDNE) {
//     auto results = runQuery("test_data_edge.json", "foo.bar");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }
 
// //Ported from main.cpp Test 9: path continues past a real null
// TEST(DotPathQuery, PathContinuesPastNullReturnsDNE) {
//     // a.b is a real JSON null; querying one step further (a.b.c) can't sdescend into a null node, so it resolves to DNE, not a crash.
//     auto results = runQuery("test_data_edge.json", "a.b.c");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }
 
// // Ported from main.cpp Test 10: deep nesting. Every level here has exactly one key, so the printed object is fully deterministic regardless of how objectChildNode iterates.
// TEST(DotPathQuery, DeepNestingReturnsNestedObject) {
//     auto results = runQuery("test_data_edge.json", "nested.x");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "{\"y\":{\"z\":{\"w\":\"deep_value\"}}}");
// }
 
// // Ported from main.cpp Test 11: query resolves to a non-leaf object
// // items[0] has two keys (sku, tags); object key iteration order isn't confirmed (see note near the top of this file), so this checks both fields are present through substring rather than asserting one exact string.
// TEST(DotPathQuery, NonLeafObjectQueryReturnsFullObject) {
//     auto results = runQuery("test_data_edge.json", "items[0]");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_NE(results[0].find("\"sku\":\"A1\""), std::string::npos);
//     EXPECT_NE(results[0].find("\"tags\":[\"red\",\"small\"]"), std::string::npos);
// }
 
// // Ported from main.cpp Test 12: negative index rejected at parse time
// TEST(DotPathQuery, NegativeIndexThrowsParseError) {
//     parser p;
//     ASSERT_TRUE(p.loadFile("test_data_edge.json"));
//     p.indexStructure();
//     p.constructTree();
 
//     QueryParser qp;
//     EXPECT_THROW(qp.parse("items[-1].sku"), std::exception);
// }
 
// // Ported from main.cpp Test 24: filter FROM path does not exist 
// TEST(FilterQuery, FromResolvesToNonexistentPathSkipsGracefully) {
//     auto results = runQuery("test_data_filter.json", "GET name FROM store.nonexistentList WHERE price > 0");
//     // FROM resolves to null/non-array -- the scan finds nothing to iterate rather than throwing.
//     EXPECT_EQ(results.size(), 0u);
// }

// /*-- Suite 4: parse-error categories (empty query, malformed dots, keyword typo boundary matching, dangling AND, unquoted value with a space) --*/

// // Small helper for query-parse-only tests: loads a file (so the parser object is valid) and asserts qp.parse(queryStr) throws.
// void expectParseError(const std::string& file, const std::string& queryStr) {
//     parser p;
//     ASSERT_TRUE(p.loadFile(file));
//     p.indexStructure();
//     p.constructTree();
 
//     QueryParser qp;
//     EXPECT_THROW(qp.parse(queryStr), std::exception);
// }
 
// // Ported from main.cpp Test 14: empty query string
// TEST(ParseErrors, EmptyQueryThrows) {
//     expectParseError("test_data_edge.json", "");
// }
 
// // Ported from main.cpp Test 15: trailing dot 
// TEST(ParseErrors, TrailingDotThrows) {
//     expectParseError("test_data_edge.json", "a.b.");
// }
 
// //Ported from main.cpp Test 16: double dots
// TEST(ParseErrors, DoubleDotsThrows) {
//     expectParseError("test_data_edge.json", "a..b");
// }
 
// //Ported from main.cpp Tests 31 & 43: WHEREX / ANDX typos not matched. Two related keyword-boundary-matching regressions in one test, since they exercise the same category of bug (substring keyword matching) on the same underlying fix.
// TEST(ParseErrors, TypoKeywordsNotMatchedAsWhereOrAnd) {
//     expectParseError("test_data_filter.json", "GET name FROM store.products WHEREX price > 0");
//     expectParseError("test_data_filter.json", "GET name FROM store.products WHERE price > 300 ANDX inStock = true");
// }
 
// //  Ported from main.cpp Test 44: trailing AND with nothing after it 
// TEST(ParseErrors, TrailingAndWithNoConditionThrows) {
//     expectParseError("test_data_filter.json", "GET name FROM store.products WHERE price > 300 AND");
// }
 
// //  Ported from main.cpp Test 32: unquoted WHERE value with a space 
// TEST(ParseErrors, UnquotedWhereValueWithSpaceThrows) {
//     // Without quotes, only "Wireless" is read as the value and "Mouse" is leftover, unread input -- this must be rejected, not silently truncated.
//     expectParseError("test_data_filter.json", "GET price FROM store.products WHERE name = Wireless Mouse");
// }

// /*-- Suite 5: filter query variety (quoted values, boolean/string equality, AND with multiple conditions --*/

// // --- Ported from main.cpp Test 23: quoted string value with a space -------
// TEST(FilterQuery, QuotedStringValueWithSpaceParses) {
//     auto results = runQuery("test_data_filter.json", "GET price FROM store.products WHERE name = 'Wireless Mouse'");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "4500");
// }
 
// // --- Ported from main.cpp Test 19: boolean equality ------------------------
// TEST(FilterQuery, BooleanEqualityReturnsMatchingNames) {
//     auto results = runQuery("test_data_filter.json", "GET name FROM store.products WHERE inStock = true");
//     // Laptop, Mouse, Wireless Mouse, Chair are inStock=true; Desk and
//     // Monitor are inStock=false.
//     ASSERT_EQ(results.size(), 4u);
//     EXPECT_EQ(results[0], "\"Laptop\"");
//     EXPECT_EQ(results[1], "\"Mouse\"");
//     EXPECT_EQ(results[2], "\"Wireless Mouse\"");
//     EXPECT_EQ(results[3], "\"Chair\"");
// }
 
// // --- Ported from main.cpp Test 20: string equality, GET differs from WHERE
// TEST(FilterQuery, StringEqualityGetFieldDiffersFromWhereField) {
//     auto results = runQuery("test_data_filter.json", "GET price FROM store.products WHERE category = electronics");
//     // Laptop=1200, Mouse=1000, Wireless Mouse=4500, Monitor=400 are electronics.
//     ASSERT_EQ(results.size(), 4u);
//     EXPECT_EQ(results[0], "1200");
//     EXPECT_EQ(results[1], "1000");
//     EXPECT_EQ(results[2], "4500");
//     EXPECT_EQ(results[3], "400");
// }
 
// // --- Ported from main.cpp Test 41: AND with two conditions -----------------
// TEST(FilterQuery, AndWithTwoConditionsReturnsMatchingNames) {
//     auto results = runQuery("test_data_filter.json",
//         "GET name FROM store.products WHERE category = electronics AND inStock = true");
//     // Monitor is electronics but inStock=false, so it's excluded.
//     ASSERT_EQ(results.size(), 3u);
//     EXPECT_EQ(results[0], "\"Laptop\"");
//     EXPECT_EQ(results[1], "\"Mouse\"");
//     EXPECT_EQ(results[2], "\"Wireless Mouse\"");
// }
// // TEMPORARY -- escape decoding verification 
// //Will delete this block once verified
// TEST(EscapeDecoding, BasicUnicodeEscape) {
//     auto results = runQuery("test_data_escapes.json", "basicUnicode");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"testa\"");  // \u0061 -> 'a'
// }

// // Decoded output intentionally contains literal (unescaped) quote characters : nodeToString is a human-readable display function, not a strict JSON re-serializer, so this is expected behavior, not a bug.
// TEST(EscapeDecoding, EscapedQuote) {
//     auto results = runQuery("test_data_escapes.json", "quoteEscape");
//     ASSERT_EQ(results.size(), 1u);
//     std::cout << "Actual output: " << results[0] << std::endl;
// }
 
// TEST(EscapeDecoding, EscapedBackslash) {
//     auto results = runQuery("test_data_escapes.json", "backslashEscape");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"C:\\Users\\bob\"");
// }
 
// TEST(EscapeDecoding, WhitespaceEscapes) {
//     auto results = runQuery("test_data_escapes.json", "whitespaceEscape");
//     ASSERT_EQ(results.size(), 1u);
//     std::cout << "Actual output: " << results[0] << std::endl;
// }

// // U+1F600 (grinning face emoji) via a UTF-16 surrogate pair (\uD83D\uDE00) decodes to UTF-8 bytes F0 9F 98 80. 
// TEST(EscapeDecoding, EmojiSurrogatePair) {
//     auto results = runQuery("test_data_escapes.json", "emojiSurrogatePair");
//     ASSERT_EQ(results.size(), 1u);
//     std::cout << "Actual output: " << results[0] << std::endl;
// }

// // WHERE comparisons against an escaped source value.
// TEST(EscapeDecoding, WhereMatchesEscapedSourceValue) {
//     // items[0].name is stored as "test\u0061" in the source JSON the query's plain-text "testa" must match the decoded value, not the raw escaped bytes.
//     auto results = runQuery("test_data_escapes.json", "GET sku FROM items WHERE name = testa");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"A1\"");
// }
 
// TEST(EscapeDecoding, WhereFastPathStillWorksForPlainValues) {
//     // Sanity check: the no-escape fast path (zero-copy string_view compare) still works correctly after the making new changes.
//     auto results = runQuery("test_data_escapes.json", "GET sku FROM items WHERE name = plain");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"B2\"");
// }

// // --- Case 2: special keys ("." and "") --------------------------------
// // Verifies the bracket-quoted-key grammar added to parsePath() lets a query address an object key that dot-notation itself can't express: a literal
// // "." key (which would collide with the path separator) and an empty-string key (which has no bare-identifier form at all).
// // Uses test_data_special_keys.json: {"": "computer", ".": "bob", "normal": "value"}

// TEST(SpecialKeyQuery, DotKeyReturnsCorrectValue) {
//     auto results = runQuery("test_data_special_keys.json", "[\".\"]");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"bob\"");
// }

// TEST(SpecialKeyQuery, EmptyStringKeyReturnsCorrectValue) {
//     auto results = runQuery("test_data_special_keys.json", "[\"\"]");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"computer\"");
// }

// // Regression check: a normal bare-key query still works correctly on the same file that also contains the special keys -- guards against the new "a segment may start with '[' " branch breaking the ordinary bare-key path.
// TEST(SpecialKeyQuery, NormalKeyStillWorksAlongsideSpecialKeys) {
//     auto results = runQuery("test_data_special_keys.json", "normal");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "\"value\"");
// }

// // A quoted key that doesn't exist should resolve to DNE, the same convention every other missing-key case in the engine already follows, not throw or crash.
// TEST(SpecialKeyQuery, NonexistentQuotedKeyReturnsDNE) {
//     auto results = runQuery("test_data_special_keys.json", "[\"doesNotExist\"]");
//     ASSERT_EQ(results.size(), 1u);
//     EXPECT_EQ(results[0], "DNE");
// }

TEST(FilterQuery, OrConditionReturnsUnionOfMatches) {
    auto results = runQuery("test_data_filter.json",
        "GET name FROM store.products WHERE category = furniture OR price > 4000");
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], "\"Wireless Mouse\"");
    EXPECT_EQ(results[1], "\"Desk\"");
    EXPECT_EQ(results[2], "\"Chair\"");
}

TEST(FilterQuery, NotConditionInvertsMatch) {
    auto results = runQuery("test_data_filter.json",
        "GET name FROM store.products WHERE NOT inStock = true");
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0], "\"Desk\"");
    EXPECT_EQ(results[1], "\"Monitor\"");
}
TEST(FilterQuery, NotOnMissingFieldNeverMatches) {
    auto results = runQuery("test_data_filter.json",
        "GET name FROM store.products WHERE NOT maker.location = USA");
    EXPECT_EQ(results.size(), 0u);
}

// Confirms AND binds tighter than OR: groups as (category = electronics AND inStock = true) OR (category = furniture)
TEST(FilterQuery, MixedAndOrGroupingBindsAndTighter) {
    auto results = runQuery("test_data_filter.json",
        "GET name FROM store.products WHERE category = electronics AND inStock = true OR category = furniture");
    ASSERT_EQ(results.size(), 5u);
    EXPECT_EQ(results[0], "\"Laptop\"");
    EXPECT_EQ(results[1], "\"Mouse\"");
    EXPECT_EQ(results[2], "\"Wireless Mouse\"");
    EXPECT_EQ(results[3], "\"Desk\"");
    EXPECT_EQ(results[4], "\"Chair\"");
}
}  // namespace