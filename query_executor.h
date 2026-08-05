#pragma once
#include "AaronJsonParser/parser.h"
#include "TasnimQueryParser/QueryStruct.h"
#include <vector>
#include<string>
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
inline std::string nodeToString(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "null";
    switch (node->nodeType) {
        case parser::NodeType::string:
            return "\"" + std::string(jsonData.data() + node->position + 1, jsonData.data() + node->ePosition) + "\"";
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string(jsonData.data() + node->position, jsonData.data() + node->ePosition + 1);
        case parser::NodeType::null:
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
            return "null";
    }
}

// [NEW] its like nodeToString, but returns the bare value with no surrounding quotes, its needed here because we are comparing values (WHERE age > 30), not just displaying them for the user. Only string/number/boolean leaves have a comparable raw value; object, array, and null nodes return "" (so a WHERE condition against one of those always fails to match, rather than crashing).
inline std::string nodeRawValue(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "";
    switch (node->nodeType) {
        case parser::NodeType::string:
            return std::string(jsonData.data() + node->position + 1,
                                jsonData.data() + node->ePosition);
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string(jsonData.data() + node->position,
                                jsonData.data() + node->ePosition + 1);
        default:
            return "";
    }
}

// [NEW] Applies a single WHERE comparison: numeric compare for number nodes (parses both sides as double), lexical/string compare otherwise (covers string and boolean values, e.g. WHERE active = true). If the numeric parse fails on either side (e.g. WHERE age > "abc"), the comparison is treated as a non-match instead of throwing and aborting the whole query.
inline bool compareValues(const parser::Node* node, const std::vector<char>& jsonData,
                           FilterOperators op, const std::string& conditionValue) {
    if (!node) return false;

    if (node->nodeType == parser::NodeType::number) {
        double lhs, rhs;
        try {
            lhs = std::stod(nodeRawValue(node, jsonData));
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
    std::string lhs = nodeRawValue(node, jsonData);

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

        // Step 2 (WHERE): scan every row in the source array.
        for (const auto& rowNode : source->arrayChildNode) {
            bool include = true;

            for (const Condition& cond : filter.conditions) {
                std::vector<const parser::Node*> fieldResults;
                executeStep(&rowNode, cond.field, 0, fieldResults);

                // WHERE expects a single scalar field per row (no wildcards in the condition field) — if it resolved to nothing, or the comparison fails, the row is excluded.
                if (fieldResults.empty() ||
                    !compareValues(fieldResults[0], jsonData, cond.comparisonOp, cond.value)) {
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