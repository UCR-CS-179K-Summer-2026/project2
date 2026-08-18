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
            return nullptr;
    }
    return nullptr;
}
 
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const DotPathQuery& query) {
    std::vector<const parser::Node*> results;
    executeStep(&root, query.path, 0, results);
    return results;
}
 
inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const Query& query) {
    if (query.type == QueryType::Filter) {
        throw std::runtime_error(
            "Filter queries require the jsonData buffer: use executeQuery(root, query, jsonData) instead."
        );
    }
    return executeQuery(root, query.dotPath);
}
 
inline void appendUtf8(std::string& out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (codepoint >> 18));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

inline long parseHex4(std::string_view s, size_t i) {
    if (i + 4 > s.size()) return -1;
    long value = 0;
    for (size_t k = 0; k < 4; ++k) {
        char c = s[i + k];
        value <<= 4;
        if (c >= '0' && c <= '9') value |= (c - '0');
        else if (c >= 'a' && c <= 'f') value |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') value |= (c - 'A' + 10);
        else return -1;
    }
    return value;
}

inline std::string decodeJsonString(std::string_view raw) {
    if (raw.find('\\') == std::string_view::npos) {
        return std::string(raw);
    }

    std::string out;
    out.reserve(raw.size());

    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (c != '\\' || i + 1 >= raw.size()) {
            out += c;
            continue;
        }
        char next = raw[i + 1];
        switch (next) {
            case '"':  out += '"';  i += 1; break;
            case '\\': out += '\\'; i += 1; break;
            case '/':  out += '/';  i += 1; break;
            case 'b':  out += '\b'; i += 1; break;
            case 'f':  out += '\f'; i += 1; break;
            case 'n':  out += '\n'; i += 1; break;
            case 'r':  out += '\r'; i += 1; break;
            case 't':  out += '\t'; i += 1; break;
            case 'u': {
                long hi = parseHex4(raw, i + 2);
                if (hi < 0) {
                    out += c;
                    break;
                }
                i += 5;

                if (hi >= 0xD800 && hi <= 0xDBFF) {
                    if (i + 2 < raw.size() && raw[i + 1] == '\\' && raw[i + 2] == 'u') {
                        long lo = parseHex4(raw, i + 3);
                        if (lo >= 0xDC00 && lo <= 0xDFFF) {
                            unsigned int codepoint =
                                0x10000 + ((static_cast<unsigned int>(hi) - 0xD800) << 10) +
                                (static_cast<unsigned int>(lo) - 0xDC00);
                            appendUtf8(out, codepoint);
                            i += 6;
                            break;
                        }
                    }
                    appendUtf8(out, 0xFFFD);
                } else if (hi >= 0xDC00 && hi <= 0xDFFF) {
                    appendUtf8(out, 0xFFFD);
                } else {
                    appendUtf8(out, static_cast<unsigned int>(hi));
                }
                break;
            }
            default:
                out += c;
                out += next;
                i += 1;
                break;
        }
    }
    return out;
}

inline std::string nodeToString(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "DNE";
    switch (node->nodeType) {
        case parser::NodeType::string: {
            std::string_view raw(jsonData.data() + node->position + 1,
                                  static_cast<size_t>(node->ePosition - node->position - 1));
            return "\"" + decodeJsonString(raw) + "\"";
        }
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
            return "DNE";
    }
}

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

inline bool compareValues(const parser::Node* node, const std::vector<char>& jsonData,
                           FilterOperators op, const std::string& conditionValue) {
    if (!node) return false;

    if (node->nodeType == parser::NodeType::number) {
        double lhs, rhs;
        try {
            lhs = std::stod(std::string(nodeRawValue(node, jsonData)));
            rhs = std::stod(conditionValue);
        } catch (const std::exception&) {
            return false;
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

    std::string_view lhs = nodeRawValue(node, jsonData);
    std::string lhsDecoded;
    std::string_view lhsCompare = lhs;
    if (lhs.find('\\') != std::string_view::npos) {
        lhsDecoded = decodeJsonString(lhs);
        lhsCompare = lhsDecoded;
    }

    switch (op) {
        case FilterOperators::Equal:              return lhsCompare == conditionValue;
        case FilterOperators::NotEqual:           return lhsCompare != conditionValue;
        case FilterOperators::LessThan:           return lhsCompare <  conditionValue;
        case FilterOperators::LessThanOrEqual:    return lhsCompare <= conditionValue;
        case FilterOperators::GreaterThan:        return lhsCompare >  conditionValue;
        case FilterOperators::GreaterThanOrEqual: return lhsCompare >= conditionValue;
    }
    return false;
}

// [NEW] Evaluates a single WHERE condition against one row, honoring the
// `negative` flag (NOT). Design decision: a missing field NEVER matches,
// negated or not -- "missing" is its own category (same principle as the
// DNE-vs-null distinction elsewhere in this file), not something NOT can
// flip into a match. This keeps "missing" meaning the same thing whether
// or not the condition is wrapped in NOT.
inline bool evaluateCondition(const parser::Node* row, const Condition& cond,
                               const std::vector<char>& jsonData) {
    const parser::Node* fieldNode = resolveSingle(row, cond.field, 0);
    if (!fieldNode) {
        return false; // missing field: never matches, regardless of NOT
    }
    bool result = compareValues(fieldNode, jsonData, cond.comparisonOp, cond.value);
    return cond.negative ? !result : result;
}

// [NEW] Evaluates the full WHERE clause for one row, supporting AND/OR.
// wordOperator is stored on the PRECEDING condition (set by the parser when
// it sees AND/OR after a condition), so conditions[i].wordOperator is the
// operator connecting conditions[i] and conditions[i+1].
//
// Semantics: AND binds tighter than OR (same convention as most languages,
// and the only sensible default given no parentheses support), so the
// condition list is treated as OR-separated groups of AND-connected
// conditions -- e.g. "a AND b OR c AND d" groups as (a AND b) OR (c AND d).
//
// Short-circuits at both levels: stops evaluating a group the moment one of
// its conditions fails (no point checking the rest of an AND-group that's
// already false), and stops evaluating further groups the moment one group
// fully matches (no point checking further OR-groups once the row is
// already included). A conditions list with no OR at all collapses to
// exactly one group, which reproduces the original AND-only short-circuit
// behavior unchanged.
inline bool evaluateWhere(const parser::Node* row, const std::vector<Condition>& conditions,
                          const std::vector<char>& jsonData) {
    if (conditions.empty()) return true; // no WHERE clause: every row matches

    bool anyGroupMatched = false;
    bool currentGroupFailed = false;

    for (size_t i = 0; i < conditions.size(); ++i) {
        if (!currentGroupFailed) {
            if (!evaluateCondition(row, conditions[i], jsonData)) {
                currentGroupFailed = true; // short-circuit: skip rest of this AND-group
            }
        }

        bool isLast = (i + 1 == conditions.size());
        bool groupEndsHere = isLast || conditions[i].wordOperator == WordOperators::Or;

        if (groupEndsHere) {
            if (!currentGroupFailed) {
                anyGroupMatched = true;
                break; // short-circuit: whole WHERE is true, stop checking further groups
            }
            currentGroupFailed = false; // reset for the next OR-group
        }
    }

    return anyGroupMatched;
}

inline std::vector<const parser::Node*> executeFilterQuery(const parser::Node& root,
    const FilterQuery& filter, const std::vector<char>& jsonData) {
    std::vector<const parser::Node*> results;

    std::vector<const parser::Node*> sources;
    executeStep(&root, filter.sourcePath, 0, sources);

    for (const parser::Node* source : sources) {
        if (!source || source->nodeType != parser::NodeType::array) {
            continue;
        }

        results.reserve(results.size() + source->arrayChildNode.size());

        for (const auto& rowNode : source->arrayChildNode) {
            // [CHANGED] was an inline AND-only loop; now delegates to
            // evaluateWhere(), which supports AND/OR/NOT with short-circuit
            // preserved at both the AND-group and OR-group level.
            if (!evaluateWhere(&rowNode, filter.conditions, jsonData)) continue;

            std::vector<const parser::Node*> selected;
            executeStep(&rowNode, filter.selectField, 0, selected);
            for (const parser::Node* sel : selected) {
                results.push_back(sel);
            }
        }
    }

    return results;
}

inline std::vector<const parser::Node*> executeQuery(const parser::Node& root, const Query& query,
                                                        const std::vector<char>& jsonData) {
    if (query.type == QueryType::Filter) {
        return executeFilterQuery(root, query.filter, jsonData);
    }
    return executeQuery(root, query.dotPath);
}