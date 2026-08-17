# Query Executor & Query Parsing — Algorithm Documentation

**Files covered:** `query_executor.h`, and the `parsePath()` bracket-dispatch logic in `TasnimQueryParser/QueryParser.cpp` (modified jointly for Case 2 special-key support)

---

## 1. Algorithm Documentation

### 1.1 Query Path Parsing — `parsePath()`

Converts a raw query string into a list of `PathPart` steps (Key / ArrayIndex / AllElements). Called by both the dot-path parser and the GET/FROM/WHERE filter parser, since both ultimately need to parse a path expression.

```
FUNCTION parsePath(input, i):
    path = empty list of PathPart

    WHILE i < length(input):

        IF input[i] is whitespace:
            BREAK   # end of this path expression

        # A segment must start with either '[' (bracket-only segment,
        # e.g. special-key or standalone array access) or a bare
        # identifier character.
        IF input[i] != '[' AND input[i] is not alpha AND input[i] != '_':
            THROW "Expected key at position i"

        # --- Bare identifier key (skipped if segment starts with '[') ---
        IF input[i] != '[':
            key = ""
            WHILE i < length(input) AND (input[i] is alnum OR input[i] == '_'):
                key += input[i]
                i += 1
            APPEND PathPart(Key, key) TO path

        # --- Bracket segment: wildcard / array index / quoted key ---
        IF i < length(input) AND input[i] == '[':
            i += 1
            IF i >= length(input):
                THROW "Unclosed bracket at end of query"

            IF input[i] == '*':                          # [*]  wildcard
                i += 1
                IF input[i] != ']': THROW "Unclosed bracket after *"
                i += 1
                APPEND PathPart(AllElements) TO path

            ELSE IF input[i] is digit OR input[i] == '-': # [N]  array index
                digits = ""
                IF input[i] == '-': digits += '-'; i += 1
                IF input[i] is not digit:
                    THROW "'-' must be followed by digits"
                WHILE input[i] is digit:
                    digits += input[i]; i += 1
                index = toInt(digits)
                IF index < 0:
                    THROW "negative array indices not supported"
                IF input[i] != ']': THROW "Unclosed bracket after array index"
                i += 1
                APPEND PathPart(ArrayIndex, index) TO path

            ELSE IF input[i] == '"':                      # ["key"]  quoted key  [NEW]
                i += 1                                     # skip opening quote
                keyContent = ""
                WHILE i < length(input) AND input[i] != '"':
                    keyContent += input[i]
                    i += 1
                IF i >= length(input):
                    THROW "Unterminated quoted key in brackets"
                i += 1                                     # skip closing quote
                IF input[i] != ']':
                    THROW "Unclosed bracket after quoted key"
                i += 1
                APPEND PathPart(Key, keyContent) TO path

            ELSE:
                THROW "missing array index, *, or quoted key in brackets"

        IF i < length(input) AND input[i] is whitespace:
            BREAK

        # --- Segment separator ---
        IF i < length(input):
            IF input[i] != '.':
                THROW "Expected '.' at position i"
            i += 1
            IF i >= length(input):
                THROW "Query is ending with a dot"

    RETURN path
```

**Design notes:**
- The quoted-key branch exists to address two classes of object key that dot-notation cannot express: a literal `"."` key (would collide with the path separator) and an empty-string key `""` (no bare-identifier form exists at all).
- Chained brackets with no dot between them (e.g. `items[0]["weird.key"]`) are deliberately unsupported — the loop only checks for one bracket per segment, so this correctly throws `"Expected '.'"` rather than misparsing.

---

### 1.2 Dot-Path Traversal — `executeStep()`

Recursively walks the parsed JSON tree, following one `PathPart` per recursive call. This is the core traversal algorithm used by every dot-path query.

```
FUNCTION executeStep(node, path, stepIndex, results):
    IF node is NULL:
        APPEND NULL TO results        # "DNE" at display time
        RETURN

    IF stepIndex == length(path):
        APPEND node TO results        # full path consumed -- this is the answer
        RETURN

    part = path[stepIndex]

    SWITCH part.type:

        CASE Key:
            IF node.type == Object:
                child = node.objectChildNode.find(part.key)
                IF child found:
                    executeStep(child, path, stepIndex + 1, results)
                    RETURN
            APPEND NULL TO results     # key missing, or node isn't an object

        CASE AllElements:              # [*]
            IF node.type == Array:
                FOR EACH element IN node.arrayChildNode:
                    executeStep(element, path, stepIndex + 1, results)
            ELSE:
                APPEND NULL TO results # wildcard on a non-array

        CASE ArrayIndex:               # [N]
            IF node.type == Array AND 0 <= part.index < size(node.arrayChildNode):
                element = node.arrayChildNode[part.index]   # O(n) via std::advance
                executeStep(element, path, stepIndex + 1, results)
            ELSE:
                APPEND NULL TO results # out of range, or node isn't an array
```

**Complexity note:** array indexing is O(n) per index due to `std::advance` over `std::list` (chosen for pointer stability during tree construction, since `Node` is self-referential and `std::deque` doesn't guarantee support for incomplete/self-referential element types under the C++ standard).

---

### 1.3 Single-Node Resolve — `resolveSingle()`

A non-allocating variant of `executeStep()` for paths that only ever need to resolve to one node — exactly what a WHERE condition's field lookup needs (no wildcards expected there). Avoids a heap-allocated `vector` per call, which matters when scanning N conditions × M rows.

```
FUNCTION resolveSingle(node, path, stepIndex) -> Node* or NULL:
    IF node is NULL: RETURN NULL
    IF stepIndex == length(path): RETURN node

    part = path[stepIndex]

    SWITCH part.type:

        CASE Key:
            IF node.type == Object:
                child = node.objectChildNode.find(part.key)
                IF child found:
                    RETURN resolveSingle(child, path, stepIndex + 1)
            RETURN NULL

        CASE ArrayIndex:
            IF node.type == Array AND index in range:
                element = node.arrayChildNode[part.index]
                RETURN resolveSingle(element, path, stepIndex + 1)
            RETURN NULL

        CASE AllElements:
            RETURN NULL   # wildcard is inherently multi-result; not
                          # representable as a single resolved node
```

---

### 1.4 Filter Query Evaluation — `executeFilterQuery()` (GET / FROM / WHERE)

The main filter algorithm. Three steps matching the query's three clauses.

```
FUNCTION executeFilterQuery(root, filter, jsonData) -> list of Node*:
    results = empty list

    # Step 1 (FROM): resolve the source array
    sources = executeStep(root, filter.sourcePath, 0)

    FOR EACH source IN sources:
        IF source is NULL OR source.type != Array:
            CONTINUE                    # FROM must resolve to an array

        reserve(results, size(results) + size(source.arrayChildNode))

        # Step 2 (WHERE): scan every row, AND semantics across conditions
        FOR EACH row IN source.arrayChildNode:
            include = TRUE
            FOR EACH condition IN filter.conditions:
                fieldNode = resolveSingle(row, condition.field, 0)
                IF fieldNode is NULL OR NOT compareValues(fieldNode, jsonData, condition.op, condition.value):
                    include = FALSE
                    BREAK
            IF NOT include:
                CONTINUE

            # Step 3 (GET): resolve the selected field for this row
            selected = executeStep(row, filter.selectField, 0)
            APPEND ALL selected TO results

    RETURN results
```

`compareValues()` does a numeric compare (`std::stod` both sides) when the field node is a `number`, and a lexical string compare otherwise (covering strings and booleans, e.g. `WHERE active = true`). Non-numeric values on either side of a numeric comparison are treated as a non-match rather than thrown.

---

## 2. System Usage Flow Documentation

This system is consumed as a **library** (linked into `main.cpp` / `test_executor`), not as an interactive CLI or GUI with distinct states — so this section documents the exposed API surface and typical usage, rather than a state diagram.

### 2.1 Exposed API

| Function | Signature | Purpose |
|---|---|---|
| `QueryParser::parse` | `Query parse(const std::string& input) const` | Parses a raw query string (dot-path or `GET/FROM/WHERE`) into a `Query` struct |
| `executeQuery` (dot-path) | `vector<const Node*> executeQuery(const Node& root, const DotPathQuery& query)` | Runs a parsed dot-path query against a tree |
| `executeQuery` (convenience) | `vector<const Node*> executeQuery(const Node& root, const Query& query)` | Unwraps `query.dotPath` automatically; throws if `query` is a filter query |
| `executeQuery` (full entry point) | `vector<const Node*> executeQuery(const Node& root, const Query& query, const vector<char>& jsonData)` | Handles **both** dot-path and filter queries; the one most callers should use |
| `executeFilterQuery` | `vector<const Node*> executeFilterQuery(const Node& root, const FilterQuery& filter, const vector<char>& jsonData)` | The GET/FROM/WHERE algorithm directly, if you already know it's a filter query |
| `nodeToString` | `std::string nodeToString(const Node* node, const vector<char>& jsonData)` | Human-readable display string for a result node (decodes escapes, "DNE" for null pointer, "null" for real JSON null) |
| `nodeRawValue` | `std::string_view nodeRawValue(const Node* node, const vector<char>& jsonData)` | Bare comparable value with no quotes/formatting, used internally by WHERE comparisons |
| `compareValues` | `bool compareValues(const Node* node, const vector<char>& jsonData, FilterOperators op, const string& conditionValue)` | Evaluates a single WHERE comparison |
| `decodeJsonString` | `std::string decodeJsonString(std::string_view raw)` | Decodes JSON escape sequences (`\n`, `\"`, `\uXXXX` incl. surrogate pairs) into real UTF-8 bytes |

### 2.2 Typical Use Cases

**Dot-path query:**
```cpp
parser p;
p.loadFile("data.json");
p.indexStructure();
p.constructTree();

QueryParser qp;
Query q = qp.parse("store.products[*].name");

auto results = executeQuery(p.getRoot(), q, p.getJsonData());
for (const auto* node : results) {
    std::cout << nodeToString(node, p.getJsonData()) << std::endl;
}
```

**Filter query (GET/FROM/WHERE):**
```cpp
Query q = qp.parse("GET name FROM store.products WHERE price > 300");
auto results = executeQuery(p.getRoot(), q, p.getJsonData());
```

**Special-key query (Case 2 — bracket-quoted key):**
```cpp
// Addresses an object key that dot-notation itself can't express,
// e.g. querying {"." : "bob"}
Query q = qp.parse("[\".\"]");
auto results = executeQuery(p.getRoot(), q, p.getJsonData());
// results[0] -> "bob"
```
