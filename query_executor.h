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
 
// [NEW] Encodes a single Unicode codepoint as UTF-8 bytes, appended to out.
// Used by decodeJsonString() below to turn \uXXXX escapes (and surrogate
// pairs) into their real character bytes.
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

// [NEW] Parses exactly 4 hex digits starting at s[i]. Returns -1 if there
// aren't 4 characters left, or any of them isn't a valid hex digit.
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

// [NEW] Decodes JSON string escapes (\", \\, \/, \b, \f, \n, \r, \t, and \uXXXX -- including UTF-16 surrogate pairs like \uD83D\uDE00 for characters outside the Basic Multilingual Plane, e.g. emoji) into their real UTF-8 bytes.
// Fast path: if raw contains no backslash at all (the common case for most real-world data), this just copies it as-is with no per-character processing
// Malformed escapes (invalid hex digits, unmatched surrogates, an unknown escape letter) are handled defensively rather than thrown: unmatched surrogates become the U+FFFD replacement character, and other malformed sequences are emitted as-is. This keeps display robust for slightly invalid input rather than aborting the whole query.
inline std::string decodeJsonString(std::string_view raw) {
    if (raw.find('\\') == std::string_view::npos) {
        return std::string(raw); // fast path -- no escapes present
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
                    // malformed \u escape emit the backslash as-is and let the next loop iteration process 'u' and the following characters as plain text.
                    out += c;
                    break;
                }
                i += 5; // consumed \uXXXX (backslash + 'u' + 4 hex digits)

                if (hi >= 0xD800 && hi <= 0xDBFF) {
                    // High surrogate: a valid character requires an immediately following low surrogate \uXXXX.
                    if (i + 2 < raw.size() && raw[i + 1] == '\\' && raw[i + 2] == 'u') {
                        long lo = parseHex4(raw, i + 3);
                        if (lo >= 0xDC00 && lo <= 0xDFFF) {
                            unsigned int codepoint =
                                0x10000 + ((static_cast<unsigned int>(hi) - 0xD800) << 10) +
                                (static_cast<unsigned int>(lo) - 0xDC00);
                            appendUtf8(out, codepoint);
                            i += 6; // consumed the low surrogate's \uXXXX too
                            break;
                        }
                    }
                    // Unmatched high surrogate : no valid low surrogate followed.
                    appendUtf8(out, 0xFFFD);
                } else if (hi >= 0xDC00 && hi <= 0xDFFF) {
                    // Unmatched low surrogate (appeared without a preceding high).
                    appendUtf8(out, 0xFFFD);
                } else {
                    // Ordinary Basic-Multilingual-Plane codepoint, no pairing needed.
                    appendUtf8(out, static_cast<unsigned int>(hi));
                }
                break;
            }
            default:
                // Unrecognized escape letter -- emit both characters as-is
                // rather than throwing.
                out += c;
                out += next;
                i += 1;
                break;
        }
    }
    return out;
}

// Node leaves only store position/ePosition, so printing or reading a leaf's actual value requires the raw jsonData buffer too. For object/array nodes, this recursively rebuilds a JSON string from the children so that query results that resolve to a whole object/array print real data.
// [CHANGED] per TA feedback: a null node pointer means the query itself resolved to nothing -- key not found, array index out of range, wildcard on a non-array, type mismatch, etc. That's not the same thing as the data actually containing a JSON `null` literal, so it now prints "DNE" instead of "null" here, keeping "null" reserved for the real thing (see the NodeType::null case below).
inline std::string nodeToString(const parser::Node* node, const std::vector<char>& jsonData) {
    if (!node) return "DNE";
    switch (node->nodeType) {
        case parser::NodeType::string: {
            // [CHANGED] previously returned the raw byte slice unmodified, so an escaped source value like "\u0061" printed the six literal escape characters instead of the character it represents ('a'). Now decodes standard JSON escapes (including \uXXXX / surrogate pairs) via decodeJsonString() before wrapping in display quotes.
            std::string_view raw(jsonData.data() + node->position + 1,
                                  static_cast<size_t>(node->ePosition - node->position - 1));
            return "\"" + decodeJsonString(raw) + "\"";
        }
        case parser::NodeType::number:
        case parser::NodeType::boolean:
            return std::string(jsonData.data() + node->position, jsonData.data() + node->ePosition + 1);
        case parser::NodeType::null:
            // [NEW] this is the real thing, an actual `null` literal present in the source JSON, distinct from the !node "DNE" case above.
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
            // [CHANGED] unreachable in practice (every NodeType is handled above) but kept as a defensive fallback, labeled DNE rather than null since an unrecognized/corrupted node isn't a valid parsed null value either.
            return "DNE";
    }
}

// [NEW] its like nodeToString, but returns the bare value with no surrounding quotes, its needed here because we are comparing values (WHERE age > 30), not just displaying them for the user. Only string/number/boolean leaves have a comparable raw value; object, array, and null nodes return "" (so a WHERE condition against one of those always fails to match, rather than crashing).
// [CHANGED] Sprint 2 optimization: returns std::string_view instead of std::string now a view into the existing jsonData buffer, no copy. compareValues()'s string/boolean branch compares directly against this view with zero allocations, instead of always building a new std::string first even for a plain equality check.
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
    // [CHANGED]: previously compared the raw, possibly-escaped bytes directly against conditionValue (an already-plain-text query literal) - so WHERE name = "a" would silently fail to match a source value stored as "\u0061". Now checks for a backslash first and only pays for decoding + an allocation on the rare row that actually needs it.
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