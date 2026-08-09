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

Expected Outputs:


## Integration Tests

Tests the complete pipeline:

JSON file:

→ JSON Parser
→ Query Parser
→ Query Executor
→ Output

Expected Outputs:

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


