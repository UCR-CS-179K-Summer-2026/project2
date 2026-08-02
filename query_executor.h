//taking parsed json tree fro mAaron and parsed query from Tasnim and executing the query on the json tree and returning the result
//Algorithm: brute-force, full traversal, no indexing At every step we just look at the current node and follow the next QueryStep
// When we hit a Wildcard step, we branch into every array element and continue independently for each one. 

#pragma once
 
#include "json_value.h"
#include "query_ast.h"
#include <vector>
 
// Recursive brute-force walk, appends matches (or nullptr for missing paths) to "results".
inline void executeStep(const JsonValuePtr& node,
                         const QueryAST& ast,
                         size_t stepIndex,
                         std::vector<JsonValuePtr>& results) {
    // Base case: we've consumed every step in the query, so this node is a result.
    if (stepIndex == ast.size()) {
        results.push_back(node);
        return;
    }
 
    const QueryStep& step = ast[stepIndex];
 
    if (step.kind == StepKind::Field) {
        // Need an object to look up a field on.
        if (!node || node->type != JsonType::Object) {
            results.push_back(nullptr);
            return;
        }
        auto it = node->objVal.find(step.fieldName);
        if (it == node->objVal.end()) {
            results.push_back(nullptr); // field missing on this node
            return;
        }
        executeStep(it->second, ast, stepIndex + 1, results);
 
    } else { // StepKind::Wildcard. Need an array to expand.
        if (!node || node->type != JsonType::Array) {
            results.push_back(nullptr);
            return;
        }
        // Brute-force: visit every element, no shortcuts.
        for (const auto& elem : node->arrVal) {
            executeStep(elem, ast, stepIndex + 1, results);
        }
    }
}
 
// Public entry point: run a parsed query (QueryAST) against a parsed JSON tree (JsonValuePtr) and get back the list of matching values.
inline std::vector<JsonValuePtr> executeQuery(const JsonValuePtr& root, const QueryAST& ast) {
    std::vector<JsonValuePtr> results;
    executeStep(root, ast, 0, results);
    return results;
}