#pragma once
#include "AaronJsonParser/parser.h"
#include <vector>
#include <string>
#include <sstream>

// TREE VERIFIER — automated round-trip check
// Two checks:
//   1. verifyLeafValues()   — for every string/number/boolean/null node, confirms position/ePosition are in-bounds and the raw bytes at that range are actually well-formed for the claimed type.
//   2. verifyStructuralCounts() — counts objectStart/arrayStart/quoteStart tokens in the raw typeIndex pass and confirms those match the number of object/array/string nodes actually present in the built tree.

struct VerificationResult {
    bool passed = true;
    std::vector<std::string> issues;

    void fail(const std::string& msg) {
        passed = false;
        issues.push_back(msg);
    }
};

// ---- Check 1: leaf byte-range validation ----------------------------------

inline void verifyLeafValues(const parser::Node& node, const std::vector<char>& jsonData,
                              const std::string& pathSoFar, VerificationResult& result) {
    using NT = parser::NodeType;

    switch (node.nodeType) {
        case NT::object: {
            for (const auto& [key, child] : node.objectChildNode) {
                verifyLeafValues(child, jsonData, pathSoFar + "." + key, result);
            }
            break;
        }
        case NT::array: {
            size_t idx = 0;
            for (const auto& child : node.arrayChildNode) {
                verifyLeafValues(child, jsonData, pathSoFar + "[" + std::to_string(idx) + "]", result);
                ++idx;
            }
            break;
        }
        case NT::string: {
            if (node.position >= jsonData.size() || node.ePosition >= jsonData.size()) {
                result.fail(pathSoFar + ": string position/ePosition out of bounds ("
                            + std::to_string(node.position) + ".." + std::to_string(node.ePosition)
                            + ", buffer size " + std::to_string(jsonData.size()) + ")");
                break;
            }
            if (node.position > node.ePosition) {
                result.fail(pathSoFar + ": string position (" + std::to_string(node.position)
                            + ") is after ePosition (" + std::to_string(node.ePosition) + ")");
                break;
            }
            // position/ePosition point at the opening/closing '"' themselves (confirmed by printNode's own reconstruction: data+position+1 to data+ePosition, excluding both quote chars).
            if (jsonData[node.position] != '"') {
                result.fail(pathSoFar + ": string does not start with '\"' at position "
                            + std::to_string(node.position) + " (found '" + jsonData[node.position] + "')");
            }
            if (jsonData[node.ePosition] != '"') {
                result.fail(pathSoFar + ": string does not end with '\"' at ePosition "
                            + std::to_string(node.ePosition) + " (found '" + jsonData[node.ePosition] + "')");
            }
            break;
        }
        case NT::number: {
            if (node.position >= jsonData.size() || node.ePosition >= jsonData.size() ||
                node.position > node.ePosition) {
                result.fail(pathSoFar + ": number position/ePosition invalid ("
                            + std::to_string(node.position) + ".." + std::to_string(node.ePosition) + ")");
                break;
            }
            char first = jsonData[node.position];
            if (!(std::isdigit(static_cast<unsigned char>(first)) || first == '-')) {
                result.fail(pathSoFar + ": number does not start with digit/'-' (found '"
                            + std::string(1, first) + "')");
            }
            for (size_t k = node.position; k <= node.ePosition; ++k) {
                char c = jsonData[k];
                bool validNumChar = std::isdigit(static_cast<unsigned char>(c)) ||
                                     c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
                if (!validNumChar) {
                    result.fail(pathSoFar + ": non-numeric character '" + std::string(1, c)
                                + "' inside number range at index " + std::to_string(k));
                    break;
                }
            }
            break;
        }
        case NT::boolean: {
            if (node.position >= jsonData.size() || node.ePosition >= jsonData.size() ||
                node.position > node.ePosition) {
                result.fail(pathSoFar + ": boolean position/ePosition invalid");
                break;
            }
            std::string raw(jsonData.data() + node.position, node.ePosition - node.position + 1);
            if (raw != "true" && raw != "false") {
                result.fail(pathSoFar + ": boolean raw bytes are neither 'true' nor 'false' (found '"
                            + raw + "')");
            }
            break;
        }
        case NT::null: {
            if (node.position >= jsonData.size() || node.ePosition >= jsonData.size() ||
                node.position > node.ePosition) {
                result.fail(pathSoFar + ": null position/ePosition invalid");
                break;
            }
            std::string raw(jsonData.data() + node.position, node.ePosition - node.position + 1);
            if (raw != "null") {
                result.fail(pathSoFar + ": null raw bytes do not spell 'null' (found '" + raw + "')");
            }
            break;
        }
    }
}

// ---- Check 2: structural count validation ----------------------------------
// Counts container/string nodes actually present in the built tree, to compare against the raw typeIndex token counts. Object/array nodes have no position/ePosition to check directly, so this is how they get covered.

struct NodeCounts {
    size_t objects = 0;
    size_t arrays = 0;
    size_t strings = 0;
    size_t numbers = 0;
    size_t booleans = 0;
    size_t nulls = 0;
};

inline void countTreeNodes(const parser::Node& node, NodeCounts& counts) {
    using NT = parser::NodeType;
    switch (node.nodeType) {
        case NT::object:
            ++counts.objects;
            for (const auto& [key, child] : node.objectChildNode) countTreeNodes(child, counts);
            break;
        case NT::array:
            ++counts.arrays;
            for (const auto& child : node.arrayChildNode) countTreeNodes(child, counts);
            break;
        case NT::string:  ++counts.strings;  break;
        case NT::number:  ++counts.numbers;  break;
        case NT::boolean: ++counts.booleans; break;
        case NT::null:    ++counts.nulls;    break;
    }
}

inline void verifyStructuralCounts(const parser::Node& root,
                                    const std::vector<parser::TypeStruct>& typeIndex,
                                    VerificationResult& result) {
    using T = parser::Type;

    NodeCounts treeCounts;
    countTreeNodes(root, treeCounts);

    // Raw pass counts. Note: typeIndex records BOTH quoteStart and quoteEnd for every string, plus a separate string-typed rewrite in-place
    // indexStructure(): the quoteStart entry gets overwritten to type=string once its matching quoteEnd is found. So counting Type::string in typeIndex counts each string token exactly once, which includes OBJECT KEYS as well as string values. To compare apples to apples, this check counts raw string tokens minus the ones consumed as keys.
    size_t rawObjectStarts = 0, rawArrayStarts = 0, rawStringTokens = 0, rawKeyTokens = 0;
    for (size_t i = 0; i < typeIndex.size(); ++i) {
        if (typeIndex[i].type == T::objectStart) ++rawObjectStarts;
        else if (typeIndex[i].type == T::arrayStart) ++rawArrayStarts;
        else if (typeIndex[i].type == T::string) {
            ++rawStringTokens;
            bool nextIsColon = (i + 1 < typeIndex.size()) && (typeIndex[i + 1].type == T::colon);
            if (nextIsColon) ++rawKeyTokens;
        }
    }
    size_t rawValueStrings = rawStringTokens - rawKeyTokens;

    if (treeCounts.objects != rawObjectStarts) {
        result.fail("object count mismatch: tree has " + std::to_string(treeCounts.objects)
                    + ", raw buffer has " + std::to_string(rawObjectStarts) + " '{' tokens");
    }
    if (treeCounts.arrays != rawArrayStarts) {
        result.fail("array count mismatch: tree has " + std::to_string(treeCounts.arrays)
                    + ", raw buffer has " + std::to_string(rawArrayStarts) + " '[' tokens");
    }
    if (treeCounts.strings != rawValueStrings) {
        result.fail("string VALUE count mismatch: tree has " + std::to_string(treeCounts.strings)
                    + ", raw buffer has " + std::to_string(rawValueStrings)
                    + " string tokens not used as object keys");
    }
}

// ---- Entry point ------------------------------------------------------------

inline VerificationResult verifyTree(const parser::Node& root, const std::vector<char>& jsonData,
                                      const std::vector<parser::TypeStruct>& typeIndex) {
    VerificationResult result;
    verifyLeafValues(root, jsonData, "$", result);
    verifyStructuralCounts(root, typeIndex, result);
    return result;
}