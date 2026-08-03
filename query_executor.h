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
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const Query& query) {
    return executeQuery(root, query.dotPath);
}
 
// Node leaves only store position/ePosition, so printing or reading a leaf's actual value requires the raw jsonData buffer too.
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