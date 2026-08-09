# Sprint 2

## Sprint Goal

In this sprint, we focused mostly on optimizing our JSON parser and adding more supported features.

## Features Completed

### Query Parser
- AND condition for FILTER queries
- benchmarker to check how long it takes to traverse a query
- more testing, better error handling
  ...

### JSON Parser / Indexer
- Optimization for bigger JSON files
- brought down the time from (...) to (...)
- ...

### Query Executor
- more edge case handling (when query contains a string, etc)
- fixing the null return (?)
...
  
## User Stories

### JSON Parser:

As a user, I want to be able to load a large JSON file to query datasets.

As a user, I want to be able to query nested data.

### Query Executer:

As a user, I want a dot-path query to return the data at my exact requested path.

Example:
`items[0].sku`

As a user, I want my filter queries to return the elements satisfying my conditions.

Example:
`GET name FROM store.products WHERE price > 300`

### Query Parser:

**Wildcard Queries**

As a user, I want to be able to access all elements in an array or object from a JSON file with only one query.

Example:
`School.students[*].name`

**Filter Queries**

As a user, I want to be able to access elements that only fall under a certain condition I want.

Example:
`GET name FROM School.students WHERE year > 2`

**Multiple Conditions**

As a user, I want to be able to have multiple conditions and get elements that only satisfy all my conditions.

Example:
`GET name FROM School.students WHERE year > 2 AND grade >= 90`



## Test Cases for Demo

| Feature | Input | Expected Result |
|---|---|---|
| SIMD optimization |---|---|
| Invalid query | ... | ... |
| Filter | ... | ... |
| AND | ... | ... |

## Design / Implementation Changes



## Team Contributions/Commits

### Tasnim
- Query parser
- AND condition
- OR condition (?)
- Parser benchmark
- Parser unit tests
- Docs

### Aaron
- SIMDE & optimization ...

### Poojan
- Fix of errors/edge cases in query parser
- edge case handling
- optimization
...

## Challenges
...

## Performance / Optimization

Our performance improved from
...
to
...

## Sprint 3 Plan:

In sprint 3, we plan to work further on optimization and 
