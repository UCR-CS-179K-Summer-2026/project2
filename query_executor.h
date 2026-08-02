//taking parsed json tree fro mAaron and parsed query from Tasnim and executing the query on the json tree and returning the result
//Algorithm: brute-force, full traversal, no indexing At every step we just look at the current node and follow the next QueryStep
// When we hit a Wildcard step, we branch into every array element and continue independently for each one. 

#pragma once
#include "AaronJsonParser/parser.h"
#include "query_ast.h"
#include <vector>
#include<string>
 
inline void executeStep(const parser::Node* node, const QueryAST& ast,
                         size_t stepIndex, std::vector<const parser::Node*>& results) {
    if (!node) {
        results.push_back(nullptr);
        return;
    }
    if (stepIndex == ast.size()) {
        results.push_back(node);
        return;
    }
 
    const QueryStep& step = ast[stepIndex];
 
    if (step.kind == StepKind::Field) {
        if (node->nodeType == parser::NodeType::object) {
            auto it = node->objectChildNode.find(step.fieldName);
            if (it != node->objectChildNode.end()) {
                executeStep(&(it->second), ast, stepIndex + 1, results);
                return;
            }
        }
        results.push_back(nullptr);
    } else { // StepKind::Wildcard
        if (node->nodeType == parser::NodeType::array) {
            for (const auto& elem : node->arrayChildNode) {
                executeStep(&elem, ast, stepIndex + 1, results);
            }
        } else {
            results.push_back(nullptr);
        }
    }
}
 
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const QueryAST& ast) {
    std::vector<const parser::Node*> results;
    executeStep(&root, ast, 0, results);
    return results;
}
 
// Node leaves only store position/ePosition (lazy extraction), so printing
// or reading a leaf's actual value requires the raw jsonData buffer too.
inline std::string nodeToString(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "null";
    switch (node->nodeType) {
        case parser::NodeType::string:
            return "\"" + std::string(jsonData.data() + node->position + 1,
                                       jsonData.data() + node->ePosition) + "\"";
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string(jsonData.data() + node->position,
                                jsonData.data() + node->ePosition + 1);
        case parser::NodeType::null:
            return "null";
        default:
            return "<complex>";
    }
}