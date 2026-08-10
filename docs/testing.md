# Testing

## Query Parser Unit Tests

Tests include:
- Valid dot paths
- Invalid dot paths
- Wildcards
- Array indexes
- Filter queries
- AND conditions
- Invalid keywords
- Whitespace
- Malformed operators

Expected Outputs for example test cases:

| Test Case | Expected |
|---|---|
| `School.students[*].name` | Parses successfully |
| `School.students[14].year` | Parses successfully |
| `School..students` | Parse error |
| `School.students[-1].name` | Parse error |
| `GET name FROM School.students WHERE year > 2` | Parses successfully |
| `GET name FROM School.students WHERE year > 2 AND grade >= 90` | Parses successfully |
| `GET name FROM School.students WHERE year > 2 AND` | Parse error |

### Parsed Structure Verification

The parser tests also verify that valid queries are converted into the correct internal query structure.

Example:

`School.students[*].name`

Expected structure:

- `Key("School")`
- `Key("students")`
- `AllElements`
- `Key("name")`

AND filter query:

`GET name FROM School.students WHERE year > 2 AND grade >= 90`

Expected:

- Query type: `Filter`
- Select field: `name`
- Source path: `School.students`
- 2 conditions
  - `year > 2`
  - `grade >= 90`


## Integration Tests

Tests the complete pipeline:

JSON file:

→ JSON Parser
→ Query Parser
→ Query Executor
→ Output

Expected Outputs for integration tests:

| Query | Expected Output |
|---|---|
| `items[0].sku` | Value stored in the first item's `sku` field |
| `items[*].inStock` | One result for each array element |
| `GET name FROM store.products WHERE price > 300` | Names of products whose price is greater than 300 |
| `GET name FROM store.products WHERE inStock = true` | Names of products that are in stock |
| `GET name FROM store.products WHERE price > 300 AND inStock = true` | Products satisfying both conditions |

## Edge Cases

- Missing keys
- Empty arrays
- Empty objects
- Null values
- Out-of-range indexes
- Negative indexes
- Deep nesting
- Invalid syntax

Expected Outputs:

### Expected Behavior

- Missing fields return `DNE` when appropriate
- Wildcards over empty arrays return an empty result
- Out-of-range array indexes return `null`
- Negative array indexes are rejected, and return `Error`
- Invalid query syntax produces a parse error instead of crashing
- Invalid keywords are rejected


### Running

query parser:

`g++ -std=c++17 TasnimQueryParser/QueryParser.cpp TasnimQueryParser/QParserTests.cpp -o qparser_tests`

`./qparser_tests`
