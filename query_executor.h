#pragma once
#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryStruct.h"
#include <vector>
#include<string>
#include<string_view>
#include<list>
#include<iterator>
 
// query_executor.h — now consuming Tasnim's PathPart/DotPathQuery directly
//New: PathPartType::ArrayIndex supportdescends into exactly one array element by position, rather than expanding every element (AllElements)

inline void executeStep(const parser::Node* node, const std::vector<PathPart>& path, size_t stepIndex, std::vector<const parser::Node*>& results) {
    if (!node) {
        results.push_back(nullptr);
        return;
    }
    if (stepIndex == path.size()) {
        results.push_back(node);
        return;
    }
 
    const PathPart& part = path[stepIndex];
 
    switch (part.type) {
        case PathPartType::Key: {
            if (node->nodeType == parser::NodeType::object) {
                auto it = node->objectChildNode.find(part.key);
                if (it != node->objectChildNode.end()) {
                    executeStep(&(it->second), path, stepIndex + 1, results);
                    return;
                }
            }
            results.push_back(nullptr);
            break;
        }
 
        case PathPartType::AllElements: {
            if (node->nodeType == parser::NodeType::array) {
                for (const auto& elem : node->arrayChildNode) {
                    executeStep(&elem, path, stepIndex + 1, results);
                }
            } else {
                results.push_back(nullptr);
            }
            break;
        }
 
        case PathPartType::ArrayIndex: {
            if (node->nodeType == parser::NodeType::array &&
                part.index >= 0 &&
                static_cast<size_t>(part.index) < node->arrayChildNode.size()) {
                auto it = node->arrayChildNode.begin();
                std::advance(it, part.index);
                executeStep(&(*it), path, stepIndex + 1, results);
            } else {
                results.push_back(nullptr);
            }
            break;
        }
    }
}

// [NEW] Sprint 2 optimization: a non-allocating version of executeStep() for paths that only ever need to resolve to one node, which is exactly what a WHERE field is (no wildcards expected there). executeStep() heap-allocates a std::vector on every call, even when there's only ever going to be a single result — for a WHERE with N conditions scanning M rows, that's up to N*M allocations doing work a single pointer return could do instead. If the path hits a wildcard mid-traversal, that's ambiguous for a single-result resolve, so this returns nullptr (same effective behavior as the old fieldResults.empty() check — a WHERE condition against a wildcarded field simply doesn't match).
inline const parser::Node* resolveSingle(const parser::Node* node, const std::vector<PathPart>& path, size_t stepIndex) {
    if (!node) return nullptr;
    if (stepIndex == path.size()) return node;

    const PathPart& part = path[stepIndex];

    switch (part.type) {
        case PathPartType::Key: {
            if (node->nodeType == parser::NodeType::object) {
                auto it = node->objectChildNode.find(part.key);
                if (it != node->objectChildNode.end()) {
                    return resolveSingle(&(it->second), path, stepIndex + 1);
                }
            }
            return nullptr;
        }

        case PathPartType::ArrayIndex: {
            if (node->nodeType == parser::NodeType::array &&
                part.index >= 0 &&
                static_cast<size_t>(part.index) < node->arrayChildNode.size()) {
                auto it = node->arrayChildNode.begin();
                std::advance(it, part.index);
                return resolveSingle(&(*it), path, stepIndex + 1);
            }
            return nullptr;
        }

        case PathPartType::AllElements:
            // wildcard can produce multiple nodes -- not representable as a single result, so WHERE fields with a wildcard fall back to "no match" here.
            return nullptr;
    }
    return nullptr;
}
 
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const DotPathQuery& query) {
    std::vector<const parser::Node*> results;
    executeStep(&root, query.path, 0, results);
    return results;
}
 
// Convenience overload: takes the full Query (as returned by QueryParser::parse()) and pulls out the dot-path automatically.
/* [CHANGED] previously this just unwrapped query.dotPath unconditionally, with no awareness that Query could also represent a filter query. It now checks query.type first and throws a clear error if it's a filter query. Callers running a filter query must use the new 3-arg executeQuery(root, query, jsonData) overload defined near the bottom of this file instead.*/
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const Query& query) {
    if (query.type == QueryType::Filter) {
        throw std::runtime_error(
            "Filter queries require the jsonData buffer: use executeQuery(root, query, jsonData) instead."
        );
    }
    return executeQuery(root, query.dotPath);
}
 
// Node leaves only store position/ePosition, so printing or reading a leaf's actual value requires the raw jsonData buffer too. For object/array nodes, this recursively rebuilds a JSON string from the children so that query results that resolve to a whole object/array print real data.
// [CHANGED] per TA feedback: a null node pointer means the query itself resolved to nothing -- key not found, array index out of range, wildcard on a non-array, type mismatch, etc. That's not the same thing as the data actually containing a JSON `null` literal, so it now prints "DNE" instead of "null" here, keeping "null" reserved for the real thing (see the NodeType::null case below).
inline std::string nodeToString(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "DNE";
    switch (node->nodeType) {
        case parser::NodeType::string:
            return "\"" + std::string(jsonData.data() + node->position + 1, jsonData.data() + node->ePosition) + "\"";
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string(jsonData.data() + node->position, jsonData.data() + node->ePosition + 1);
        case parser::NodeType::null:
            // [NEW] this is the real thing -- an actual `null` literal present in the source JSON, distinct from the !node "DNE" case above.
            return "null";
        case parser::NodeType::object: {
            std::string result = "{";
            bool first = true;
            for (const auto& [key, child] : node->objectChildNode) {
                if (!first) result += ",";
                first = false;
                result += "\"" + key + "\":" + nodeToString(&child, jsonData);
            }
            result += "}";
            return result;
        }
        case parser::NodeType::array: {
            std::string result = "[";
            bool first = true;
            for (const auto& child : node->arrayChildNode) {
                if (!first) result += ",";
                first = false;
                result += nodeToString(&child, jsonData);
            }
            result += "]";
            return result;
        }
        default:
            // [CHANGED] unreachable in practice (every NodeType is handled above) but kept as a defensive fallback -- labeled DNE rather than null since an unrecognized/corrupted node isn't a valid parsed null value either.
            return "DNE";
    }
}

// [NEW] its like nodeToString, but returns the bare value with no surrounding quotes, its needed here because we are comparing values (WHERE age > 30), not just displaying them for the user. Only string/number/boolean leaves have a comparable raw value; object, array, and null nodes return "" (so a WHERE condition against one of those always fails to match, rather than crashing).
// [CHANGED] Sprint 2 optimization: returns std::string_view instead of std::string now -- a view into the existing jsonData buffer, no copy. compareValues()'s string/boolean branch compares directly against this view with zero allocations, instead of always building a new std::string first even for a plain equality check.
inline std::string_view nodeRawValue(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return std::string_view();
    switch (node->nodeType) {
        case parser::NodeType::string:
            return std::string_view(jsonData.data() + node->position + 1,
                                static_cast<size_t>(node->ePosition - node->position - 1));
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string_view(jsonData.data() + node->position,
                                static_cast<size_t>(node->ePosition - node->position + 1));
        default:
            return std::string_view();
    }
}

// [NEW] Applies a single WHERE comparison: numeric compare for number nodes (parses both sides as double), lexical/string compare otherwise (covers string and boolean values, e.g. WHERE active = true). If the numeric parse fails on either side (e.g. WHERE age > "abc"), the comparison is treated as a non-match instead of throwing and aborting the whole query.
inline bool compareValues(const parser::Node* node, const std::vector<char>& jsonData,
                           FilterOperators op, const std::string& conditionValue) {
    if (!node) return false;

    if (node->nodeType == parser::NodeType::number) {
        double lhs, rhs;
        try {
            // [CHANGED] nodeRawValue() now returns a string_view, so it's wrapped in std::string(...) here since std::stod needs a null-terminated string -- numeric literals are short enough that this hits small-string-optimization and doesn't actually heap-allocate on any mainstream standard library.
            lhs = std::stod(std::string(nodeRawValue(node, jsonData)));
            rhs = std::stod(conditionValue);
        } catch (const std::exception&) {
            return false; // non-numeric value on either side: no match
        }

        switch (op) {
            case FilterOperators::Equal:              return lhs == rhs;
            case FilterOperators::NotEqual:           return lhs != rhs;
            case FilterOperators::LessThan:           return lhs <  rhs;
            case FilterOperators::LessThanOrEqual:    return lhs <= rhs;
            case FilterOperators::GreaterThan:        return lhs >  rhs;
            case FilterOperators::GreaterThanOrEqual: return lhs >= rhs;
        }
        return false;
    }

        // string / boolean: lexical compare. <, <=, >, >= still work
    // (alphabetical ordering) but are mainly meant for Equal/NotEqual here.
    // [CHANGED] lhs is now a string_view straight from nodeRawValue() -- compares directly against conditionValue with zero allocation, unlike before where nodeRawValue() always built a std::string first.
    std::string_view lhs = nodeRawValue(node, jsonData);

    switch (op) {
        case FilterOperators::Equal:              return lhs == conditionValue;
        case FilterOperators::NotEqual:           return lhs != conditionValue;
        case FilterOperators::LessThan:           return lhs <  conditionValue;
        case FilterOperators::LessThanOrEqual:    return lhs <= conditionValue;
        case FilterOperators::GreaterThan:        return lhs >  conditionValue;
        case FilterOperators::GreaterThanOrEqual: return lhs >= conditionValue;
    }
    return false;
}

/* [NEW] The actual filter algorithm. Three steps, matching GET/FROM/WHERE:
   1. Resolve FROM (filter.sourcePath) via the existing executeStep(), reused unchanged to find the array to scan.
   2. For every element (row) in that array, evaluate every WHERE condition against it using executeStep() + compareValues(). A row must pass ALL conditions to be included (AND semantics).
   3. For rows that pass, resolve GET (filter.selectField) relative to that row via executeStep() again, and collect the result(s). */
/* [CHANGED] Sprint 2 optimization: step 2 now uses resolveSingle() instead of executeStep() for the per-condition field lookup, since a WHERE field only ever needs one node, not a vector -- this removes one heap allocation per condition per row. Also added a results.reserve() before the row loop so appending matched rows doesn't repeatedly reallocate as the source array is scanned. */

inline std::vector<const parser::Node*> executeFilterQuery(const parser::Node& root,
    const FilterQuery& filter, const std::vector<char>& jsonData) {
    std::vector<const parser::Node*> results;

    // Step 1 (FROM): resolve the source array node(s).
    std::vector<const parser::Node*> sources;
    executeStep(&root, filter.sourcePath, 0, sources);

    for (const parser::Node* source : sources) {
        if (!source || source->nodeType != parser::NodeType::array) {
            continue; // FROM must resolve to an array; skip null/non-array sources
        }

        // [NEW] reserve up front so appending matched rows doesn't repeatedly reallocate results as the source array is scanned.
        results.reserve(results.size() + source->arrayChildNode.size());

        // Step 2 (WHERE): scan every row in the source array.
        for (const auto& rowNode : source->arrayChildNode) {
            bool include = true;

            for (const Condition& cond : filter.conditions) {
                // [CHANGED] resolveSingle() instead of executeStep() -- no vector allocation for what's always a single-node lookup.
                const parser::Node* fieldNode = resolveSingle(&rowNode, cond.field, 0);

                // WHERE expects a single scalar field per row (no wildcards in the condition field) — if it resolved to nothing, or the comparison fails, the row is excluded.
                if (!fieldNode ||
                    !compareValues(fieldNode, jsonData, cond.comparisonOp, cond.value)) {
                    include = false;
                    break;
                }
            }

            if (!include) continue;

            // Step 3 (GET): resolve the select field relative to this row and collect it.
            std::vector<const parser::Node*> selected;
            executeStep(&rowNode, filter.selectField, 0, selected);
            for (const parser::Node* sel : selected) {
                results.push_back(sel);
            }
        }
    }

    return results;
}

// [NEW] Entry point for filter queries. Unlike the dot-path executeQuery overloads above, this one takes jsonData, because WHERE comparisons need to read actual values out of the raw buffer, not just locate matching nodes. This is a new overload (3 arguments instead of 2), so it does not conflict with or replace the existing executeQuery(root, query) above callers (functions) pick whichever overload matches the query type they're running.
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const Query& query,
                                                        const std::vector<char>& jsonData) {
    if (query.type == QueryType::Filter) {
        return executeFilterQuery(root, query.filter, jsonData);
    }
    return executeQuery(root, query.dotPath);
}