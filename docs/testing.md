# Testing

tests include query parser unit tests, integration tests, edge cases and regression tests, and large-dataset tests

## Filter Query Testing

| Test | Query | Expected Result | Purpose |
|---|---|---|---|
| Basic numeric filter | `GET name FROM store.products WHERE price > 300` | `["Laptop", "Mouse", "Wireless Mouse", "Desk", "Chair", "Monitor"]` | numeric comparison |
| Boolean filter | `GET name FROM store.products WHERE inStock = true` | `["Laptop", "Mouse", "Wireless Mouse", "Chair"]` | boolean equality |
| Single-quoted string | `GET price FROM store.products WHERE name = 'Wireless Mouse'` | `[4500]` | quoted strings containing spaces |
| Double-quoted string | `GET price FROM store.products WHERE name = "Wireless Mouse"` | `[4500]` | double-quoted string support |
| Two AND conditions | `GET name FROM store.products WHERE category = electronics AND inStock = true` | `["Laptop", "Mouse", "Wireless Mouse"]` | multiple filter conditions are met |
| Invalid AND | `GET name FROM store.products WHERE price > 300 AND` | Parse error | error for incomplete conditions |
| Wildcard lookup | `Google.employees[*].name` | `["John Doe", "Jane Doe", DNE]` | wildcard traversal and missing-field behavior |
| Array index lookup | `items[0].sku` | `["A1"]` | exact array indexing |
| Deep nesting | `school.classroom.students[*].studentName` | `["Alice", "Bob"]` | traversal through nested objects and arrays |
| Missing key | `foo.bar` | `[DNE]` | missing data is represented as `DNE` |
| Real JSON null | `user.middleName` | `[null]` | separate actual JSON `null` from nonexistent data |
| Invalid array syntax | `items[abc]` | Parse error | rejecting wrong array indexes |
| Negative array index | `items[-1].sku` | Parse error | rejecting negative indices |
| Missing FROM path | `GET name FROM store.nonexistentList WHERE price > 0` | `[]` | nonexistent filter handling (without error) |
| No matching filter rows | `GET name FROM store.products WHERE price > 9999` | `[]` | valid queries return nothing |
| Invalid keyword | `GET name FROMX store.products` | Parse error | test partial keyword matches such as `FROMX` |
| Leading whitespace | `   GET name FROM store.products WHERE price > 300` | result should be as normal | testing whitespace handling |
| Out-of-range array index | `store.products[5654].name` | `[DNE]` | array boundary handling on a large dataset |
| Large-file index lookup | `store.products[5653].name` | `["Smart Chair 5653"]` | testing lookup at last valid index of a large dataset |
| Large-file AND filter | `GET name FROM store.products WHERE category = electronics AND inStock = true` | everythinng that meets both conditions | multiple-conditions at larger scale |
| Large-file no-match filter | `GET name FROM store.products WHERE price > 999999` | `[]` | full filter scan through file with no matching records |


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
