# Sprint Task Assignment Plan

## Team Members

- **Poojan** — Query Search Algorithm
- **Aaron** — JSON Parser and Indexer
- **Tasnim** — Query Parser, interface, error handling & testing

Poojan will also assist Aaron with the Indexer when needed.

---

## Sprint 1: Initial Working Features

### Sprint Goal

Determine the parsing and query algorithms and create the first working versions of dot-path and filter queries using a small dataset.

### Tasks

- Design the Query AST structure.
- Implement the first version of the Query Parser.
- Determine how JSON input will be represented.
- Implement the first version of the JSON Parser.
- Design the first version of the Indexer.
- Define and implement the query search algorithm.
- Implement dot-path lookup.
- Create a small JSON dataset for testing.

---

## Sprint 2: Scaling and Optimization

### Sprint Goal

Scale the algorithms to larger datasets and improve parsing, indexing, and query performance. Create Filter query parsing. Improve testing and edge case range.

### Tasks

- Implement Filter query lookup.
- Test communication between the modules.
- Improve query parsing and error handling.
- Optimize JSON parsing.
- Improve the path and key indexes.
- Optimize dot-path traversal.
- Optimize filter-query searching.
- Add timing and benchmark tests.
- Document the optimization decisions.
- Improve edge case range.


---

## Sprint 3: System Integration

### Sprint Goal

Merge parsing, indexing, and query searching into one working program and obtain a larger dataset for testing.

### Tasks

- Connect the Query Parser to the Query Executor.
- Connect the JSON Parser to the Indexer.
- Connect the Indexer to the query search algorithm.
- Create the main program that coordinates all modules.
- Resolve any integration errors.
- find or create a larger JSON dataset.
- Add integration tests for both query formats.
- Verify output formatting and error messages.


---

## Sprint 4: Testing and Final Demo

### Sprint Goal

Add a user interface if applicable, fully test the final system, fix remaining issues, and prepare the project for the demo.

### Tasks

- Add a command-line or user interface if applicable.
- Test valid and invalid JSON input.
- Test valid and invalid query syntax.
- Test query accuracy and edge cases.
- Run performance tests using the larger dataset.
- Fix remaining issues and integration problems.
- Finalize documentation and setup instructions.
- Prepare for the final demo.


---


